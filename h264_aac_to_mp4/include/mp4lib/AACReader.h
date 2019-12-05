
#ifndef __MP4_AAC_READER__
#define __MP4_AAC_READER__


#include "MP4Reader.h"
#include "mp4.h"

class CAACReader : public CMP4Reader
{
public:
	CAACReader(void);
	~CAACReader(void);

public:
	//initialize mp4 reader
	virtual int Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo);

	//set input mp4 file
	virtual int SetInputFile(const char* pMp4File);

	//set track ID 
	virtual int SetTrackID(UINT nTrackID);

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

private:
	MP4FileHandle		m_hFile;
	WAVE_FORMAT			m_wfx;
	int					m_nSamplePerFrm;
	BYTE				m_aacProfileLevel;
	int					m_nSampleCount;
	MP4_TRACK_INFO		m_stTrackInfo;
	int					m_nMaxAAcDecBufSize;
	BOOL				m_bprependADTS;

	char m_strMP4File[255];
	UINT				m_nTrackID;

	LPBYTE				m_pAudioSampleBuf;
};


#endif
