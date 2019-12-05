// H264Reader.cpp: implementation of the CH264Reader class.
//
//////////////////////////////////////////////////////////////////////
#include "H264Reader.h"
#include "mp4.h"
#include "mp4util.h"
#ifdef WIN32
#include <tchar.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CH264Reader::CH264Reader()
{
	m_hFile = NULL;

	m_pSeqParm = NULL;
	m_nSeqNum = 0;
	m_pPicParm = NULL;
	m_nPicNum = 0;
	m_nMaxFrameSize = 0;
	m_pFrameBuf = NULL;

	memset(&m_stTrackInfo,0,sizeof(m_stTrackInfo));
	memset(&m_strMp4SrcFile,0, 256);
	m_bFirstRead = TRUE;
}

CH264Reader::~CH264Reader()
{
	UnInitialize();
}


int CH264Reader::Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo)
{
	DWORD nTrackID = MP4_INVALID_TRACK_ID;
	SetInputFile(strPath);

	if(!getTrackID(1,lsTrackInfo,nTrackID))
	{
		return 0;
	}
	SetTrackID( nTrackID );

	return StartRead();
}

int CH264Reader::SetInputFile(const char* pMp4File)
{
	strcpy(m_strMp4SrcFile , pMp4File);
	return TRUE;
}

int CH264Reader::SetTrackID( UINT nTrackID)
{
	m_nTrackID = nTrackID;
	return 1;
}

int CH264Reader::StartRead()
{
	StopRead();

	int		nRet = 0;
	char	file[/*_MAX_PATH*/260];
	strncpy( file, m_strMp4SrcFile, /*_MAX_PATH*/260 );
	
	m_hFile	 = MP4Read(file);
	if( m_hFile == NULL )
	{
		printf(/*_T*/("h264 MP4Read() failed! "));
		return 0;
	}

	m_stTrackInfo.dwTrackType				= MP4_TRACKTYPE_VIDEO;
	m_stTrackInfo.dwTrackID					= m_nTrackID;
	m_stTrackInfo.dwFrameCount				= MP4GetTrackNumberOfSamples(m_hFile,m_nTrackID);
	m_stTrackInfo.stVideoBmpInfo.biWidth	= MP4GetTrackVideoWidth(m_hFile,m_nTrackID);
	m_stTrackInfo.stVideoBmpInfo.biHeight	= MP4GetTrackVideoHeight(m_hFile,m_nTrackID);

	//	m_stTrackInfo.dFps = MP4GetTrackVideoFrameRate(m_hFile,m_nTrackID);
	//开源代码中存在精度丢失，在外部计算帧率
	u_int32_t	nTimeScale = MP4GetTrackTimeScale( m_hFile, m_nTrackID );
	MP4Duration	nDuration = MP4GetTrackDuration( m_hFile, m_nTrackID );
	
	m_stTrackInfo.dwVideoType	= MP4_VIDEOTYPE_H264;
	m_stTrackInfo.dFps		= 1.0 * m_stTrackInfo.dwFrameCount * nTimeScale/nDuration;
	m_stTrackInfo.stVideoBmpInfo.biBitCount		= 24;
	m_stTrackInfo.stVideoBmpInfo.biCompression	= '462H';
	
	if ( m_bFirstRead ) 
	{
		uint8_t**	pSeqHeader	= NULL;
		uint8_t**	pPicHeader	= NULL;
		uint32_t*	pSeqSize	= NULL;
		uint32_t*	pPicSize	= NULL;
		bool bRet = MP4GetTrackH264SeqPictHeaders(m_hFile, m_nTrackID, &pSeqHeader, &pSeqSize, &pPicHeader, &pPicSize);		
		if (!bRet)
		{
			printf(/*_T*/("h264 GetSeqPictHeaders failed! "));
			return 0;
		}

		uint32_t*	pTemp = pSeqSize;
		m_nSeqNum	= 0;
		while ( *pTemp > 0 )
		{
			m_nSeqNum += *pTemp;
			pTemp++;
		}

		m_pSeqParm	= new BYTE[m_nSeqNum];
		while ( *pSeqHeader != NULL && *pSeqSize > 0 )
		{
			memcpy(m_pSeqParm,*pSeqHeader,*pSeqSize);
			pSeqHeader++;
			pSeqSize++;
		}
		
		m_nPicNum	= 0;
		pTemp		= pPicSize;
		while( *pTemp > 0 ) 
		{
			m_nPicNum += *pTemp;
			pTemp++;
		}
		m_pPicParm	= new BYTE[m_nPicNum];

		while (*pPicHeader != NULL && *pPicSize > 0)
		{
			memcpy(m_pPicParm, *pPicHeader, *pPicSize);
			pPicHeader++;
			pPicSize++;
		}

		m_bFirstRead = FALSE;
		
		free(*pSeqHeader);
		free(*pPicHeader);
	}
	
	m_nMaxFrameSize	= MP4GetTrackMaxSampleSize(m_hFile,m_nTrackID);
	m_nMaxFrameSize	+= m_nSeqNum + m_nPicNum;
	//m_pFrameBuf		= new BYTE[ m_nMaxFrameSize + 1024];
	
	//ASSERT(m_pFrameBuf);
	
	return TRUE;
}

