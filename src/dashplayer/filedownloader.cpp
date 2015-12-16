#include "filedownloader.hpp"

ndn::FileDownloader::FileDownloader()
{
}

shared_ptr<itec::Buffer> ndn::FileDownloader::getFile(string name){
  this->buffer.clear();
  this->file_name = name;
  sendInterest(0); // send first interest-packet
  // processEvents will block until the requested data received or timeout occurs
  m_face.processEvents();

  return file;
}

void ndn::FileDownloader::sendInterest(int seq_nr)
{
   Interest interest(Name(this->file_name).appendSequenceNumber(seq_nr));  // e.g. "/example/testApp/randomData"
  // appendSequenceNumber
  interest.setInterestLifetime(time::milliseconds(this->interest_lifetime));   // time::milliseconds(1000)
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
                         bind(&ndn::FileDownloader::onData, this,  _1, _2),
                         bind(&ndn::FileDownloader::onTimeout, this, _1));

  cout << "Sending " << interest << endl;
}


// private methods
// react to the reception of a reply from a Producer
void ndn::FileDownloader::onData(const Interest& interest, const Data& data)
{
  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  cout << "data-packet #" << seq_nr <<  " received: " << endl;

  const Block& block = data.getContent();

  if(this->buffer.empty())
  {
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
  shared_ptr<itec::Buffer> b(new itec::Buffer((char*)block.value(), block.value_size()));
  buffer.insert(buffer.begin() + seq_nr, b);

  // request next one
  if(seq_nr < this->finalBockId) //TODO
  {
      sendInterest(seq_nr + 1);
  }
  else
  {
    cout << "got all " << seq_nr + 1 << " parts" << endl;

    file = shared_ptr<itec::Buffer>(new itec::Buffer());

    for(vector<shared_ptr<itec::Buffer> >::iterator it = buffer.begin (); it != buffer.end (); it++)
    {
       file->append((*it)->getData(),(*it)->getSize());
    }
    onFileReceived();
  }
}

// react on the request / Interest timing out
void ndn::FileDownloader::onTimeout(const Interest& interest)
{
  cout << "Timeout " << interest << endl;
}

void ndn::FileDownloader::onFileReceived ()
{
  fprintf(stderr, "File received!\n");
}
