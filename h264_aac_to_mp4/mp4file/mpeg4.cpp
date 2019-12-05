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
 * Copyright (C) Cisco Systems Inc. 2000-2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

//#include <mp4av_common.h>
#ifdef WIN32
#include "mpeg4ip_win32.h"
#endif
#include "mbs.h"
#include "mp4av_mpeg4.h"
#include "mp4.h"
#define DEFAULT_PARM(x)	=x

extern "C" uint8_t *MP4AV_Mpeg4FindVosh (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	pBuf[3] == MP4AV_MPEG4_VOSH_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}

extern "C" bool MP4AV_Mpeg4ParseVosh(
				     u_int8_t* pVoshBuf, 
				     u_int32_t voshSize,
				     u_int8_t* pProfileLevel)
{
	CMemoryBitstream vosh;

	vosh.SetBytes(pVoshBuf, voshSize);

	try {
		vosh.GetBits(32);				// start code
		*pProfileLevel = vosh.GetBits(8);
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVosh(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profileLevel)
{
	CMemoryBitstream vosh;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 5);
			vosh.SetBytes(*ppBytes, (*pNumBytes) + 5);
			vosh.SetBitPosition((*pNumBytes) << 3);
		} else {
			vosh.AllocBytes(5);
		}

		vosh.PutBits(MP4AV_MPEG4_SYNC, 24);
		vosh.PutBits(MP4AV_MPEG4_VOSH_START, 8);
		vosh.PutBits(profileLevel, 8);

		*ppBytes = vosh.GetBuffer();
		*pNumBytes = vosh.GetNumberOfBytes();
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVo(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t objectId)
{
	CMemoryBitstream vo;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 9);
			vo.SetBytes(*ppBytes, *pNumBytes + 9);
			vo.SetBitPosition((*pNumBytes) << 3);
		} else {
			vo.AllocBytes(9);
		}

		vo.PutBits(MP4AV_MPEG4_SYNC, 24);
		vo.PutBits(MP4AV_MPEG4_VO_START, 8);
		vo.PutBits(0x08, 8);	// no verid, priority, or signal type
		vo.PutBits(MP4AV_MPEG4_SYNC, 24);
		vo.PutBits(objectId - 1, 8);

		*ppBytes = vo.GetBuffer();
		*pNumBytes = vo.GetNumberOfBytes();
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" uint8_t *MP4AV_Mpeg4FindVol (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	(pBuf[3] & 0xf0) == MP4AV_MPEG4_VOL_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}
extern "C" uint8_t *MP4AV_Mpeg4FindVop (uint8_t *pBuf, uint32_t buflen)
{
  while (buflen > 4) {
    if (pBuf[0] == 0x0 &&
	pBuf[1] == 0x0 &&
	pBuf[2] == 0x1 &&
	pBuf[3] == MP4AV_MPEG4_VOP_START) {
      return pBuf;
    }
    pBuf++;
    buflen--;
  }
  return NULL;
}

extern "C" bool MP4AV_Mpeg4ParseVol(
	u_int8_t* pVolBuf, 
	u_int32_t volSize,
	u_int8_t* pTimeBits, 
	u_int16_t* pTimeTicks, 
	u_int16_t* pFrameDuration, 
	u_int16_t* pFrameWidth, 
	u_int16_t* pFrameHeight,
	u_int8_t * aspectRatioDefine,
	u_int8_t * aspectRatioWidth,
	u_int8_t * aspectRatioHeight)

