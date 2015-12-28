#include "filedownloader.hpp"

using namespace player;

#define DATA_SIZE 4096.0 //byte

FileDownloader::FileDownloader(int interest_lifetime) : m_face(m_ioService)
{
  this->interest_lifetime = interest_lifetime;
}

/**
 * @brief FileDownloader::getFile
 * @param name and prefix of file to be fetched
 * @param bitrate [kbits]
 * @return a itec::Buffer containing the file
 */
shared_ptr<itec::Buffer> FileDownloader::getFile(string name, double bitrate){
  this->buffer.clear();
  this->buffer.resize (20,chunk());
  this->state = process_state::running;
  this->requestRate = 1000000/((bitrate*1000)/(DATA_SIZE*8.0)); //microseconds
  this->file = shared_ptr<itec::Buffer>(new itec::Buffer());
  this->file_name = name;

  boost::asio::deadline_timer timer(m_ioService, boost::posix_time::microseconds(requestRate));
  sendNextInterest(&timer); // starts the download

  // processEvents will block until the requested data received or timeout occurs#

  boost::posix_time::ptime startTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  m_face.processEvents();
  boost::posix_time::ptime endTime = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  boost::posix_time::time_duration downloadDuration = endTime-startTime;

  double dwrate = 0.0;
  for(size_t i = 0; i < buffer.size (); i++)
  {
    if(buffer.at(i).state == chunk_state::received)
     dwrate+=buffer.at(i).data->getSize();
  }
  if(downloadDuration.total_milliseconds () > 0)
  {
    dwrate = (dwrate * 8.0) / downloadDuration.total_milliseconds (); //bits/ms
    dwrate *= 1000; //bits/s
  }
  else
    dwrate = 0.0;

  fprintf(stderr, "dwrate = %f\n", dwrate);

  return file;
}

// send next pending interest, returns success-indicator
void FileDownloader::sendNextInterest(boost::asio::deadline_timer* timer)
{

  if(allChunksReceived ())// all chunks / file has been downloaded
  {
    //TODO Cleanup;
    onFileReceived();
    return;
  }

  // check if caller cancelled
  if(this->state == process_state::cancelled)
  {
    m_face.shutdown(); // dangerous?
    return;
  }

  for(uint index = 0; index < buffer.size(); index++)
  {
    if(buffer[index].state == chunk_state::unavailable)
    {
      expressInterest(index);
      buffer[index].state = chunk_state::requested;

      timer->expires_at (timer->expires_at ()+ boost::posix_time::microseconds(requestRate));
      timer->async_wait(bind(&FileDownloader::sendNextInterest, this, timer));
      return;
    }
  }
  timer->expires_at (timer->expires_at ()+ boost::posix_time::microseconds(requestRate));
  timer->async_wait(bind(&FileDownloader::sendNextInterest, this, timer));
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

  //cout << "Expressing interest #" << seq_nr << " " << interest << endl;
}


// private methods
// react to the reception of a reply from a Producer
void FileDownloader::onData(const Interest& interest, const Data& data)
{
  if(!boost::starts_with(data.getName().toUri (), file_name))
    return; // data packet for previous requsted file(s)

  if(state == process_state::cancelled || state == process_state::finished)
  {
    //cout << "onData after cancel" << endl;
    return;
  }

  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  //cout << "data-packet #" << seq_nr <<  " received: " << endl;

  const Block& block = data.getContent();

  // first data-packet arrived, allocate space
  this->finalBockId = boost::lexical_cast<int>(data.getFinalBlockId().toUri());

  if(! ((int)buffer.size () == finalBockId+1)) // we assume producer always transmitts a valid final block id
  {
    this->buffer.resize(finalBockId+1,chunk());
  }

  // store received data in buffer
  shared_ptr<itec::Buffer> b(new itec::Buffer((char*)block.value(), block.value_size()));
  buffer[seq_nr].data = b;
  buffer[seq_nr].state = chunk_state::received;
}

// check if actually all parts have been downloaded
bool FileDownloader::allChunksReceived(){
    for(std::vector<chunk>::iterator it = buffer.begin(); it != buffer.end(); ++it)
    {
        if((*it).state != chunk_state::received)
          return false;
    }
    return true;
}

// react on the request / Interest timing out
void FileDownloader::onTimeout(const Interest& interest)
{
  if(state == process_state::cancelled || state == process_state::finished)
  {
      //cout << "onTimeout after cancel" << endl;
      return;
  }
  //cout << "Timeout " << interest << endl;
  // get sequence number
  int seq_nr = interest.getName().at(-1).toSequenceNumber();
  buffer[seq_nr].state = chunk_state::unavailable; // reset state to pending -> queue for re-request
}

// cancel downloading, returned file will be empty
void FileDownloader::cancel()
{
  this->state = process_state::cancelled;
}

void FileDownloader::onFileReceived ()
{
  for(vector<chunk>::iterator it = buffer.begin (); it != buffer.end (); it++)
  {
    file->append((*it).data->getData(),(*it).data->getSize());
  }

  state = process_state::finished;
  //fprintf(stderr, "File received!\n");
}
