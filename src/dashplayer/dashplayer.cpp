#include "dashplayer.hpp"
// public methods
ndn::DashPlayer::DashPlayer(std::string MPD, int interest_lifetime)
{
    this->interest_lifetime = interest_lifetime;
    streaming_active = false;
    this->MPD=MPD;
}

ndn::DashPlayer::~DashPlayer()
{
}

void ndn::DashPlayer::startStreaming ()
{
  //1fetch MPD
  shared_ptr<itec::Buffer> mpd_buffer = downloader.getFile (MPD);
  writeFileToDisk(mpd_buffer, "/tmp/video.mpd");

  //2. start streaming (1. thread)

  //3. start consuming (2. thread)

  //wait until threads finished
  exit(0);
}

void ndn::DashPlayer::writeFileToDisk(shared_ptr<itec::Buffer> buf, string file_path)
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
  ndn::DashPlayer consumer(vm["name"].as<string>(), lifetime);

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
