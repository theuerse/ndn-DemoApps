#include "FileConsumer.h"
// public methods
ndn::FileConsumer::FileConsumer(string interest_name, int seq_nr, int interest_lifetime)
{
    this->interest_name = interest_name;
    this->interest_lifetime = interest_lifetime;
    this->seq_nr = seq_nr;
}

ndn::FileConsumer::~FileConsumer()
{
    cout << "Bye you cruel world" << endl;
}

// register prefix on NFD
void ndn::FileConsumer::run()
{
    sendInterest(this->seq_nr);
    // processEvents will block until the requested data received or timeout occurs
    m_face.processEvents();
}

void ndn::FileConsumer::sendInterest(int seq_nr)
{
     Interest interest(Name(this->interest_name).appendSequenceNumber(this->seq_nr));  // e.g. "/example/testApp/randomData"
    // appendSequenceNumber
    interest.setInterestLifetime(time::milliseconds(this->interest_lifetime));   // time::milliseconds(1000)
    interest.setMustBeFresh(true);

    m_face.expressInterest(interest,
                           bind(&ndn::FileConsumer::onData, this,  _1, _2),
                           bind(&ndn::FileConsumer::onTimeout, this, _1));

    cout << "Sending " << interest << endl;
}


// private methods
// react to the reception of a reply from a Producer
void ndn::FileConsumer::onData(const Interest& interest, const Data& data)
{
    cout << "data-packet received: " << endl;

    if(this->first_data_received){
        // allocate space!!
        this->first_data_received = false;
        //this->
    }
    const Block& block = data.getContent();
    std::cout.write((const char*)block.value(),block.value_size());
    cout << endl;
    cout << "finalBlockId: " << data.getFinalBlockId() << endl;
}

// react on the request / Interest timing out
void ndn::FileConsumer::onTimeout(const Interest& interest)
{
    cout << "Timeout " << interest << endl;
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

  // create new FileConsumer instance with given parameters
  // get sequence number
    string name = vm["name"].as<string>();
    vector<string> parts;
    boost::split(parts, name, boost::is_any_of("/"));
    int seq_nr = 0;
    try {
        seq_nr = boost::lexical_cast<int>(parts[parts.size()-1]);
        name = name.erase(name.find_last_of('/'));
    } catch( boost::bad_lexical_cast const& ) {
        cout << "Info: no seq_nr given, using default of 0" << endl;
    }
    cout << "Info: seq-nr: " << seq_nr << endl;

    cout << "name: " << name << endl;
    ndn::FileConsumer consumer(name,seq_nr,lifetime);

  try
  {
    // start file consumer
    consumer.run();
  }
  catch (const exception& e)
  {
    // shit happens
    cerr << "ERROR: " << e.what() << endl;
  }

  return 0;
}
