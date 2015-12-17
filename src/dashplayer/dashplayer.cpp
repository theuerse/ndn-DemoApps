#include "dashplayer.hpp"

using namespace player;

DashPlayer::DashPlayer(std::string MPD, int interest_lifetime)
{
    this->interest_lifetime = interest_lifetime;
    streaming_active = false;
    mpd_url=MPD;
    mpd = NULL;
    base_url = "";
}

DashPlayer::~DashPlayer()
{
}

void DashPlayer::startStreaming ()
{
  //1fetch MPD and parse MPD
  std::string mpd_path("/tmp/video.mpd");
  shared_ptr<itec::Buffer> mpd_data = downloader.getFile (mpd_url);
  writeFileToDisk(mpd_data, mpd_path);

  if(!parseMPD(mpd_path))
    return;

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
  fprintf(stderr, "downloading segment...\n");
  sleep(3);
}

void DashPlayer::schedulePlayback ()
{
  fprintf(stderr, "doing playback\n");
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
    cerr << "lifetime       ... The lifetime of the interest in milliseconds. (Default 1000ms)" << endl;
    cerr << "usage-example: " << "./" << appName << " --name /example/testApp/randomData" << endl;
    cerr << "usage-example: " << "./" << appName << " --name /example/testApp/randomData --lifetime 1000" << endl;

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
  DashPlayer consumer(vm["name"].as<string>(), lifetime);

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
