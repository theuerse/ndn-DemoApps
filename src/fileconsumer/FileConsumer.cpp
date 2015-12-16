#include "FileConsumer.hpp"
// public methods
ndn::FileConsumer::FileConsumer(int interest_lifetime)
{
    this->interest_lifetime = interest_lifetime;
}

ndn::FileConsumer::~FileConsumer()
{
}

void ndn::FileConsumer::getFile(string name){
    this->buffer.clear();
    this->file_name = name;
    sendInterest(0); // send first interest-packet
    // processEvents will block until the requested data received or timeout occurs
    m_face.processEvents();
}

void ndn::FileConsumer::sendInterest(int seq_nr)
{
     Interest interest(Name(this->file_name).appendSequenceNumber(seq_nr));  // e.g. "/example/testApp/randomData"
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
    // get sequence number
    int seq_nr = interest.getName().at(-1).toSequenceNumber();

    cout << "data-packet #" << seq_nr <<  " received: " << endl;

    const Block& block = data.getContent();

    if(this->buffer.empty()){
         // first data-packet arrived, allocate space
        this->finalBockId = boost::lexical_cast<int>(data.getFinalBlockId().toUri());
        int buffer_size = this->finalBockId + 1;

        cout << "init buffer_size: " << buffer_size << endl;
        this->buffer.reserve(buffer_size);
    }
    // Debug-output:
    std::cout.write((const char*)block.value(),block.value_size());
    cout << endl;

    // store received data in buffer
    string chunk((const char*)block.value(), block.value_size());
    this->buffer.insert(this->buffer.begin() + seq_nr, chunk);

    // request next one
    if(seq_nr < this->finalBockId){
        sendInterest(seq_nr + 1);
    }else{
        cout << "got all " << seq_nr + 1 << " parts" << endl;
        flushBufferToFile("./out.txt"); // DEBUG fixture
    }
}

void ndn::FileConsumer::flushBufferToFile(string path){
    fstream outputStream(path, ios::out | ios::trunc); // open for write, drop prev content
    outputStream.close(); // file exists now
    outputStream.open(path, ios::out);

    for(uint index = 0; index < this->buffer.size(); index++){
        outputStream.write(this->buffer[index].c_str(), this->buffer[index].size());
    }
    outputStream.close();

    this->buffer.clear();
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
    ndn::FileConsumer consumer(lifetime);

  try
  {
    // start file consumer
    consumer.getFile(vm["name"].as<string>());
  }
  catch (const exception& e)
  {
    // shit happens
    cerr << "ERROR: " << e.what() << endl;
  }

  return 0;
}