int CH264Reader::GetTrackInfo(MP4_TRACK_INFO* pstTrackInfo)
{
	if (pstTrackInfo != NULL) 
	{
		memcpy(pstTrackInfo,&m_stTrackInfo,sizeof(m_stTrackInfo));
	}

	return 1;
}

//外部提供的pFrame指向的内存空间必须大于（m_nMaxFrameSize + 1024） ，否则有可能出现越界写入的情况
int CH264Reader::GetFrame(DWORD dwSampleId, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameBufSize,int& nFrameType)
{
	*pFrameBufSize = 0;
	dwSampleId ++;
	if(frmBufLenth<m_nMaxFrameSize + 1024)
	{
		printf(/*_T*/("h264  buf lenth is too short! "));
		return 0;
	}

	//By cwl
	nFrameType = MP4GetSampleSync(m_hFile,m_nTrackID,dwSampleId);
	//nFrameType = 0;
	
	if ( pFrame == NULL)
	{
		*pFrameBufSize = 0;
		return 0;
	}
	
	BYTE*   pOut = pFrame;
	int     nFrameSize = 0;
	DWORD   nSysWord = 0x01000000;
	int     nOut = 0;
 
	if( nFrameType == 1 )
	{	
		printf("The %d sample is key sample!\n",dwSampleId);
#if 0
		//I帧才写
		//写同步
		memcpy( pOut,&nSysWord,4 );
		pOut += 4;
		memcpy( pOut, m_pSeqParm,m_nSeqNum );
		pOut += m_nSeqNum;
		
		//写同步
		memcpy( pOut,&nSysWord,4);
		pOut += 4;
		memcpy( pOut,m_pPicParm,m_nPicNum);
		pOut += m_nPicNum;

		nOut = m_nSeqNum + m_nPicNum + 8;
#endif
	}

	bool bRet = MP4ReadSample( m_hFile, m_nTrackID,dwSampleId,(uint8_t**)&pOut,(uint32_t*)&nFrameSize);
	if ( !bRet )
	{
		char szMsg[256] = {0};
		sprintf(szMsg,/*_T*/("h264  MP4ReadSample failed!  TrackID = %d ,Frame = %d"), m_nTrackID, dwSampleId);
		printf(szMsg);
		return 0;
	}

//By cwl
//#if 0
	DWORD  dwNAlSize = 0;
	int    nLeftSize = nFrameSize;
	while( TRUE ) 
	{
		dwNAlSize = pOut[0] * 256 * 256 * 256 
					+ pOut[1] * 256 * 256
					+ pOut[2] * 256 
					+ pOut[3] ;
	
		memcpy(pOut,&nSysWord,4);
		pOut += 4;
		nLeftSize -= 4;

		if (dwNAlSize == nLeftSize) 
			break;
		
		pOut += dwNAlSize;
		nLeftSize -= dwNAlSize;
		if ( nLeftSize < 0)
		{
			*pFrameBufSize = 0;
			printf(/*_T*/("h264  analyse error! "));
			return 0;
		}
	}
//#endif
	
	nOut += nFrameSize;

	//*ppFrame = m_pFrameBuf;
	*pFrameBufSize = nOut;

	return 1;
}

int CH264Reader::StopRead()
{
	if(m_hFile)
	{
		MP4Close(m_hFile);
		m_hFile = NULL;
	}

	return TRUE;
}

int CH264Reader::UnInitialize()
{
	StopRead();

	/*if (m_pFrameBuf != NULL)
	{
		delete[] m_pFrameBuf;
		m_pFrameBuf = NULL;
	}*/

	if (m_pSeqParm != NULL)
	{
		delete[] m_pSeqParm;
		m_pSeqParm = NULL;
	}

	if (m_pPicParm != NULL)
	{
		delete[] m_pPicParm;
		m_pPicParm = NULL;
	}

	return TRUE;
}