{
	CMemoryBitstream vol;
	uint8_t aspect;

	vol.SetBytes(pVolBuf, volSize);

	try {
		vol.SkipBits(32);				// start code

		vol.SkipBits(1);				// random accessible vol
		vol.SkipBits(8);				// object type id
		u_int8_t verid = 1;
		if (vol.GetBits(1)) {			// is object layer id
			verid = vol.GetBits(4);			// object layer verid
			vol.SkipBits(3);				// object layer priority
		}
		aspect = vol.GetBits(4);
		if (aspectRatioDefine != NULL)
		  *aspectRatioDefine = aspect;
		if (aspect == 0xF) { 	// aspect ratio info
		  if (aspectRatioWidth != NULL) 
		    *aspectRatioWidth = vol.GetBits(8);
		  else
		    vol.SkipBits(8);				// par width
		  if (aspectRatioHeight != NULL) 
		    *aspectRatioHeight = vol.GetBits(8);
		  else
		    vol.SkipBits(8);				// par height
		}
		if (vol.GetBits(1)) {			// vol control parameters
			vol.SkipBits(2);				// chroma format
			vol.SkipBits(1);				// low delay
			if (vol.GetBits(1)) {			// vbv parameters
				vol.SkipBits(15);				// first half bit rate
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// latter half bit rate
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// first half vbv buffer size
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(3);				// latter half vbv buffer size
				vol.SkipBits(11);				// first half vbv occupancy
				vol.SkipBits(1);				// marker bit
				vol.SkipBits(15);				// latter half vbv occupancy
				vol.SkipBits(1);				// marker bit
			}
		}
		u_int8_t shape = vol.GetBits(2); // object layer shape
		if (shape == 3 /* GRAYSCALE */ && verid != 1) {
			vol.SkipBits(4);				// object layer shape extension
		}
		vol.SkipBits(1);				// marker bit
		*pTimeTicks = vol.GetBits(16);		// vop time increment resolution 

		u_int8_t i;
		u_int32_t powerOf2 = 1;
		for (i = 0; i < 16; i++) {
			if (*pTimeTicks < powerOf2) {
				break;
			}
			powerOf2 <<= 1;
		}
		*pTimeBits = i;

		vol.SkipBits(1);				// marker bit
		if (vol.GetBits(1)) {			// fixed vop rate
			// fixed vop time increment
			*pFrameDuration = vol.GetBits(*pTimeBits); 
		} else {
			*pFrameDuration = 0;
		}
		if (shape == 0 /* RECTANGULAR */) {
			vol.SkipBits(1);				// marker bit
			*pFrameWidth = vol.GetBits(13);	// object layer width
			vol.SkipBits(1);				// marker bit
			*pFrameHeight = vol.GetBits(13);// object layer height
			vol.SkipBits(1);				// marker bit
		} else {
			*pFrameWidth = 0;
			*pFrameHeight = 0;
		}
		// there's more, but we don't need it
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4CreateVol(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profile,
	double frameRate,
	bool shortTime,
	bool variableRate,
	u_int16_t width,
	u_int16_t height,
	u_int8_t quantType,
	u_int8_t* pTimeBits)
{
	CMemoryBitstream vol;

	try {
		if (*ppBytes) {
			// caller must guarantee buffer against overrun
			memset((*ppBytes) + (*pNumBytes), 0, 20);
			vol.SetBytes(*ppBytes, *pNumBytes + 20);
			vol.SetBitPosition((*pNumBytes) << 3);
		} else {
			vol.AllocBytes(20);
		}

		/* VOL - Video Object Layer */
		vol.PutBits(MP4AV_MPEG4_SYNC, 24);
		vol.PutBits(MP4AV_MPEG4_VOL_START, 8);

		/* 1 bit - random access = 0 (1 only if every VOP is an I frame) */
		vol.PutBits(0, 1);
		/*
		 * 8 bits - type indication 
		 * 		= 1 (simple profile)
		 * 		= 4 (main profile)
		 */
		vol.PutBits(profile, 8);
		/* 1 bit - is object layer id = 1 */
		vol.PutBits(1, 1);
		/* 4 bits - visual object layer ver id = 1 */
		vol.PutBits(1, 4); 
		/* 3 bits - visual object layer priority = 1 */
		vol.PutBits(1, 3); 

		/* 4 bits - aspect ratio info = 1 (square pixels) */
		vol.PutBits(1, 4);
		/* 1 bit - VOL control params = 0 */
		vol.PutBits(0, 1);
		/* 2 bits - VOL shape = 0 (rectangular) */
		vol.PutBits(0, 2);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);

		u_int16_t ticks;
		if (shortTime /* && frameRate == (float)((int)frameRate) */) {
			ticks = (u_int16_t)(frameRate + 0.5);
		} else {
			ticks = 30000;
		}
		/* 16 bits - VOP time increment resolution */
		vol.PutBits(ticks, 16);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);

		u_int8_t rangeBits = 1;
		while (ticks > (1 << rangeBits)) {
			rangeBits++;
		}
		if (pTimeBits) {
			*pTimeBits = rangeBits;
		}

		/* 1 bit - fixed vop rate = 0 or 1 */
		if (variableRate) {
			vol.PutBits(0, 1);
		} else {
			vol.PutBits(1, 1);

			u_int16_t frameDuration = 
				(u_int16_t)((double)ticks / frameRate);

			/* 1-16 bits - fixed vop time increment in ticks */
			vol.PutBits(frameDuration, rangeBits);
		}
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 13 bits - VOL width */
		vol.PutBits(width, 13);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 13 bits - VOL height */
		vol.PutBits(height, 13);
		/* 1 bit - marker = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - interlaced = 0 */
		vol.PutBits(0, 1);

		/* 1 bit - overlapped block motion compensation disable = 1 */
		vol.PutBits(1, 1);
#if 0
		/* 2 bits - sprite usage = 0 */
		vol.PutBits(0, 2);
#else
		vol.PutBits(0, 1);
#endif
		/* 1 bit - not 8 bit pixels = 0 */
		vol.PutBits(0, 1);
		/* 1 bit - quant type = 0 */
		vol.PutBits(quantType, 1);
		if (quantType) {
			/* 1 bit - load intra quant mat = 0 */
			vol.PutBits(0, 1);
			/* 1 bit - load inter quant mat = 0 */
			vol.PutBits(0, 1);
		}
#if 0
		/* 1 bit - quarter pixel = 0 */
		vol.PutBits(0, 1);
#endif
		/* 1 bit - complexity estimation disable = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - resync marker disable = 1 */
		vol.PutBits(1, 1);
		/* 1 bit - data partitioned = 0 */
		vol.PutBits(0, 1);
#if 0
		/* 1 bit - newpred = 0 */
		vol.PutBits(0, 1);
		/* 1 bit - reduced resolution vop = 0 */
		vol.PutBits(0, 1);
#endif
		/* 1 bit - scalability = 0 */
		vol.PutBits(0, 1);

		/* pad to byte boundary with 0 then as many 1's as needed */
		vol.PutBits(0, 1);
		if ((vol.GetBitPosition() & 7) != 0) {
			vol.PutBits(0xFF, 8 - (vol.GetBitPosition() & 7));
		}

		*ppBytes = vol.GetBuffer();
		*pNumBytes = vol.GetBitPosition() >> 3;
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4ParseGov(
	u_int8_t* pGovBuf, 
	u_int32_t govSize,
	u_int8_t* pHours, 
	u_int8_t* pMinutes, 
	u_int8_t* pSeconds)
{
	CMemoryBitstream gov;

	gov.SetBytes(pGovBuf, govSize);

	try {
		gov.SkipBits(32);	// start code
		*pHours = gov.GetBits(5);
		*pMinutes = gov.GetBits(6);
		gov.SkipBits(1);		// marker bit
		*pSeconds = gov.GetBits(6);
	}
	catch (int e) {
		return false;
	}

	return true;
}

static bool Mpeg4ParseShortHeaderVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	int* pVopType)
{
	CMemoryBitstream vop;

	vop.SetBytes(pVopBuf, vopSize);

	try {
		// skip start code, temporal ref, and into type
		vop.SkipBits(22 + 8 + 5 + 3);	
		if (vop.GetBits(1) == 0) {
			*pVopType = VOP_TYPE_I;
		} else {
			*pVopType = VOP_TYPE_P;
		}
	}
	catch (int e) {
		return false;
	}

	return true;
}

extern "C" bool MP4AV_Mpeg4ParseVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	int* pVopType, 
	u_int8_t timeBits, 
	u_int16_t timeTicks, 
	u_int32_t* pVopTimeIncrement)
{
	CMemoryBitstream vop;

	vop.SetBytes(pVopBuf, vopSize);

	try {
		vop.SkipBits(32);	// skip start code

		switch (vop.GetBits(2)) {
		case 0:
			/* Intra */
			*pVopType = VOP_TYPE_I;
			break;
		case 1:
			/* Predictive */
			*pVopType = VOP_TYPE_P;
			break;
		case 2:
			/* Bidirectional Predictive */
			*pVopType = VOP_TYPE_B;
			break;
		case 3:
			/* Sprite */
			*pVopType = VOP_TYPE_S;
			break;
		}

		if (!pVopTimeIncrement) {
			return true;
		}

		u_int8_t numSecs = 0;
		while (vop.GetBits(1) != 0) {
			numSecs++;
		}
		vop.SkipBits(1);		// skip marker
		u_int16_t numTicks = vop.GetBits(timeBits);
		*pVopTimeIncrement = (numSecs * timeTicks) + numTicks; 
	}
	catch (int e) {
		return false;
	}

	return true;
}


