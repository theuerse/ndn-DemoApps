// ndn-File-Consumer
// requests files (in chunks) from a Producer

#ifndef DASHPLAYER_H
#define DASHPLAYER_H

// using boost for file-system handling / cmd_options
#include "filedownloader.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

// file ops
#include <iostream>
#include <fstream>

#include <libdash/libdash.h>
#include <libdash/IMPD.h>

using namespace std;
using namespace dash::mpd;
using namespace boost::program_options;
using namespace ndn;

namespace player {
class DashPlayer
{
    public:
        DashPlayer(string MPD, int interest_lifetime);
        virtual ~DashPlayer();
        void startStreaming();

    private:
        void writeFileToDisk(shared_ptr<itec::Buffer> buf, string file_path);
        bool parseMPD(std::string mpd_path);

        int interest_lifetime;
        string mpd_url;
        std::string base_url;
        bool streaming_active;

        dash::mpd::IMPD *mpd;
        std::map<std::string, IRepresentation*> availableRepresentations;

        FileDownloader downloader;

        void scheduleDownloadNextSegment();
        void schedulePlayback();
};
}   // end namespace ndn

#endif // DashPlayer_H
