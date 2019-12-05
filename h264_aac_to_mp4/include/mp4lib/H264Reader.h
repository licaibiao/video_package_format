
#ifndef __MP4_H264_READER__
#define __MP4_H264_READER__

#include "MP4Analyze.h"
#include "MP4Reader.h"

class CH264Reader : public CMP4Reader
{
public:
	CH264Reader();
	virtual ~CH264Reader();

public:
	//initialize mp4 reader
	virtual int Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo);

	//set input mp4 file
	virtual int SetInputFile( const char* pMp4File);

	//set track ID 
	virtual int SetTrackID(uint32_t nTrackID);

	//start read
	virtual int StartRead();


	//get track infomation
	virtual int GetTrackInfo(MP4_TRACK_INFO* pstTrackInfo) ;

	//get Video frame
	virtual int GetFrame(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameBufSize,int& nFrameType);

	//stop reader
	virtual int StopRead();

	//release resource
	virtual int UnInitialize();

protected:
	typedef void*		MP4FileHandle;

private:
	int					m_nTrackID;

	BYTE*				m_pSeqParm;
	int					m_nSeqNum;
	BYTE*				m_pPicParm;
	int					m_nPicNum;
	BYTE*				m_pFrameBuf;
	int					m_nMaxFrameSize;

	BOOL				m_bFirstRead;
	MP4_TRACK_INFO		m_stTrackInfo;
	MP4FileHandle		m_hFile;
	char				m_strMp4SrcFile[256];
};

#endif