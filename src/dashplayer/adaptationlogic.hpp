#ifndef DASH_ADAPTATION_LOGIC
#define DASH_ADAPTATION_LOGIC

#define ENSURE_ADAPTATION_LOGIC_REGISTERED(x) player::AdaptationLogicFactory::RegisterAdaptationLogic(x::GetName(), &(x::Create));
#define ENSURE_ADAPTATION_LOGIC_INITIALIZED(x) x x::_staticLogic;
#include "adaptation-logic-factory.hpp"

#include <iostream>
#include <string>
#include <map>

#include "libdash/libdash.h"

using namespace dash::mpd;

namespace player
{

class AdaptationLogic
{
public:
  AdaptationLogic(DashPlayer* dashplayer);
  ~AdaptationLogic();

  static boost::shared_ptr<AdaptationLogic> Create(DashPlayer* mPlayer)
  {
    return boost::shared_ptr<AdaptationLogic>(new AdaptationLogic(mPlayer));
  }

  virtual std::string GetName() const
  {
    return "player::AdaptationLogic";
  }

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation *> availableRepresentations);

  virtual ISegmentURL* GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation, bool* hasDownloadedAllSegments);
  unsigned int getTotalSegments();

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);

  IRepresentation* GetLowestRepresentation();

protected:

  static AdaptationLogic _staticLogic;
  DashPlayer *dashplayer;
  std::map<std::string, IRepresentation*> m_availableRepresentations;

  AdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(AdaptationLogic);
  }
};

}
#endif // DASH_ADAPTATION_LOGIC
