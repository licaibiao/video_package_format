#pragma once
#include "MP4Analyze.h"

enum AACType
{
	AAC_Main = 1, 
	AAC_Low = 2, 
	AAC_SSR = 3, 
	AAC_Lip = 4
};

class  CMP4Writer
{
public:
	

public:
	CMP4Writer(void);
	virtual ~CMP4Writer(void);

public:
	static CMP4Writer*	CreateWriter();
	static void			DeleteWriter(CMP4Writer* pWriter);

public:
	virtual BOOL	Initialize() = 0;
	virtual BOOL	SetOutputFile(const char* lpszMp4File) = 0;
	virtual BOOL	AddMPEG4Track(int& nTrackId, const int nWidth, const int nHeight, const double fps ) = 0;
	virtual BOOL	AddH264Track(int& nTrackId, const int nWidth, const int nHeight, const double fps ) = 0;
	virtual BOOL	AddAACTrack(int& nTrackId, const int nSamplePerSec, const int nSamplePerFrame = -1) = 0;
	virtual BOOL	StartWrite() = 0;
	virtual BOOL	WriteFrame(const int nTrackId, const BYTE* const lpData, const int nSize, const MP4Timestamp nTimestamp,const MP4Duration nDuration = -1) = 0;
	virtual BOOL	StopWrite() = 0;
};
