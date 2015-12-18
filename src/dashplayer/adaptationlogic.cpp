#include "adaptationlogic.hpp"

#include "dashplayer.hpp"

namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(AdaptationLogic)

AdaptationLogic::AdaptationLogic(DashPlayer* dashplayer)
{
  this->dashplayer = dashplayer;
}

AdaptationLogic::~AdaptationLogic()
{
}


void
AdaptationLogic::SetAvailableRepresentations(std::map<std::string, IRepresentation*> availableRepresentations)
{
  this->m_availableRepresentations = availableRepresentations;
}

ISegmentURL* AdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool *hasDownloadedAllSegments)
{
  *requested_segment_number = 0;
  *usedRepresentation = NULL;
  *hasDownloadedAllSegments = false;
  return NULL;
}


IRepresentation* AdaptationLogic::GetLowestRepresentation()
{
  return m_availableRepresentations.begin()->second;
}

// we assume that in all representations the same amount of segments exists..
unsigned int AdaptationLogic::getTotalSegments()
{
  return m_availableRepresentations.begin()->second->GetSegmentList ()->GetSegmentURLs().size();
}

bool AdaptationLogic::hasMinBufferLevel(const dash::mpd::IRepresentation* rep)
{
  return true;
}
}
