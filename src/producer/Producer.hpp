// ndn-File-Producer
// works like a simple http-server

#ifndef PRODUCER_H
#define PRODUCER_H

// for communication with NFD
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

// file-stream
#include <fstream>

#include <math.h>

// using boost for file-system handling / cmd_options
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/shared_ptr.hpp"
#include "../utils/buffer.hpp"

using namespace std;
using namespace boost::program_options;

namespace ndn {
class Producer : noncopyable
{
    public:
        Producer(string prefix, string document_root, int data_size, int freshness_seconds);
        void run();
        virtual ~Producer();
    protected:
    private:
        struct file_chunk_t {
            bool success;
            shared_ptr<itec::Buffer> buffer;
            int final_block_id;
        };

        string generateContent(const int length);
        void onInterest(const InterestFilter& filter, const Interest& interest);
        void onRegisterFailed(const Name& prefix, const string& reason);
        file_chunk_t getFileContent(const Interest& interest);

        Face m_face;
        KeyChain m_keyChain;
        string prefix;
        string document_root;
        int data_size;
        int freshness_seconds;
};
}   // end namespace ndn

#endif // PRODUCER_H
