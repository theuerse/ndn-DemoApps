#include "filedownloader.hpp"

using namespace player;

FileDownloader::FileDownloader(int interest_lifetime)
{
  this->interest_lifetime = interest_lifetime;
}

shared_ptr<itec::Buffer> FileDownloader::getFile(string name){
  this->buffer.clear();
  this->state = process_state::startup;
  //this->bitrate = ....  uebergeben!
  this->file = shared_ptr<itec::Buffer>(new itec::Buffer());
  this->file_name = name;

  download(); // start downloading

  // processEvents will block until the requested data received or timeout occurs
  m_face.processEvents();

  return file;
}

// (start) downloading the file
void FileDownloader::download(){
  this->buffer.resize(5,chunk()); // insert initial 5 entries (requested)

  do
  {
    // check if caller cancelled
    if(this->state == process_state::cancelled)
    {
      m_face.shutdown(); // dangerous?
      return; // cancelled, do not touch file (-buffer)
    }

    // send bitrate / interestsize? (e.g. 40) interests out
    sendNextInterests(40);

    // wait for a sec
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
  }while(!allChunksReceived());

  // all chunks / file has been downloaded
  // unify buffers and return
  for(vector<chunk>::iterator it = buffer.begin (); it != buffer.end (); it++)
  {
    file->append((*it).data->getData(),(*it).data->getSize());
  }
  onFileReceived();
}

// send next pending interest, returns success-indicator
void FileDownloader::sendNextInterests(int max){
  int expressed_interests = 0;
  for(uint index = 0; index < buffer.size(); index++)
  {
    if(buffer[index].state == chunk_state::pending)
    {
      expressInterest(index);
      expressed_interests++;
      buffer[index].state = chunk_state::requested;
      if(expressed_interests >= max)
      {
          const std::time_t ctt = std::time(0);
          cout << asctime(localtime(&ctt)) << "sending batch" << endl;
        return;
      }
    }
  }
  const std::time_t ctt = std::time(0);
  cout << asctime(localtime(&ctt)) << "sending batch" << endl;

  m_face.processEvents(); // blocking?
  //m_face.processEvents(time::milliseconds(-1),false); // just process events, not blocking here
}

void FileDownloader::expressInterest(int seq_nr)
{
   Interest interest(Name(file_name).appendSequenceNumber(seq_nr));  // e.g. "/example/testApp/randomData"
  // appendSequenceNumber
  interest.setInterestLifetime(time::milliseconds(this->interest_lifetime));   // time::milliseconds(1000)
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
                         bind(&FileDownloader::onData, this,  _1, _2),
                         bind(&FileDownloader::onTimeout, this, _1));

  cout << "Expressing interest #" << seq_nr << " " << interest << endl;
}


// private methods
// react to the reception of a reply from a Producer
void FileDownloader::onData(const Interest& interest, const Data& data)
{
  if(this->state == process_state::cancelled){
      cout << "onData after cancel" << endl;
      return;
  }

  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  cout << "data-packet #" << seq_nr <<  " received: " << endl;

  const Block& block = data.getContent();

  if(this->state == process_state::startup)
  {
     // first data-packet arrived, allocate space
    this->finalBockId = boost::lexical_cast<int>(data.getFinalBlockId().toUri());
    int buffer_size = this->finalBockId + 1;

    //cout << "init buffer_size: " << buffer_size << endl;
    this->buffer.resize(0); // allow for smaller buffersizes
    this->buffer.resize(buffer_size,chunk());
    this->state = process_state::running;
  }

  // Debug-output:
  //std::cout.write((const char*)block.value(),block.value_size());
  //cout << endl;

  // store received data in buffer
  shared_ptr<itec::Buffer> b(new itec::Buffer((char*)block.value(), block.value_size()));
  buffer[seq_nr].data = b;
  buffer[seq_nr].state = chunk_state::received;
}

// check if actually all parts have been downloaded
bool FileDownloader::allChunksReceived(){
    for(std::vector<chunk>::iterator it = buffer.begin(); it != buffer.end(); ++it) {
        if((*it).state != chunk_state::received) return false;
    }
    return true;
}

// react on the request / Interest timing out
void FileDownloader::onTimeout(const Interest& interest)
{
  if(this->state == process_state::cancelled){
      cout << "onTimeout after cancel" << endl;
      return;
  }
  cout << "Timeout " << interest << endl;
  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  buffer[seq_nr].state = chunk_state::pending; // reset state to pending -> queue for re-request
}

// cancel downloading, returned file will be empty
void FileDownloader::cancel()
{
  this->state = process_state::cancelled;
}

void FileDownloader::onFileReceived ()
{
  fprintf(stderr, "File received!\n");
}
