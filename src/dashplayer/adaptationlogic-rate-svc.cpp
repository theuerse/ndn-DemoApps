#include "adaptationlogic-rate-svc.hpp"

#include "dashplayer.hpp"

namespace player
{
  ENSURE_ADAPTATION_LOGIC_INITIALIZED(SVCRateBasedAdaptationLogic)

  SVCRateBasedAdaptationLogic::SVCRateBasedAdaptationLogic(DashPlayer* dashPlayer) : AdaptationLogic (dashPlayer)
  {
    ema_download_bitrate = dashplayer->GetLastDownloadBitRate();
    curSegmentNumber = 0;
    bufMinLevel = RateBasedMinBufferLevel;
  }

  ISegmentURL* SVCRateBasedAdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool *hasDownloadedAllSegments)
  {
    //update EMA of download bitrate
    updateEMA ();

    //fprintf(stderr, "bufferLevel = %f\n", dashplayer->GetBufferPercentage());
    //fprintf(stderr, "ema_download_bitrate = %f\n",ema_download_bitrate);

    if(!hasMinBufferLevel () || //check if we have the minbuffer level
       (repsForCurSegment.empty () && dashplayer->GetBufferLevel() < bufMinLevel + GrowingBuffer)) // check if stack is empty if we should download a rep based on EMA bitrate
    {
      //clear download stack
      while(!repsForCurSegment.empty ())
        repsForCurSegment.pop ();

      //if not, check if there are base layer segments left
      if(curSegmentNumber < getTotalSegments());
      {
        //if so, we only download the base layer
        repsForCurSegment.push (GetLowestRepresentation());
      }
    }
    else if (repsForCurSegment.empty ()) // we need new stack for downloading
    {
      //evaluate the max_rep we would like to take
      double max_allowed_bitrate = ema_download_bitrate; //
      if(dashplayer->GetBufferPercentage() > 0.5) // we dont need to "lower" bitrate when buffer is nearly empty as we have a MinBufferCheck before
      {
        max_allowed_bitrate *= 1.2;
      }
      else if(dashplayer->GetBufferPercentage() > 0.75)
      {
        max_allowed_bitrate *= 1.3;
      }
      //fprintf(stderr, "max_allowed_bitrate = %f\n\n",max_allowed_bitrate);

      //const IRepresentation* useRep = GetLowestRepresentation();
      int layerForRep = 0;

      double highest_bitrate = 0.0;
      for (std::map<int /*level/layer*/, IRepresentation*>::iterator it = m_orderdByDepIdReps.begin (); it != m_orderdByDepIdReps.end (); ++it)
      {
        const IRepresentation* rep = it->second;
        //std::cerr << "Rep=" << keyValue.first << " has bitrate " << rep->GetBandwidth() << std::endl;
        if (rep->GetBandwidth() < max_allowed_bitrate)
        {
          if (rep->GetBandwidth() > highest_bitrate)
          {
            //useRep = rep;
            layerForRep = it->first;
            highest_bitrate = rep->GetBandwidth();
          }
        }
      }

      for(int i = layerForRep; i >= 0; i--)
        repsForCurSegment.push (m_orderdByDepIdReps[i]);
    }

    //download what is on the top of stack
    *usedRepresentation = repsForCurSegment.top ();
    repsForCurSegment.pop ();

    *requested_segment_number = curSegmentNumber;

    if(curSegmentNumber < getTotalSegments ())
      *hasDownloadedAllSegments = false;
    else
    {
      *hasDownloadedAllSegments = true;
      return NULL; // everything downloaded
    }

    //check if stack is empty now
    if(repsForCurSegment.empty ())
      curSegmentNumber++; // then increase segment number

    return (*usedRepresentation)->GetSegmentList()->GetSegmentURLs().at(*requested_segment_number);
  }

  void SVCRateBasedAdaptationLogic::updateEMA ()
  {
    ema_download_bitrate = dashplayer->GetLastDownloadBitRate() * RateBasedEMA_W + (1-RateBasedEMA_W) * ema_download_bitrate;
  }

  bool SVCRateBasedAdaptationLogic::hasMinBufferLevel(const dash::mpd::IRepresentation*)
  {
    return hasMinBufferLevel();
  }

  bool SVCRateBasedAdaptationLogic::hasMinBufferLevel()
  {
    if(dashplayer->GetBufferLevel() > bufMinLevel)
      return true;

    return false;
  }

  void SVCRateBasedAdaptationLogic::SetAvailableRepresentations(std::map<std::string, IRepresentation*> availableRepresentations)
  {
    AdaptationLogic::SetAvailableRepresentations (availableRepresentations);
    orderRepresentationsByDepIds();

    //calc typical segment duration (we assume all reps have the same duration..)
    segment_duration = (double) m_orderdByDepIdReps.begin()->second->GetSegmentList()->GetDuration();
    segment_duration /= (double) m_orderdByDepIdReps.begin()->second->GetSegmentList()->GetTimescale ();
  }

  //this functions classifies reps into layers depending on the DepIds.
  void SVCRateBasedAdaptationLogic::orderRepresentationsByDepIds()
  {
    //fprintf(stderr, "SVCRateBasedAdaptationLogic::orderRepresentationsByDepIds()\n");

    std::map<std::string/*RepId*/, IRepresentation*> reps = m_availableRepresentations;
    std::map<std::string/*RepId*/, IRepresentation*> selectedReps;

    //fprintf(stderr, "m_availableRepresentations.size()=%d\n",m_availableRepresentations->size ());
    //fprintf(stderr, "reps.size()=%d\n",reps.size ());

    m_orderdByDepIdReps.clear ();
    int level = 0;

    while(reps.size () > 0)
    {
      //find rep with the lowest depIds.
      std::map<std::string/*RepId*/, IRepresentation*>::iterator lowest = reps.begin ();
      for(std::map<std::string/*RepId*/, IRepresentation*>::iterator it = reps.begin (); it!=reps.end (); ++it)
      {
        if(lowest->second->GetDependencyId().size() < lowest->second->GetDependencyId().size())
          lowest = it;
      }
      std::vector<std::string> v = lowest->second->GetDependencyId();
      // check if all required DepIds are in m_orderdByDepIdReps
      for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); it++ )
      {
        if(selectedReps.find(*it) == selectedReps.end ())
        {
          fprintf(stderr, "Error in ordering Representations based on DepIds\n");
          return;
        }
      }
      m_orderdByDepIdReps[level++] = lowest->second;
      selectedReps[lowest->first] = lowest->second;
      reps.erase (lowest);
    }
    //for(std::map<int /*level*/, IRepresentation*>::iterator it = m_orderdByDepIdReps.begin (); it!=m_orderdByDepIdReps.end (); it++)
    //  fprintf(stderr, "layer[%d]=%s\n",it->first, it->second->GetId().c_str());
  }

}
