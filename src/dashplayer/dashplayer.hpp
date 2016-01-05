// ndn-File-Consumer
// requests files (in chunks) from a Producer

#ifndef DASHPLAYER_H
#define DASHPLAYER_H

// using boost for file-system handling / cmd_options
#include "filedownloader.hpp"

#include <ndn-cxx/util/scheduler.hpp>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// file ops
#include <iostream>
#include <fstream>

#include <libdash/libdash.h>
#include <libdash/IMPD.h>

#include "adaptationlogic.hpp"
#include "adaptationlogic-buffer-svc.hpp"

#include "multimediabuffer.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <iomanip>
#include <ctime>
#include <iostream>

using namespace std;
using namespace dash::mpd;
using namespace boost::program_options;
using namespace ndn;

namespace player {
class DashPlayer
{
    public:
        DashPlayer(string MPD, string adaptionlogic_name, int interest_lifetime, int run_time, double request_rate, std::string logFileName);
        virtual ~DashPlayer();
        void startStreaming();

        double GetBufferLevel(std::string repId = std::string("NULL"));
        double GetBufferPercentage(std::string repId = std::string("NULL"));
        double GetLastDownloadBitRate();

        unsigned int nextSegmentNrToConsume();
        unsigned int getHighestBufferedSegmentNr(std::string repId);

    private:
        void writeFileToDisk(shared_ptr<itec::Buffer> buf, string file_path);
        bool parseMPD(std::string mpd_path);

        int interest_lifetime;
        string mpd_url;
        std::string base_url;
        bool streaming_active;

        dash::mpd::IMPD *mpd;
        std::map<std::string, IRepresentation*> availableRepresentations;

        boost::posix_time::ptime stallStartTime;
        bool isStalling;

        boost::shared_ptr<FileDownloader> downloader;

        string adaptionlogic_name;

        const dash::mpd::IRepresentation* requestedRepresentation;
        unsigned int requestedSegmentNr;
        dash::mpd::ISegmentURL* requestedSegmentURL;

        void scheduleDownloadNextSegment();
        void schedulePlayback();

        boost::shared_ptr<MultimediaBuffer> mbuffer;
        boost::shared_ptr<AdaptationLogic> alogic;
        int max_buffered_seconds;

        unsigned int totalConsumedSegments;
        bool hasDownloadedAllSegments;

        ofstream logFile;

        void logFilePrintHeader();
        void logSegmentConsume(player::MultimediaBuffer::BufferRepresentationEntry &entry, boost::posix_time::time_duration& stallingTime);

        std::string myId;
        void setMyId();

        void stopPlayer();
        int maxRuntimeSeconds;

        bool maxRunTimeReached;

        double requestRate;
        double lastDWRate;
};
}   // end namespace ndn

#endif // DashPlayer_H
