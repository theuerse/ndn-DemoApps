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

namespace ndn {
class FileDownloader
{
public:
  FileDownloader();

  shared_ptr<itec::Buffer> getFile(string name);

protected:
  void onData(const Interest& interest, const Data& data);
  void onTimeout(const Interest& interest);
  void onFileReceived();
  void sendInterest(int seq_nr);

  Face m_face;
  int interest_lifetime;
  vector<shared_ptr<itec::Buffer> > buffer;
  int finalBockId;
  string file_name;

  shared_ptr<itec::Buffer> file;
};
}
#endif // FILEDOWNLOADER_HPP
