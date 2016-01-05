#include "multimediabuffer.hpp"

using namespace dash;
using namespace player;

MultimediaBuffer::MultimediaBuffer(unsigned int maxBufferedSeconds)
{
  this->maxBufferedSeconds= (double) maxBufferedSeconds;

  toBufferSegmentNumber = 0;
  toConsumeSegmentNumber = 0;
}

MultimediaBuffer::~MultimediaBuffer()
{
}

bool MultimediaBuffer::addToBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //check if we receive a segment with a too large number
  if(toBufferSegmentNumber < segmentNumber)
    return false;

  //fprintf(stderr, "toBufferSegmentNumber=%d < segmentNumber=%d\n",toBufferSegmentNumber,segmentNumber);

  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  // Check if segment has depIds
  if(usedRepresentation->GetDependencyId ().size() > 0)
  {
    // if so find the correct map
    MBuffer::iterator it = buff.find (segmentNumber);
    if(it == buff.end ())
      return false;

    BufferRepresentationEntryMap map = it->second;

    mtx.lock ();
    for(std::vector<std::string>::const_iterator k = usedRepresentation->GetDependencyId ().begin ();
        k !=  usedRepresentation->GetDependencyId ().end (); k++)
    {
      //depId not found we can not add this layer
      if(map.find ((*k)) == map.end ())
      {
        //fprintf(stderr, "Could not find '%s' in map\n", (*k).c_str());
        mtx.unlock ();
        return false;
      }
    }
  }
  else
  {
    //check if segment with layer == 0 fits in buffer
    if(isFull(usedRepresentation->GetId (),duration))
    {
      mtx.unlock ();
      return false;
    }
  }

  // Add segment to buffer
  //fprintf(stderr, "Inserted something for Segment %d in Buffer\n", segmentNumber);

  MultimediaBuffer::BufferRepresentationEntry entry;
  entry.repId = usedRepresentation->GetId ();
  entry.segmentDuration = duration;
  entry.segmentNumber = segmentNumber;
  entry.depIds = usedRepresentation->GetDependencyId ();
  entry.bitrate_bit_s = usedRepresentation->GetBandwidth ();

  buff[segmentNumber][usedRepresentation->GetId ()] = entry;
  toBufferSegmentNumber++;
  mtx.unlock ();
  return true;
}

bool MultimediaBuffer::enoughSpaceInBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  if(isFull(usedRepresentation->GetId (),duration))
    return false;

  return true;
}

bool MultimediaBuffer::isFull(std::string repId, double additional_seconds)
{
  if(maxBufferedSeconds < additional_seconds+getBufferedSeconds(repId))
    return true;
  return false;
}

bool MultimediaBuffer::isEmpty()
{
  if(getBufferedSeconds() <= 0.0)
    return true;
  return false;
}


double MultimediaBuffer::getBufferedSeconds()
{
  double bufferSize = 0.0;

  for(MBuffer::iterator it = buff.begin (); it != buff.end (); ++it)
    bufferSize += it->second.begin()->second.segmentDuration;

  //fprintf(stderr, "BufferSize for lowestRep = %f\n", bufferSize);
  return bufferSize;
}

double MultimediaBuffer::getBufferedSeconds(std::string repId)
{
  double bufferSize = 0.0;
  for(MBuffer::iterator it = buff.begin (); it != buff.end (); ++it)
  {
    BufferRepresentationEntryMap::iterator k = it->second.find(repId);
    if(k != it->second.end())
    {
      bufferSize += k->second.segmentDuration;
    }
  }
  //fprintf(stderr, "BufferSize for rep[%s] = %f\n", repId.c_str (),bufferSize);
  return bufferSize;
}

unsigned int MultimediaBuffer::getHighestBufferedSegmentNr(std::string repId)
{
  //std::map should be orderd based on operator<, so iterate from the back
  for(MBuffer::reverse_iterator it = buff.rbegin (); it != buff.rend (); ++it)
  {
    BufferRepresentationEntryMap::iterator k = it->second.find(repId);
    if(k != it->second.end())
    {
      return k->second.segmentNumber;
    }
  }
  return 0;
}


MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::consumeFromBuffer()
{
  BufferRepresentationEntry entryConsumed;

  if(isEmpty())
    return entryConsumed;

  MBuffer::iterator it = buff.find (toConsumeSegmentNumber);
  if(it == buff.end ())
  {
    fprintf(stderr, "Could not find SegmentNumber. This should never happen\n");
    return entryConsumed;
  }

  mtx.lock ();
  entryConsumed = getHighestConsumableRepresentation(toConsumeSegmentNumber);
  buff.erase (it);
  toConsumeSegmentNumber++;
  mtx.unlock ();
  return entryConsumed;
}

MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::getHighestConsumableRepresentation(int segmentNumber)
{
  BufferRepresentationEntry consumableEntry;

  // find the correct entry map
  MBuffer::iterator it = buff.find (segmentNumber);
  if(it == buff.end ())
  {
    return consumableEntry;
  }

  BufferRepresentationEntryMap map = it->second;

  //find entry with most depIds.
  unsigned int most_depIds = 0;
  for(BufferRepresentationEntryMap::iterator k = map.begin (); k != map.end (); ++k)
  {
    if(most_depIds <= k->second.depIds.size())
    {
      consumableEntry = k->second;
      most_depIds = k->second.depIds.size();
    }
  }
  return consumableEntry;
}

double MultimediaBuffer::getBufferedPercentage()
{
  return getBufferedSeconds() / maxBufferedSeconds;
}

double MultimediaBuffer::getBufferedPercentage(std::string repId)
{
  return getBufferedSeconds (repId) / maxBufferedSeconds;
}

