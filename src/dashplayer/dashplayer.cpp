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

    last_requestedRepresentation = NULL;
    last_requestedSegmentNr = 0;
}

DashPlayer::~DashPlayer()
{
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
  const dash::mpd::IRepresentation* requestedRepresentation = NULL;
  unsigned int requestedSegmentNr = 0;
  dash::mpd::ISegmentURL* requestedSegmentURL = alogic->GetNextSegment(&requestedSegmentNr, &requestedRepresentation, &hasDownloadedAllSegments);

  if(hasDownloadedAllSegments) // we got all segmetns, exit download thread
    return;

  if(requestedSegmentURL == NULL) // IDLE e.g. buffer is full
  {
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    scheduleDownloadNextSegment();
    return;
  }

  fprintf(stderr, "downloading segment = %s\n",(base_url+requestedSegmentURL->GetMediaURI()).c_str ());

  last_requestedRepresentation = requestedRepresentation;
  last_requestedSegmentNr = requestedSegmentNr;

  shared_ptr<itec::Buffer> segmentData = downloader->getFile (base_url+requestedSegmentURL->GetMediaURI());

  if(segmentData != NULL)
  {
    mbuffer->addToBuffer (requestedSegmentNr, requestedRepresentation);
  }

  scheduleDownloadNextSegment();
}

void DashPlayer::schedulePlayback ()
{

  fprintf(stderr, "Schedule playback\n");

  unsigned int buffer_level = mbuffer->getBufferedSeconds();

  // did we finish streaming yet?
  if (buffer_level == 0 && hasDownloadedAllSegments == true)
     return; //finished streaming

  player::MultimediaBuffer::BufferRepresentationEntry entry = mbuffer->consumeFromBuffer ();
  double consumedSeconds = entry.segmentDuration;
  if ( consumedSeconds > 0)
  {
    fprintf(stderr, "Consumed Segment %d, with Rep %s for %f seconds\n",entry.segmentNumber,entry.repId.c_str(), entry.segmentDuration);

    if (isStalling)
    {
      boost::posix_time::ptime stallEndTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
      boost::posix_time::time_duration stallDuration;

      // we had a freeze/stall, but we can continue playing now
      stallDuration = (stallEndTime - stallStartTime);
      isStalling = false;
    }

    /*m_playerTracer(this, entry.segmentNumber, entry.segmentDuration, entry.repId,
                   entry.bitrate_bit_s, freezeTime, entry.depIds);*/
  }
  else
  {
    // could not consume, means buffer is empty
    if ( isStalling == false)
    {
      isStalling = true;
      stallStartTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
    }
  }

  if(consumedSeconds == 0.0) // we are stalling
  {
    //check for download abort
    if(last_requestedRepresentation != NULL && !hasDownloadedAllSegments && last_requestedRepresentation->GetDependencyId ().size ()>0)
    {
      if(alogic->hasMinBufferLevel (last_requestedRepresentation))
      {
        //TODO abort download of current segment!
      }
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
  }
  else
  {
    boost::this_thread::sleep(boost::posix_time::milliseconds(consumedSeconds*1000));
  }
  schedulePlayback();
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
