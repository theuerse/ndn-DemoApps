#include "dashplayer.hpp"

using namespace player;

DashPlayer::DashPlayer(std::string MPD, string adaptionlogic_name, int interest_lifetime)
{
    this->adaptionlogic_name = adaptionlogic_name;
    this->interest_lifetime = interest_lifetime;

    streaming_active = false;

    mpd_url=MPD;
    mpd = NULL;

    base_url = "";

    max_buffered_seconds=50;
    mbuffer = boost::shared_ptr<MultimediaBuffer>(new MultimediaBuffer(max_buffered_seconds));

    //factory - to be replaced
    alogic = AdaptationLogicFactory::Create(adaptionlogic_name, this); //TODO
    downloader = boost::shared_ptr<FileDownloader>(new FileDownloader(this->interest_lifetime ));

    hasDownloadedAllSegments = false;
    isStalling=false;

    requestedRepresentation = NULL;
    requestedSegmentNr = 0;
    requestedSegmentURL = NULL;

    setMyId ();

    logFile.open ("dashplayer_trace" + myId + ".txt", ios::out); // keep logfile open until app shutdown
    logFilePrintHeader();
}

DashPlayer::~DashPlayer()
{
  logFile.close ();
}

void DashPlayer::startStreaming ()
{
  //1fetch MPD and parse MPD
  std::string mpd_path("/tmp/video.mpd");
  shared_ptr<itec::Buffer> mpd_data = downloader->getFile (mpd_url);

  if(mpd_data == NULL)
  {
    fprintf(stderr, "Error fetching MPD! Exiting..\n");
    return;
  }

  writeFileToDisk(mpd_data, mpd_path);

  fprintf(stderr, "parsing...\n");

  if(!parseMPD(mpd_path))
    return;

  alogic->SetAvailableRepresentations (availableRepresentations);

  //2. start streaming (1. thread)
  boost::thread downloadThread(&DashPlayer::scheduleDownloadNextSegment, this);

  //3. start consuming (2. thread)
  boost::thread playbackThread(&DashPlayer::schedulePlayback, this);

  //wait until threads finished
  downloadThread.join ();
  playbackThread.join ();

  exit(0);
}

void DashPlayer::scheduleDownloadNextSegment ()
{

  while(!hasDownloadedAllSegments)
  {
    requestedRepresentation = NULL;
    requestedSegmentNr = 0;

    requestedSegmentURL = alogic->GetNextSegment(&requestedSegmentNr, &requestedRepresentation, &hasDownloadedAllSegments);

    if(requestedSegmentURL == NULL) // IDLE e.g. buffer is full
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(500));
      continue;
    }

    fprintf(stderr, "downloading segment = %s\n",(base_url+requestedSegmentURL->GetMediaURI()).c_str ());

    shared_ptr<itec::Buffer> segmentData = downloader->getFile (base_url+requestedSegmentURL->GetMediaURI());

    if(segmentData->getSize() != 0)
    {
      while(!mbuffer->addToBuffer (requestedSegmentNr, requestedRepresentation));
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }
  }
}

