#ifndef FILEDOWNLOADER_HPP
#define FILEDOWNLOADER_HPP

// for communication with NFD
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

// string ops
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/shared_ptr.hpp"

#include <string>
#include <stdio.h>

#include "../utils/buffer.hpp"


using namespace std;
using namespace ndn;

namespace player {
class FileDownloader
{
public:
  FileDownloader(int interest_lifetime);

  shared_ptr<itec::Buffer> getFile(string name);
  void cancel();
protected:
  enum class process_state {startup, running, cancelled};
  enum class chunk_state {pending, requested, received};
  struct chunk {
      shared_ptr<itec::Buffer> data;
      chunk_state state = chunk_state::pending;
  };

  void onData(const Interest& interest, const Data& data);
  void onTimeout(const Interest& interest);
  void onFileReceived();
  void download();
  void sendNextInterests(int max);
  void expressInterest(int seq_nr);
  bool allChunksReceived();

  Face m_face;
  int interest_lifetime;
  vector<chunk> buffer;
  int finalBockId;
  string file_name;
  int bitrate;
  process_state state;

  shared_ptr<itec::Buffer> file;
};
}
#endif // FILEDOWNLOADER_HPP
