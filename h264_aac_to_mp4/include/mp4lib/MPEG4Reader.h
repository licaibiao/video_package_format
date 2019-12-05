#pragma once
#include "MP4Analyze.h"
#include "MP4Reader.h"
#include <vector>

class CMPEG4Reader : public CMP4Reader
{
public:
	CMPEG4Reader();
	virtual ~CMPEG4Reader();

public:
	//initialize mp4 reader
	virtual int Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo);

	//set input mp4 file
	virtual int SetInputFile(const char* pMp4File);

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

	char	m_strMp4SrcFile[255];
	MP4FileHandle		m_hFile;
	BYTE*				m_pBuffer;
	int					m_nBufSize;

	MP4_TRACK_INFO		m_stTrackInfo;
	//CArray<int , int>	m_arFrmType1;
	//std::map<int, int> m_arFrmType1;
	std::vector<int>	m_arFrmType1;
	BOOL				m_bFirstRead;
};