#ifndef ADAPTATIONLOGICRATESVC_HPP
#define ADAPTATIONLOGICRATESVC_HPP

#include "adaptationlogic.hpp"
#include <stack>

#define RateBasedMinBufferLevel 8.0 //Seconds
#define GrowingBuffer 4.0 //Seconds
#define RateBasedEMA_W 0.3

namespace player
{

class SVCRateBasedAdaptationLogic : public AdaptationLogic
{
public:
  SVCRateBasedAdaptationLogic(DashPlayer* dashPlayer);

  virtual std::string GetName() const
  {
    return "rate";
  }

  static boost::shared_ptr<AdaptationLogic> Create(DashPlayer* mPlayer)
  {
    return boost::shared_ptr<AdaptationLogic>(new SVCRateBasedAdaptationLogic(mPlayer));
  }

  virtual ISegmentURL* GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool* hasDownloadedAllSegments);

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation *> availableRepresentations);

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);


protected:
  static SVCRateBasedAdaptationLogic _staticLogic;

  SVCRateBasedAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCRateBasedAdaptationLogic);
  }

  void updateEMA();

  void orderRepresentationsByDepIds();
  bool hasMinBufferLevel();

  std::map<int /*level/layer*/, IRepresentation*> m_orderdByDepIdReps;

  //unsigned int getNextNeededSegmentNumber(int layer);
  unsigned int curSegmentNumber;

  double segment_duration;
  double bufMinLevel;

  double ema_download_bitrate; /* moving average*/

  std::stack<dash::mpd::IRepresentation *> repsForCurSegment;
};
}

#endif // ADAPTATIONLOGICRATESVC_HPP
