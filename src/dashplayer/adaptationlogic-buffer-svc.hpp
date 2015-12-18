#ifndef ADAPTATIONLOGICBUFFERSVC_HPP
#define ADAPTATIONLOGICBUFFERSVC_HPP

#include "adaptationlogic.hpp"
#include <cmath>

#define BUFFER_MIN_SIZE 16 // in seconds
#define BUFFER_ALPHA 8 // in seconds

namespace player
{
class SVCBufferAdaptationLogic : public AdaptationLogic
{
public:

  SVCBufferAdaptationLogic(DashPlayer* dashplayer) : AdaptationLogic (dashplayer)
  {
    alpha = BUFFER_ALPHA;
    gamma = BUFFER_MIN_SIZE;
  }

  virtual std::string GetName() const
  {
    return "player::SVCBufferAdaptationLogic";
  }

  static boost::shared_ptr<AdaptationLogic> Create(DashPlayer* mPlayer)
  {
    return boost::shared_ptr<AdaptationLogic>(new SVCBufferAdaptationLogic(mPlayer));
  }

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation *> availableRepresentations);

  virtual ISegmentURL* GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation,  bool* hasDownloadedAllSegments);

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);


protected:

  void orderRepresentationsByDepIds();
  unsigned int desired_buffer_size(int i, int i_curr);
  unsigned int getNextNeededSegmentNumber(int layer);

  static SVCBufferAdaptationLogic _staticLogic;

  SVCBufferAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferAdaptationLogic);
  }

  std::map<int /*level*/, IRepresentation*> m_orderdByDepIdReps;

  double alpha;
  int gamma; //BUFFER_MIN_SIZE

  double segment_duration;

  enum AdaptationPhase
  {
    Steady = 0,
    Growing = 1,
    Upswitching = 2
  };

  AdaptationPhase lastPhase;
  AdaptationPhase allowedPhase;
};
}
#endif // ADAPTATIONLOGICSVCBUFFERBASED_HPP
