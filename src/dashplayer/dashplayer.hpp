// ndn-File-Consumer
// requests files (in chunks) from a Producer

#ifndef DASHPLAYER_H
#define DASHPLAYER_H

// using boost for file-system handling / cmd_options
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

// file ops
#include <iostream>
#include <fstream>

#include <libdash/libdash.h>

#include "filedownloader.hpp"

using namespace std;
using namespace boost::program_options;

namespace ndn {
class DashPlayer
{
    public:
        DashPlayer(string MPD, int interest_lifetime);
        virtual ~DashPlayer();
        void startStreaming();

    private:
        void writeFileToDisk(shared_ptr<itec::Buffer> buf, string file_path);

        int interest_lifetime;
        string MPD;
        bool streaming_active;

        FileDownloader downloader;
};
}   // end namespace ndn

#endif // DashPlayer_H
