// ndn-File-Consumer
// requests files (in chunks) from a Producer

#ifndef FILECONSUMER_H
#define FILECONSUMER_H

// for communication with NFD
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

// using boost for file-system handling / cmd_options
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

// string ops
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

// file ops
#include<fstream>

using namespace std;
using namespace boost::program_options;

namespace ndn {
class FileConsumer : noncopyable
{
    public:
        FileConsumer(int interest_lifetime);
        void getFile(string name);
        virtual ~FileConsumer();
    protected:
    private:
        void onData(const Interest& interest, const Data& data);
        void onTimeout(const Interest& interest);
        void sendInterest(int seq_nr);
        void flushBufferToFile(string path);

        Face m_face;
        int interest_lifetime;
        vector<string> buffer;
        int finalBockId;
        string file_name;
};
}   // end namespace ndn

#endif // FILECONSUMER_H
