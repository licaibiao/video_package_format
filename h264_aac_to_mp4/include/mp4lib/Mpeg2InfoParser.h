// Mpeg2InfoParser.h: interface for the CMpeg2InfoParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MPEG2INFOPARSER_H__0DDC0671_894F_4015_AECD_BEFE766945C6__INCLUDED_)
#define AFX_MPEG2INFOPARSER_H__0DDC0671_894F_4015_AECD_BEFE766945C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "TsStructDefine.h"
#include "Utls.h"

#include "mpeg4ip.h"

struct FRAME_Info
{
	//only valid for video
	int nWidth;
	int nHeight;
	double dFramerate;
	//1 for I frame, 2 for P frame, 3 for B frame
	int nFrameType;
	
	BOOL bHaveSeqHeader;
	BOOL bHaveSeqExt;
	
	int  nBitRate;

	FRAME_Info()
	{
		nWidth = 0;
		nHeight = 0;
		dFramerate = 0;
		nFrameType = 0;
		bHaveSeqHeader = FALSE;
		bHaveSeqExt = FALSE;
		nBitRate = 0;
	}
	
	/////////////////////////
};


//use this struct to store bits which are shifted out of a bit stream
//##ModelId=3D241F980353
typedef struct _Shift_Delta_Bits
{
	int len;//in bytes, to store all bits
	byte data ;
	bool bLeftAligned; //true for shift from left to right, false for right to left
}stShift_Delta_Bits,* LPShift_Delta_Bits;


class CMpeg2InfoParser  
{
public:
	CMpeg2InfoParser();
	virtual ~CMpeg2InfoParser();
public:
	int GetMpeg2Info(BYTE* pBuffer, int nBufSize, FRAME_Info& stFrameInfo);
	int GetFrameType(BYTE* pBuffer, int nBufSize,int& nFrameType);

private:
	BOOL FindStartCode(BYTE* pBuffer, int nBufSize,int& nOff);
};

#endif // !defined(AFX_MPEG2INFOPARSER_H__0DDC0671_894F_4015_AECD_BEFE766945C6__INCLUDED_)