extern "C" int MP4AV_Mpeg4GetVopType(u_int8_t* pVopBuf, u_int32_t vopSize)
{
	int vopType = 0;

	if (vopSize <= 4) {
		return vopType;
	}

	if (pVopBuf[0] == 0 && pVopBuf[1] == 0 
	  && (pVopBuf[2] & 0xFC) == 0x08 && (pVopBuf[3] & 0x03) == 0x02) {
		// H.263, (MPEG-4 short header mode)
		Mpeg4ParseShortHeaderVop(pVopBuf, vopSize, &vopType);
		
	} else {
		// MPEG-4 (normal mode)
		MP4AV_Mpeg4ParseVop(pVopBuf, vopSize, &vopType, 0, 0, NULL);
	}

	return vopType;
}

// MP4AV_calculate_dts_from_pts
// a crude method of determining the decode timestamp from the 
// presentation timestamp.
// the player needs the decode timestamp - it uses that coupled with
// if the decoder returns a decoded frame for timing
extern "C" int MP4AV_calculate_dts_from_pts (mp4av_pts_to_dts_t *ptr,
					     uint64_t_ pts,
					     int type,
					     uint64_t_ *dts)
{
  if (type == VOP_TYPE_B) {
    // If we have a B type, the dts is always the pts.
    ptr->last_type = type;
    ptr->last_dts = *dts = pts;
    return 0;
  } 
  // We have a I or P type.
  if (ptr->have_last_pts && ptr->last_pts >= ptr->last_dts) {
    // if we have a PTS stored, and the PTS is greater than the last
    // dts we stored (from a b frame, most likely), we will return
    // that pts as the dts.
    // we also need to record this frames pts
    ptr->have_last_pts = 1;
    *dts = ptr->last_dts = ptr->last_pts;
    ptr->last_pts = pts;
    ptr->last_type = type;
    return 0;
  }
 
  // we don't have a pts stored, or we have a situation where the last
  // stored pts is less that dts we've gotten from B frames - this is
  // when lost I or P frames occur
  ptr->have_last_pts = 1;
  ptr->last_pts = pts;
  if (ptr->last_type == 0) {
    // we can't do anything with the timing here - we just store,
    // and return an error
    ptr->last_dts = pts;
    ptr->last_type = type;
    return -1;
  }
  // we have a last type, so we'll use the dts from that frame, add
  // a small amount, then hit it up the next time - we'll have a pts
  // stored.
  ptr->last_dts = *dts = ptr->last_dts + (uint64_t_)(1000.0 / 29.97);
  ptr->last_type = type;
  return 0;
}

