#include "Producer.hpp"

// public methods
ndn::Producer::Producer(string prefix, string document_root, int data_size, int freshness_seconds)
{
    this->prefix = prefix;
    this->document_root =  (document_root.back() == '/') ? document_root.erase(document_root.size()-1) : document_root;
    this-> data_size = data_size;
    this->freshness_seconds = freshness_seconds;
}

ndn::Producer::~Producer()
{
}

// register prefix on NFD
void ndn::Producer::run()
{
    m_face.setInterestFilter(this->prefix,
                             bind(&ndn::Producer::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&ndn::Producer::onRegisterFailed, this, _1, _2));
    m_face.processEvents();
}


// private methods
// generate random content of given length
string ndn::Producer::generateContent(const int length)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    string content;

    for (int i = 0; i < length; ++i)
        content += alphanum[rand() % (sizeof(alphanum) - 1)];
    return content;
}

// get file-content
ndn::Producer::file_chunk_t ndn::Producer::getFileContent(const Interest& interest)
{
    file_chunk_t result;
    char* buffer;
    int buffer_size;
    Name interestName = interest.getName();

    // get sequence number
    int seq_nr = interestName.at(-1).toSequenceNumber();
    interestName = interestName.getPrefix(-1); // remove seq-Nr
    cout << "Info: seq-nr: " << seq_nr << endl;

    fprintf(stderr, "interestName=%s\n",interestName.toUri ().c_str ());
    fprintf(stderr, "prefix=%s\n",prefix.c_str ());
    fprintf(stderr, "document_root=%s\n",document_root.c_str ());

    std::string fname = interestName.toUri ();
    fname = fname.substr (prefix.length ());

    // get remaining filename (ignore prefix) //TODO: support multiple level prefix?
    string file_path = this->document_root + fname;
    cout << "opening " << file_path << endl;

    // try to open file
    ifstream inputStream;
    cout << "opening file: " << file_path << endl;
    inputStream.open(file_path, ios::binary);

    // get length of file:
    inputStream.seekg (0, inputStream.end);
    int file_length = inputStream.tellg();
    cout << "file_length:" << file_length  << " byte(s)"<< endl;

    // cope with request of chunk who doesn't exists
    int chunk_count = ceil((double)file_length / (double)this->data_size);
    cout << "chunk_count " << chunk_count << endl;

    if(seq_nr > chunk_count-1)
    {
        cout << "out of bounds" << endl;
        result.success=false;
        return result;
    }

    inputStream.seekg(this->data_size * seq_nr); // seek
    int pos = inputStream.tellg();
    cout << "lesen ab position " << pos << endl;

    buffer_size = min(file_length - pos, this->data_size);
    buffer = new char [buffer_size];

    inputStream.read (buffer,buffer_size);
    cout << "buffersize: " << buffer_size << endl;
    cout << buffer << endl;
    inputStream.close();;

    cout << "done" << endl;
    result.success = true;
    result.buffer = shared_ptr<itec::Buffer>( new itec::Buffer(buffer, buffer_size));
    //delete[] buffer;
    result.final_block_id = chunk_count - 1;
    return result;
}

// react to arrival of a Interest-Package
void ndn::Producer::onInterest(const InterestFilter& filter, const Interest& interest)
{
    cout << "Received Interest: " << interest << endl;

    // Create new name, based on Interest's name
    Name dataName(interest.getName());

    // DEBUG: have a look at infos in Interest
    cout << "Interest-name:" << interest.getName() << endl;

    file_chunk_t result = getFileContent(interest);
    if(result.success)
    {
        dataName.appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

        //string content = generateContent(data_size); // fake data creation

        // Create Data packet
        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(freshness_seconds));
        //data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
        data->setContent((uint8_t* )result.buffer->getData(), result.buffer->getSize ());
        data->setFinalBlockId(ndn::Name::Component(std::to_string(result.final_block_id)));

        // Sign Data packet with default identity
        m_keyChain.sign(*data);

        // Return Data packet
        m_face.put(*data);
    }
}

// react to failure of prefix-registration
void ndn::Producer::onRegisterFailed(const Name& prefix, const std::string& reason)
{
      cerr << "ERROR: Failed to register prefix \""
                << prefix << "\" in local hub's daemon (" << reason << ")"
                << endl;
    m_face.shutdown();
}

//
// Main entry-point
//
int main(int argc, char** argv)
{
  string appName = boost::filesystem::basename(argv[0]);

  options_description desc("Programm Options");
  desc.add_options ()
      ("prefix,p", value<string>()->required (), "Prefix the Producer listens too. (Required)")
      ("document-root,b", value<string>()->required (), "The directory open to requests (Interests). (Required)")
      ("data-size,s", value<int>()->required (), "The size of the datapacket in bytes. (Required)")
      ("freshness-time,f", value<int>(), "Freshness time of the content in seconds. (Default 5min)");

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
    cerr << "prefix         ... Prefix the Producer listens to. (Required)" << endl;
    cerr << "document-root  ... The directory open to requests (Interests). (Required)" << endl;
    cerr << "data-size      ... The size of the datapacket in bytes  (Required)" << endl;
    cerr << "freshness-time ... Freshness time of the content in seconds (Default 5 min)" << endl;
    cerr << "usage-example: " << "./" << appName << " --prefix foo --document-root ./ --data-size 4096" << endl;
    cerr << "usage-example: " << "./" << appName << " --prefix foo --document-root ./ --data-size 4096 --freshness-time 300" << endl;

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

  int freshness_time = 300;
  if(vm.count ("freshness-time"))
  {
    freshness_time = vm["freshness-time"].as<int>();
  }

  // fail if document-root does not exist
  if(!boost::filesystem::is_directory(boost::filesystem::status(vm["document-root"].as<string>())))
  {
    throw invalid_argument("document-root does not exist");
  }
  cout << "document-root initialized with: " << vm["document-root"].as<string>() << endl;

  // create new Producer instance with given parameters
  ndn::Producer producer(vm["prefix"].as<string>(),
                         vm["document-root"].as<string>(),
                         vm["data-size"].as<int>(),
                         freshness_time);

  try
  {
    // start producer
    producer.run();
  }
  catch (const exception& e)
  {
    // shit happens
    cerr << "ERROR: " << e.what() << endl;
  }

  return 0;
}
