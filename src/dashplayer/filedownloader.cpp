#include "filedownloader.hpp"

using namespace player;

FileDownloader::FileDownloader(int interest_lifetime)
{
  this->interest_lifetime = interest_lifetime;
}

shared_ptr<itec::Buffer> FileDownloader::getFile(string name){
  this->buffer.clear();
  this->file_name = name;
  sendInterest(0); // send first interest-packet
  // processEvents will block until the requested data received or timeout occurs
  m_face.processEvents();

  return file;
}

void FileDownloader::sendInterest(int seq_nr)
{
   Interest interest(Name(file_name).appendSequenceNumber(seq_nr));  // e.g. "/example/testApp/randomData"
  // appendSequenceNumber
  interest.setInterestLifetime(time::milliseconds(this->interest_lifetime));   // time::milliseconds(1000)
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
                         bind(&FileDownloader::onData, this,  _1, _2),
                         bind(&FileDownloader::onTimeout, this, _1));

  //cout << "Sending " << interest << endl;
}


// private methods
// react to the reception of a reply from a Producer
void FileDownloader::onData(const Interest& interest, const Data& data)
{
  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  //cout << "data-packet #" << seq_nr <<  " received: " << endl;

  const Block& block = data.getContent();

  if(this->buffer.empty())
  {
     // first data-packet arrived, allocate space
    this->finalBockId = boost::lexical_cast<int>(data.getFinalBlockId().toUri());
    int buffer_size = this->finalBockId + 1;

    //cout << "init buffer_size: " << buffer_size << endl;
    this->buffer.resize(buffer_size,chunk());
  }

  // Debug-output:
  //std::cout.write((const char*)block.value(),block.value_size());
  //cout << endl;

  // store received data in buffer
  shared_ptr<itec::Buffer> b(new itec::Buffer((char*)block.value(), block.value_size()));
  buffer[seq_nr].data = b;
  buffer[seq_nr].state = chunk_state::received;

  // request next one
  if(seq_nr < this->finalBockId) //TODO
  {
      sendInterest(seq_nr + 1);
  }

  if(allChunksReceived())
  {
    file = shared_ptr<itec::Buffer>(new itec::Buffer());

    for(vector<chunk>::iterator it = buffer.begin (); it != buffer.end (); it++)
    {
       file->append((*it).data->getData(),(*it).data->getSize());
    }
    onFileReceived();
  }
}

bool FileDownloader::allChunksReceived(){
    for(std::vector<chunk>::iterator it = buffer.begin(); it != buffer.end(); ++it) {
        if((*it).state != chunk_state::received) return false;
    }
    return true;
}

// react on the request / Interest timing out
void FileDownloader::onTimeout(const Interest& interest)
{
  cout << "Timeout " << interest << endl;
  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  buffer[seq_nr].state = chunk_state::pending; // reset state to pending -> queue for re-request
}

void FileDownloader::onFileReceived ()
{
  fprintf(stderr, "File received!\n");
}
