#ifndef FILEDOWNLOADER_HPP
#define FILEDOWNLOADER_HPP

// for communication with NFD
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

// string ops
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/asio/deadline_timer.hpp"
#include <boost/algorithm/string/predicate.hpp>

#include <string>
#include <stdio.h>

#include "../utils/buffer.hpp"


using namespace std;
using namespace ndn;

namespace player {
class FileDownloader : boost::noncopyable
{
public:
  FileDownloader(int interest_lifetime);

  shared_ptr<itec::Buffer> getFile(string name, double bitrate);
  void cancel();
protected:
  enum class process_state {running, cancelled, finished};
  enum class chunk_state {unavailable, requested, received};
  struct chunk {
      shared_ptr<itec::Buffer> data;
      chunk_state state = chunk_state::unavailable;
  };

  void onData(const Interest& interest, const Data& data);
  void onTimeout(const Interest& interest);
  void onFileReceived();
  void sendNextInterest(boost::asio::deadline_timer *timer);
  void expressInterest(int seq_nr);
  bool allChunksReceived();

  int interest_lifetime;
  vector<chunk> buffer;
  int finalBockId;
  string file_name;
  int requestRate;
  process_state state;

  shared_ptr<itec::Buffer> file;

  boost::asio::io_service m_ioService;
  Face m_face;
};
}
#endif // FILEDOWNLOADER_HPP