extern "C" void MP4AV_clear_dts_from_pts (mp4av_pts_to_dts_t *ptr)
{
  ptr->last_type = 0;
  ptr->have_last_pts = 0;
}




///add by sb
#define MPEG3_START_CODE_PREFIX          0x000001
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af

extern "C" int MP4AV_Mpeg3FindNextStart (const uint8_t *pbuffer, 
					 uint32_t buflen,
					 uint32_t *optr, 
					 uint32_t *scode)
{
  uint32_t value;
  uint32_t offset;

  if (buflen < 4) return -1;
  for (offset = 0; offset < buflen - 3; offset++, pbuffer++) {
#ifdef WORDS_BIGENDIAN
    value = *(uint32_t *)pbuffer >> 8;
#else
    value = (pbuffer[0] << 16) | (pbuffer[1] << 8) | (pbuffer[2] << 0); 
#endif

    if (value == MPEG3_START_CODE_PREFIX) {
      *optr = offset;
      *scode = (value << 8) | pbuffer[3];
      return 0;
    }
  }
  return -1;
}

extern "C" int MP4AV_Mpeg3FindNextSliceStart (const uint8_t *pbuffer,
					      uint32_t startoffset, 
					      uint32_t buflen,
					      uint32_t *slice_offset)
{
  uint32_t slicestart, code;
  while (MP4AV_Mpeg3FindNextStart(pbuffer + startoffset, 
		       buflen - startoffset, 
		       &slicestart, 
		       &code) >= 0) {
#ifdef DEBUG_MPEG3_HINT
    printf("Code %x at offset %d\n", 
	   code, startoffset + slicestart);
#endif
    if ((code >= MPEG3_SLICE_MIN_START) &&
	(code <= MPEG3_SLICE_MAX_START)) {
      *slice_offset = slicestart + startoffset;
      return 0;
    }
    startoffset += slicestart + 4;
  }
  return -1;
}

