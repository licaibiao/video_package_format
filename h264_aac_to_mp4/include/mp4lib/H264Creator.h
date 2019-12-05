#ifndef __H264_CREATOR_H__
#define __H264_CREATOR_H__ 

//#include <mpeg4ip.h>
#include "Utls.h"
#include "mp4av_h264.h"
#include "mp4.h"
//#include <mp4av.h>
//#include <ismacryplib.h>

#ifdef NODEBUG
#define ASSERT(expr)
#else
#include <assert.h>
#define ASSERT(expr)	assert(expr)
#endif

// exit status
#define EXIT_SUCESS		0
#define EXIT_COMMAND_LINE	1
#define EXIT_CREATE_FILE	2
#define EXIT_CREATE_MEDIA	3
#define EXIT_CREATE_HINT	4
#define EXIT_OPTIMIZE_FILE	5
#define EXIT_EXTRACT_TRACK	6	
#define EXIT_INFO		7
#define EXIT_ISMACRYP_INIT     8
#define EXIT_ISMACRYP_END      9

// global variables
#ifdef MP4CREATOR_GLOBALS
#define MP4CREATOR_GLOBAL
#else
#define MP4CREATOR_GLOBAL extern
#endif

MP4CREATOR_GLOBAL char* ProgName;
MP4CREATOR_GLOBAL u_int32_t Verbosity;
MP4CREATOR_GLOBAL double VideoFrameRate;
MP4CREATOR_GLOBAL u_int32_t Mp4TimeScale;
MP4CREATOR_GLOBAL bool TimeScaleSpecified;
MP4CREATOR_GLOBAL bool VideoProfileLevelSpecified;
MP4CREATOR_GLOBAL int VideoProfileLevel;
MP4CREATOR_GLOBAL bool aacUseOldFile;
MP4CREATOR_GLOBAL int aacProfileLevel;


MP4TrackId H264Creator(MP4FileHandle mp4File, FILE *inFile);


#endif /* __H264_CREATOR_H__ */
