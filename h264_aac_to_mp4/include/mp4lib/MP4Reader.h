
#ifndef _MP4_READER_
#define _MP4_READER_


#include "MP4Analyze.h"


class CMP4Reader
{
public:
	CMP4Reader();
	virtual ~CMP4Reader();

public:
	static CMP4Reader*	CreateReader(const DWORD dwMediaType);
	static void			DeleteReader(CMP4Reader* pReader);

public:
	//initialize mp4 reader
	virtual int Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo) = 0;

	//set input mp4 file
	virtual int SetInputFile(const char* strMp4File) = 0;

	//set track ID 
	virtual int SetTrackID(uint32_t nTrackID) = 0;

	//start read
	virtual int StartRead() = 0;

	//get track infomation
	virtual int GetTrackInfo(MP4_TRACK_INFO* pstTrackInfo) = 0;

	//get Video frame
	virtual int GetFrame(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameBufSize,int& nFrameType) = 0;

	//stop reader
	virtual int StopRead() = 0;

	//release resource
	virtual int UnInitialize() = 0;
protected:
	//searchCon
	//1 for video track id
	//2 for audio track id
	//-1 for not found
	static bool getTrackID(int searchCon,MP4_TRACKINFO_LIST lsTrackInfo,DWORD &trackID);
};

#endif