void DashPlayer::schedulePlayback ()
{
  boost::posix_time::ptime stallEndTime;
  boost::posix_time::time_duration stallDuration;

  player::MultimediaBuffer::BufferRepresentationEntry entry = mbuffer->consumeFromBuffer ();;

  // did we finish streaming yet?
  while(! (entry.segmentDuration == 0.0 && hasDownloadedAllSegments == true) )
  {
    if ( entry.segmentDuration > 0.0)
    {
      fprintf(stderr, "Consumed Segment %d, with Rep %s for %f seconds\n",entry.segmentNumber,entry.repId.c_str(), entry.segmentDuration);

      if (isStalling)
      {
        stallEndTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());

        // we had a freeze/stall, but we can continue playing now
        stallDuration = (stallEndTime - stallStartTime);
        isStalling = false;
      }
      else
        stallDuration = boost::posix_time::time_duration(0,0,0,0);

      logSegmentConsume(entry,stallDuration);
      boost::this_thread::sleep(boost::posix_time::milliseconds(entry.segmentDuration*1000)); //consume the segment
    }
    else
    {
      if ( isStalling == false) // could not consume, means buffer is empty
      {
        isStalling = true;
        stallStartTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    //check for download abort
    if(requestedRepresentation != NULL && !hasDownloadedAllSegments && requestedRepresentation->GetDependencyId ().size () > 0)
    {
      if(! (alogic->hasMinBufferLevel (requestedRepresentation)))
      {
        fprintf(stderr, "canceling: %s", requestedSegmentURL->GetMediaURI ().c_str ());
        downloader->cancel ();
      }
    }

    entry = mbuffer->consumeFromBuffer ();
  }
}

bool DashPlayer::parseMPD(std::string mpd_path)
{
  dash::IDASHManager *manager;
  manager = CreateDashManager();
  mpd = manager->Open((char*)mpd_path.c_str ());

  // We don't need the manager anymore...
  manager->Delete();
  manager = NULL;

  if (mpd == NULL)
  {
    fprintf(stderr, "MPD - Parsing Error. Exiting..\n");
    return false;
  }

  // we are assuming there is only 1 period, get the first one
  IPeriod *currentPeriod = mpd->GetPeriods().at(0);

  // get base URL (we take first always)
  std::vector<dash::mpd::IBaseUrl*> baseUrls = mpd->GetBaseUrls ();
  if(baseUrls.size () < 1)
  {
    fprintf(stderr, "No BaseUrl detected!\n");
    return false;
  }
  base_url = baseUrls.at (0)->GetUrl();
  if(base_url.substr (0,7).compare ("http://") == 0)
    base_url = base_url.substr(6,base_url.length ());

  // Get the adaptation sets, though we are only consider the first one
  std::vector<IAdaptationSet *> allAdaptationSets = currentPeriod->GetAdaptationSets();

  if (allAdaptationSets.size() < 1)
  {
    fprintf(stderr, "No adaptation sets found in MPD!\n");
    return false;
  }

  IAdaptationSet* adaptationSet = allAdaptationSets.at(0);
  std::vector<IRepresentation*> reps = adaptationSet->GetRepresentation();

  if(reps.size () < 1)
  {
    fprintf(stderr, "No represntations found in MPD!\n");
    return false;
  }


  availableRepresentations.clear();
  for (IRepresentation* rep : reps)
  {
    std::string repId = rep->GetId();
    availableRepresentations[repId] = rep;
  }

  return true;
}

void DashPlayer::writeFileToDisk(shared_ptr<itec::Buffer> buf, string file_path)
{
  ofstream outputStream;
  outputStream.open(file_path, ios::out | ios::binary);
  outputStream.write(buf->getData(), buf->getSize());
  outputStream.close();
}

//
// Main entry-point
//
int main(int argc, char** argv)
{
  string appName = boost::filesystem::basename(argv[0]);

  options_description desc("Programm Options");
  desc.add_options ()
      ("name,p", value<string>()->required (), "The name of the interest to be sent (Required)")
      ("adaptionlogic,a",value<string>()->required(), "The name of the adaption-logic to be used (Required)")
      ("lifetime,s", value<int>(), "The lifetime of the interest in milliseconds. (Default 1000ms)");

  positional_options_description positionalOptions;
  variables_map vm;

  try
  {
    store(command_line_parser(argc, argv).options(desc)
                .positional(positionalOptions).run(),
              vm); // throws on error

    notify(vm); //notify if required parameters are not provided.
  }
  catch(boost::program_options::required_option& e)
  {
    // user forgot to provide a required option
    cerr << "name           ... The name of the interest to be sent (Required)" << endl;
    cerr << "adaptionlogic  ... The name of the adaption-logic to be used (Required, buffer or rate)" << endl;
    cerr << "lifetime       ... The lifetime of the interest in milliseconds. (Default 1000ms)" << endl;
    cerr << "usage-example: " << "./" << appName << " --name /example/testApp/randomData --adaptionlogic buffer" << endl;
    cerr << "usage-example: " << "./" << appName << " --name /example/testApp/randomData --adaptionlogic buffer --lifetime 1000" << endl;

    cerr << "ERROR: " << e.what() << endl << endl;
    return -1;
  }
  catch(boost::program_options::error& e)
  {
    // a given parameter is faulty (e.g. given a string, asked for int)
    cerr << "ERROR: " << e.what() << endl << endl;
    return -1;
  }
  catch(exception& e)
  {
    cerr << "Unhandled Exception reached the top of main: "
              << e.what() << ", application will now exit" << endl;
    return -1;
  }

  int lifetime = 1000;
  if(vm.count ("lifetime"))
  {
    lifetime = vm["lifetime"].as<int>();
  }

  // create new DashPlayer instance with given parameters
  DashPlayer consumer(vm["name"].as<string>(),vm["adaptionlogic"].as<string>(), lifetime);

  try
  {
    //get MPD and start streaming
    consumer.startStreaming();
  }
  catch (const exception& e)
  {
    // shit happens
    cerr << "ERROR: " << e.what() << endl;
  }

  return 0;
}

double DashPlayer::GetBufferLevel(std::string repId)
{
  if(repId.compare ("NULL") == 0)
    return mbuffer->getBufferedSeconds ();
  else
    return mbuffer->getBufferedSeconds (repId);
}

unsigned int DashPlayer::nextSegmentNrToConsume ()
{
  return mbuffer->nextSegmentNrToBeConsumed ();
}

unsigned int DashPlayer::getHighestBufferedSegmentNr(std::string repId)
{
  return mbuffer->getHighestBufferedSegmentNr (repId);
}

void DashPlayer::logFilePrintHeader ()
{
  logFile << "Time"
     << "\t"
     << "Node"
     << "\t"
     << "SegmentNumber"
     << "\t"
     << "SegmentDuration(sec)"
     << "\t"
     << "SegmentRepID"
     << "\t"
     << "SegmentBitrate(bit/s)"
     << "\t"
     << "StallingTime(msec)"
     << "\t"
     << "SegmentDepIds\n";
  logFile.flush ();
}

void  DashPlayer::logSegmentConsume(player::MultimediaBuffer::BufferRepresentationEntry &entry, boost::posix_time::time_duration& stallingTime)
{
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

  logFile << boost::posix_time::to_simple_string (now)  << "\t"
          << "PI_" << myId  << "\t"
          << entry.segmentNumber  << "\t"
          << entry.segmentDuration  << "\t"
          << entry.repId << "\t"
          << entry.bitrate_bit_s << "\t"
          << stallingTime.total_milliseconds () << "\t";

  for(unsigned int i = 0; i < entry.depIds.size (); i++)
  {
    if(i == 0)
      logFile << entry.depIds.at (i);
    else
      logFile << "," <<entry.depIds.at (i);
  }
  logFile << "\n";

  logFile.flush ();
}

void DashPlayer::setMyId()
{
  myId = "0";

  int fd;
  struct ifreq ifr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "eth0" */
  strncpy(ifr.ifr_name, "eth0.101", IFNAMSIZ-1);

  if(ioctl(fd, SIOCGIFADDR, &ifr) != 0)
  {
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
  }

  close(fd);

  std::string ip(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

  std::vector<std::string> ip_octets;
  boost::split(ip_octets, ip, boost::is_any_of("."));

  if(ip_octets.size () != 4)
  {
    fprintf(stderr, "Invalid IP using default id = 0\n");
    return;
  }

  int id = boost::lexical_cast<int>(ip_octets.at (3));

  if(id < 10)
  {
    fprintf(stderr, "Are you deploying this on the PInet? Setting default id = 0\n");
    return;
  }

  myId = boost::lexical_cast<std::string>(id-10);
}
