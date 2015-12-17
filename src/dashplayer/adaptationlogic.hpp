#ifndef DASH_ADAPTATION_LOGIC
#define DASH_ADAPTATION_LOGIC

#include <iostream>
#include <string>
#include <map>

#include "libdash/libdash.h"

using namespace dash::mpd;


namespace ndn
{
class AdaptationLogic
{
public:
  AdaptationLogic();
  ~AdaptationLogic();

  virtual std::string GetName() const
  {
    return "dash::player::AdaptationLogic";
  }

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation *> availableRepresentations);

  virtual ISegmentURL* GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation, bool* hasDownloadedAllSegments);
  unsigned int getTotalSegments();

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);

  IRepresentation* GetLowestRepresentation();

protected:

  std::map<std::string, IRepresentation*> m_availableRepresentations;
};

}
#endif // DASH_ADAPTATION_LOGIC
