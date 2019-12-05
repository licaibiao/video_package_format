/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */

#ifdef WIN32
#include "mpeg4ip_win32.h"
#endif
#include "mp4av_hinters.h"

extern "C" bool MP4AV_Rfc2429Hinter (MP4FileHandle file,
				     MP4TrackId mediaTrackId,
				     uint16_t maxPayloadSize)
{
  uint32_t numSamples, maxSampleSize;
  MP4TrackId hid;
  MP4Duration duration;

  numSamples = MP4GetTrackNumberOfSamples(file, mediaTrackId);
  if (numSamples == 0) {
    return false;
  }
  maxSampleSize = MP4GetTrackMaxSampleSize(file, mediaTrackId);
  u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
  if (pSampleBuffer == NULL) {
    return false;
  }

  hid = MP4AddHintTrack(file, mediaTrackId);
  if (hid == MP4_INVALID_TRACK_ID) {
    return false;
  }

  uint8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;
  MP4SetHintTrackRtpPayload(file,
                            hid,
                            "H263-2000",
                            &payloadNumber,
                            0,
                            NULL,
                            true,
                            false);

  // strictly speaking, this is not required for H.263 - it's a quicktime
  // thing.
  u_int16_t videoWidth = MP4GetTrackVideoWidth(file, mediaTrackId);
  u_int16_t videoHeight = MP4GetTrackVideoHeight(file, mediaTrackId);
  
  char sdpString[80];
  sprintf(sdpString, "a=cliprect:0,0,%d,%d\015\012", videoHeight, videoWidth);
  
  MP4AppendHintTrackSdp(file, 
 			hid,
 			sdpString);

  for (uint32_t sid = 1; sid <= numSamples; sid++) {

    duration = MP4GetSampleDuration(file, mediaTrackId, sid);

    MP4AddRtpVideoHint(file, hid, false, 0);

    u_int32_t sampleSize = maxSampleSize;
    MP4Timestamp startTime;
    MP4Duration duration;
    MP4Duration renderingOffset;
    bool isSyncSample;

    bool rc = MP4ReadSample(file, mediaTrackId, sid,
                            &pSampleBuffer, &sampleSize,
                            &startTime, &duration,
                            &renderingOffset, &isSyncSample);

    if (!rc) {
      MP4DeleteTrack(file, hid);
      return false;
    }

    // need to skip the first 2 bytes of the packet - it is the
    //start code
    uint16_t payload_head = htons(0x400);
    uint32_t offset = sizeof(payload_head);
    uint32_t remaining = sampleSize - sizeof(payload_head);
    while (remaining) {
      bool last_pak = false;
      uint32_t len;

      if (remaining + 2 <= maxPayloadSize) {
        len = remaining;
        last_pak = true;
      } else {
        len = maxPayloadSize - 2;
      }
      MP4AddRtpPacket(file, hid, last_pak);

      MP4AddRtpImmediateData(file, hid,
                            (u_int8_t*)&payload_head, sizeof(payload_head));
      payload_head = 0;
      MP4AddRtpSampleData(file, hid, sid,
                          offset, len);
      offset += len;
      remaining -= len;
    }
    MP4WriteRtpHint(file, hid, duration, true);
  }
  return true;
}