extern "C" bool Mpeg12Hinter (MP4FileHandle mp4file,
			      MP4TrackId trackid,
			      uint16_t maxPayloadSize)
{
  uint32_t numSamples, maxSampleSize;
  uint8_t videoType;
  uint8_t rfc2250[4], rfc2250_2;
  uint32_t offset;
  uint32_t scode;
  int have_seq;
  bool stop;
  uint8_t *buffer, *pbuffer;
  uint8_t *pstart;
  uint8_t type;
  uint32_t next_slice, prev_slice;
  bool slice_at_begin;
  uint32_t sampleSize;
  MP4SampleId sid;

  MP4TrackId hintTrackId;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, trackid);
  maxSampleSize = MP4GetTrackMaxSampleSize(mp4file, trackid);

  if (numSamples == 0) return false;

  videoType = MP4GetTrackEsdsObjectTypeId(mp4file, trackid);

  if (videoType != MP4_MPEG1_VIDEO_TYPE &&
      videoType != MP4_MPEG2_VIDEO_TYPE) {
    return false;
  }

  hintTrackId = MP4AddHintTrack(mp4file, trackid);
  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  uint8_t payload = 32;
  MP4SetHintTrackRtpPayload(mp4file, hintTrackId, "MPV", &payload, 0);


  buffer = (uint8_t *)malloc(maxSampleSize);
  if (buffer == NULL) {
    MP4DeleteTrack(mp4file, hintTrackId);
    return false;
  }

  maxPayloadSize -= 24; // this is for the 4 byte header

  for (sid = 1; sid <= numSamples; sid++) {
    sampleSize = maxSampleSize;
    MP4Timestamp startTime;
    MP4Duration duration;
    MP4Duration renderingOffset;
    bool isSyncSample;

    bool rc = MP4ReadSample(mp4file, trackid, sid,
			    &buffer, &sampleSize, 
			    &startTime, &duration, 
			    &renderingOffset, &isSyncSample);
#ifdef DEBUG_MPEG3_HINT
    printf("sid %d - sample size %d\n", sid, sampleSize);
#endif
    if (rc == false) {
      MP4DeleteTrack(mp4file, hintTrackId);
      return false;
    }

    // need to add rfc2250 header
    offset = 0;
    have_seq = 0;
    pbuffer = buffer;
    stop = false;
    do {
      uint32_t oldoffset;
      oldoffset = offset;
      if (MP4AV_Mpeg3FindNextStart(pbuffer + offset, 
			sampleSize - offset, 
			&offset, 
			&scode) < 0) {
	// didn't find the start code
#ifdef DEBUG_MPEG3_HINT
	printf("didn't find start code\n");
#endif
	stop = true;
      } else {
	offset += oldoffset;
#ifdef DEBUG_MPEG3_HINT
	printf("next sscode %x found at %d\n", scode, offset);
#endif
	if (scode == MPEG3_SEQUENCE_START_CODE) have_seq = 1;
	offset += 4; // start with next value
      }
    } while (scode != MPEG3_PICTURE_START_CODE && stop == false);
    
    pstart = pbuffer + offset; // point to inside of picture start
    type = (pstart[1] >> 3) & 0x7;

    rfc2250[0] = (*pstart >> 6) & 0x3;
    rfc2250[1] = (pstart[0] << 2) | ((pstart[1] >> 6) & 0x3); // temporal ref
    rfc2250[2] = type;
    rfc2250_2 = rfc2250[2];

    rfc2250[3] = 0;
    if (type == 2 || type == 3) {
      rfc2250[3] = pstart[3] << 5;
      if ((pstart[4] & 0x80) != 0) rfc2250[3] |= 0x10;
      if (type == 3) {
	rfc2250[3] |= (pstart[4] >> 3) & 0xf;
      }
    }

    MP4AddRtpVideoHint(mp4file, hintTrackId, type == 3, renderingOffset);
    // Find the next slice.  Then we can add the header if the next
    // slice will be in the start.  This lets us set the S bit in 
    // rfc2250[2].  Then we need to loop to find the next slice that's
    // not in the buffer size - this should be in the while loop.
    
    prev_slice = 0;
    if (MP4AV_Mpeg3FindNextSliceStart(pbuffer, offset, sampleSize, &next_slice) < 0) {
      slice_at_begin = false;
    } else {
      slice_at_begin = true;
    }

#ifdef DEBUG_MPEG3_HINT
    printf("starting slice at %d\n", next_slice);
#endif
    offset = 0;
    bool nomoreslices = false;
    bool found_slice = slice_at_begin;
    bool onfirst = true;

    while (sampleSize > 0) {
      bool isLastPacket;
      uint32_t len_to_write;

      if (sampleSize <= maxPayloadSize) {
	// leave started_slice alone
	len_to_write = sampleSize;
	isLastPacket = true;
	prev_slice = 0;
      } else {
	found_slice =  (onfirst == false) && (nomoreslices == false) && (next_slice <= maxPayloadSize);
	onfirst = false;
	isLastPacket = false;

	while (nomoreslices == false && next_slice <= maxPayloadSize) {
	  prev_slice = next_slice;
	  if (MP4AV_Mpeg3FindNextSliceStart(pbuffer, next_slice + 4, sampleSize, &next_slice) >= 0) {
#ifdef DEBUG_MPEG3_HINT
	    printf("prev_slice %u next slice %u %u\n", prev_slice, next_slice,
		   offset + next_slice);
#endif
	    found_slice = true;
	  } else {
	    // at end
	    nomoreslices = true;
	  }
	}
	// prev_slice should have the end value.  If it's not 0, we have
	// the end of the slice.
	if (found_slice) len_to_write = prev_slice;
	else len_to_write = MIN(maxPayloadSize, sampleSize);
      } 

      rfc2250[2] = rfc2250_2;
      if (have_seq != 0) {
	rfc2250[2] |= 0x20;
	have_seq = 0;
      }
      if (slice_at_begin) {
	rfc2250[2] |= 0x10; // set b bit
      }
      if (found_slice || isLastPacket) {
	rfc2250[2] |= 0x08; // set end of slice bit
	slice_at_begin = true; // for next time
      } else {
	slice_at_begin = false;
      }

#ifdef DEBUG_MPEG3_HINT
      printf("Adding packet, sid %u prev slice %u len_to_write %u\n",
	     sid, prev_slice, len_to_write);
      printf("Next slice %u offset %u %x %x %x %x\n\n", 
	     next_slice, offset, 
	     rfc2250[0], rfc2250[1], rfc2250[2], rfc2250[3]);
#endif

      // okay - we can now write out this packet.
      MP4AddRtpPacket(mp4file, hintTrackId, isLastPacket);
      // add the 4 byte header
      MP4AddRtpImmediateData(mp4file, hintTrackId, rfc2250, sizeof(rfc2250));

      // add the immediate data
      MP4AddRtpSampleData(mp4file, hintTrackId, sid, offset, len_to_write);
      offset += len_to_write;
      sampleSize -= len_to_write;
      prev_slice = 0;
      next_slice -= len_to_write;
      pbuffer += len_to_write;
    }
    MP4WriteRtpHint(mp4file, hintTrackId, duration, type == 1);
  }

  free(buffer);
  return true;
}
