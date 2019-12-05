#ifndef _MP4_ANALYZE_
#define _MP4_ANALYZE_

#include "Utls.h"
#include "mp4.h"

#define WAVE_FORMAT_AAC			0xA106

#define MP4_TRACKTYPE_NONE		0x00
#define MP4_TRACKTYPE_VIDEO		0x01
#define MP4_TRACKTYPE_AUDIO		0x02
#define MP4_MAX_TRACK_NUM		10 

#define MP4_VIDEOTYPE_UNKNOWN	0
#define MP4_VIDEOTYPE_H263		1
#define MP4_VIDEOTYPE_H264		2
#define MP4_VIDEOTYPE_MPEG4		3

#define MP4_AUDIOTYPE_UNKNOWN	0
#define MP4_AUDIOTYPE_AMR		1
#define MP4_AUDIOTYPE_AMRWB		2
#define MP4_AUDIOTYPE_MP3		3
#define MP4_AUDIOTYPE_MP4		4

typedef struct _MP4_GOP_INFO{
	_MP4_GOP_INFO()
	{
		total_gop_count = 1;
		i =1;
		b = 0;
		p =0;
	}

	int total_gop_count;
	int i;
	int b;
	int p;
}MP4_GOP_INFO;

typedef struct _WAVEFORMATEX
{
	WORD        wFormatTag;         /* format type */
	WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
	DWORD       nSamplesPerSec;     /* sample rate */
	DWORD       nAvgBytesPerSec;    /* for buffer estimation */
	WORD        nBlockAlign;        /* block size of data */
	WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	WORD        cbSize;             /* the count in bytes of the size of */
	/* extra information (after cbSize) */
} WAVE_FORMAT;

typedef struct _BITMAPINFOHEADER{
	DWORD      biSize;
	long       biWidth;
	long       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	long       biXPelsPerMeter;
	long       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} BITMAP_INFO_HEADER; //, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;


struct  MP4_TRACK_INFO
{
	MP4_TRACK_INFO()
	{
		dwTrackID		= 0;
		dwTrackType		= MP4_TRACKTYPE_NONE; 
		dFps			= 0.0f;
		dAudioDuration	= 0.0f;

		stMp4GOPInfo.i	= 0;
		stMp4GOPInfo.b	= 0;
		stMp4GOPInfo.p	= 0;
		nBitRate		= 0;
		memset(&stAudioInfo, 0, sizeof(stAudioInfo));
		memset(&stVideoBmpInfo, 0, sizeof(stVideoBmpInfo));
		stMp4GOPInfo.total_gop_count= 0;

	}

	MP4_TRACK_INFO& operator=(const MP4_TRACK_INFO& src)
	{
		dFps		 = src.dFps;
		dwTrackID	 = src.dwTrackID;
		dwTrackType	 = src.dwTrackType;
		stMp4GOPInfo = src.stMp4GOPInfo;
		nBitRate	 = src.nBitRate;
		dwFrameCount = src.dwFrameCount;
		dAudioDuration = src.dAudioDuration;

		memcpy(&stAudioInfo, &src.stAudioInfo, sizeof(WAVE_FORMAT));
		memcpy(&stVideoBmpInfo, &src.stVideoBmpInfo, sizeof(BITMAP_INFO_HEADER));

		return *this;
	}
	
	DWORD	dwTrackID;//TRACK ID	
	DWORD	dwTrackType;//TRACK TYPE

	//Video Track Info
	int		nBitRate;
	DWORD	dwVideoType;
	DWORD	dwFrameCount;
	double	dFps;
	MP4_GOP_INFO		stMp4GOPInfo;
	BITMAP_INFO_HEADER	stVideoBmpInfo;


	//Audio Track Info
	DWORD			dwAudioType;
	WAVE_FORMAT		stAudioInfo;
	//audio duration, unit ms
	double			dAudioDuration;
};

struct  MP4_TRACKINFO_LIST
{
	MP4_TRACKINFO_LIST()
	{
		memset(stuTrackList, 0, sizeof(stuTrackList));
		nTrackListNum	= 0;
	}

	int	nTrackListNum;
	MP4_TRACK_INFO	stuTrackList[MP4_MAX_TRACK_NUM];
};

class  CMP4Analyze
{
public:
	static bool GetMP4FileInfo(const char* strFile, MP4_TRACKINFO_LIST& lsTrackInfo, char* pstrDesc = NULL, const int szDesc = 0);

protected:
	static bool	GetVideoTrackInfo(void* fh, int trackId, MP4_TRACK_INFO& info);
	static bool	GetAudioTrackInfo(void* fh, int trackId, MP4_TRACK_INFO& info);
};


#endif