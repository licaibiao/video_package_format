// Mp4VReader.cpp: implementation of the CMPEG4Reader class.
//
//////////////////////////////////////////////////////////////////////

#include "MPEG4Reader.h"
#include "mp4.h"
#include "Mpeg2InfoParser.h"

#define FRAMETYPE_ERROR				0x10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef WIN32
#define __int32 int
#define __int8 char
#endif

CMPEG4Reader::CMPEG4Reader()
{
	m_hFile			= NULL;
	m_pBuffer		= NULL;

	m_nTrackID		= -1;
	m_bFirstRead	= TRUE;
	m_nBufSize		= 0;

	memset(&m_stTrackInfo,0,sizeof(m_stTrackInfo));
}

CMPEG4Reader::~CMPEG4Reader()
{
	UnInitialize();
}


int CMPEG4Reader::Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo)
{
	//UnInitialize();
	return TRUE;
}

int CMPEG4Reader::SetInputFile(/*LPCTSTR*/const char* strMp4File)
{
	strcpy( m_strMp4SrcFile, strMp4File);

	return TRUE;
}

int CMPEG4Reader::SetTrackID(UINT nTrackID)
{
	m_nTrackID = nTrackID;
	return 1;
}

int CMPEG4Reader::StartRead()
{
#ifdef WIN32
	OutputDebugString(_T("h264  start Reads! "));
#endif
	StopRead();

	int		nRet = 0;
	char	file[/*_MAX_PATH*/260];

	strcpy(file, m_strMp4SrcFile );
//	wcstombs( m_strMp4SrcFile, _MAX_PATH);
	m_hFile	= MP4Read(file);
	if(m_hFile == NULL)
	{
		printf(/*_T*/("mpeg4 MP4Read() failed! "));
		return 0;
	}

	m_stTrackInfo.dwTrackType				= MP4_TRACKTYPE_VIDEO;
	m_stTrackInfo.dwTrackID					= m_nTrackID;
	m_stTrackInfo.dwVideoType				= MP4_VIDEOTYPE_MPEG4;

	m_stTrackInfo.dwFrameCount				= MP4GetTrackNumberOfSamples(m_hFile,m_nTrackID);
	m_stTrackInfo.stVideoBmpInfo.biWidth	= MP4GetTrackVideoWidth(m_hFile,m_nTrackID);
	m_stTrackInfo.stVideoBmpInfo.biHeight	= MP4GetTrackVideoHeight(m_hFile,m_nTrackID);
	
//	m_stTrackInfo.dFps = MP4GetTrackVideoFrameRate(m_hFile,m_nTrackID);
	//开源代码中存在精度丢失，改用在外部计算帧率
	u_int32_t	nTimeScale = MP4GetTrackTimeScale(m_hFile,m_nTrackID) ;
	MP4Duration	nDur = MP4GetTrackDuration(m_hFile,m_nTrackID) ;

	if ( nDur ) 
		m_stTrackInfo.dFps = 1.0*m_stTrackInfo.dwFrameCount * nTimeScale / nDur;
	else 
		m_stTrackInfo.dFps = MP4GetTrackVideoFrameRate(m_hFile,m_nTrackID);
	
		
	m_nBufSize	= MP4GetTrackMaxSampleSize(m_hFile,m_nTrackID) + 1024;
	m_pBuffer	= new BYTE[m_nBufSize];

	int	nVideoType= MP4GetTrackEsdsObjectTypeId(m_hFile, m_nTrackID) ;
	switch(nVideoType)
	{
	case MP4_MPEG2_MAIN_VIDEO_TYPE:
	case MP4_MPEG2_SIMPLE_VIDEO_TYPE:
	case MP4_MPEG2_SNR_VIDEO_TYPE:
	case MP4_MPEG2_SPATIAL_VIDEO_TYPE:
	case MP4_MPEG2_HIGH_VIDEO_TYPE:
	case MP4_MPEG2_442_VIDEO_TYPE:
		{
			m_stTrackInfo.stVideoBmpInfo.biCompression = 'SEMM';
			//找第一个I帧，解析出码率
			for (int i = 1 ; i <= m_stTrackInfo.dwFrameCount ; i++)
			{
				if (TRUE == MP4GetSampleSync(m_hFile, m_nTrackID, i)) 
				{
					unsigned __int32 bytesRead= 0;
					BYTE*	pTempBuf= m_pBuffer ;
					bool bRet = MP4ReadSample(m_hFile, m_nTrackID, i,(BYTE **)&pTempBuf, &bytesRead) ;
					if (bRet) 
					{
						FRAME_Info	stFrameInfo;
						CMpeg2InfoParser Mpeg2Parse;

						Mpeg2Parse.GetMpeg2Info( (BYTE*)pTempBuf,bytesRead,stFrameInfo );
						m_stTrackInfo.nBitRate	= stFrameInfo.nBitRate;
					}
					break;
				}
			}
		}

		break;
	case MP4_MPEG4_VIDEO_TYPE:
		{
			m_stTrackInfo.stVideoBmpInfo.biCompression	= 'xvid';
			break;
		}
	}

	m_stTrackInfo.stVideoBmpInfo.biPlanes	= 1;
	m_stTrackInfo.stVideoBmpInfo.biBitCount	= 24;
	m_stTrackInfo.stVideoBmpInfo.biSizeImage	
		= m_stTrackInfo.stVideoBmpInfo.biWidth * m_stTrackInfo.stVideoBmpInfo.biHeight * 3;

	BYTE*	pFrmbuf = (BYTE*)malloc(m_nBufSize); 
	if(pFrmbuf==NULL)return 0;
	DWORD	nFrmbufsize;
/*	int nFrmnum=1;*/
	int		nFrmtype=0;
	int		i = 0;
	int		b = 0;
	int		p = 0;
	int		j;
	for (j = 0 ; j < m_stTrackInfo.dwFrameCount ;)
	{
		GetFrame(j, pFrmbuf,0, &nFrmbufsize, nFrmtype);
		switch (nFrmtype)
		{
		case 1: 
			i = 1;
			break;
		case 2: 
			p++;
			break;
		case 3: 
		case 4:
			b++;
			break;
		default:
			return -1;
		}

		j++;
		if (nFrmtype == 1) 
		{
			p = 0;
			b = 0;

			m_arFrmType1.push_back(nFrmtype);
			//m_arFrmType1.Add(nFrmtype);
			break;
		}
	}

	int  nNonKeyCount = 0;
	for (; j < m_stTrackInfo.dwFrameCount ; j++)
	{
		GetFrame(j,pFrmbuf,0,&nFrmbufsize,nFrmtype);
		if (nFrmtype == 1)
			break;
		else
			m_arFrmType1.push_back(nFrmtype);//m_arFrmType1.Add(nFrmtype);

		if (nFrmtype != 1) 
			nNonKeyCount++;

		switch (nFrmtype)
		{
		case 0: 
			break;//非I帧
		case 2: 
			p++;
			break;
		case 3: 
		case 4:
			b++;
			break;
		default:
			{free(pFrmbuf);return -2;}
		}
	}
	free(pFrmbuf);

	m_stTrackInfo.stMp4GOPInfo.i	= i;
	m_stTrackInfo.stMp4GOPInfo.b	= b;
	m_stTrackInfo.stMp4GOPInfo.p	= p;
	m_stTrackInfo.stMp4GOPInfo.total_gop_count	= i + nNonKeyCount;
//	m_stTrackInfo.stMp4GOPInfo.total_gop_count				= i+b+p;
	if (i == 0) m_stTrackInfo.stMp4GOPInfo.total_gop_count	= 0;
	
	return TRUE;
}

int CMPEG4Reader::GetTrackInfo(MP4_TRACK_INFO* pstTrackInfo)
{
	if (pstTrackInfo != NULL) 
	{
		memcpy(pstTrackInfo, &m_stTrackInfo, sizeof(m_stTrackInfo));
	}

	return 1;
}

int CMPEG4Reader::GetFrame(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameBufSize,int& nFrameType)
{	
	int nRet = 0;
	*pFrameBufSize = 0;

	//mp4 file sample, start from 1
	dwFrmNo	+= 1;

	BYTE* pBuffer = pFrame ;		
	BOOL  isSycSample  = MP4GetSampleSync(m_hFile, m_nTrackID, dwFrmNo) ;
	//modify by wxc 2010.1.27
	if (isSycSample && \
		/*m_stTrackInfo.stVideoBmpInfo.biCompression == 'xvid'*/ \
		strcpy((char *)(&(m_stTrackInfo.stVideoBmpInfo.biCompression)),"xvid")==0) //Modify by cwl
	{
		unsigned int	configLength = 0;//100000;
		unsigned char*	FirstBuffer = NULL;//(unsigned char *)malloc(100000);
		MP4GetTrackESConfiguration(m_hFile, m_nTrackID, (unsigned __int8 **)&FirstBuffer, (unsigned __int32 *)&configLength);
		if(FirstBuffer)
		{
			memcpy(pBuffer, FirstBuffer, configLength);
			pBuffer			+= configLength;
			*pFrameBufSize  = configLength;

			free(FirstBuffer);
			FirstBuffer		= NULL;
		}
	}
	//end

	UINT bytesRead = 0;
	if (pFrame != NULL)
	{
#ifdef ENABLE_WRITELOG
		LARGE_INTEGER llFrequency, llBegin, llEnd;
		QueryPerformanceFrequency(&llFrequency);
		QueryPerformanceCounter(&llBegin);
#endif

		bool bRet = MP4ReadSample(m_hFile, m_nTrackID, dwFrmNo, (unsigned __int8 **)&pBuffer, (unsigned __int32 *)&bytesRead);
		if (!bRet || bytesRead <= 0) 
		{
			*pFrame = NULL;
			*pFrameBufSize = 0;
			return -1;
		}

#ifdef ENABLE_WRITELOG
		QueryPerformanceCounter(&llEnd);
		double dMsc = (llEnd.QuadPart - llBegin.QuadPart)*1000.0f;
		dMsc /= llFrequency.QuadPart;
		*g_pLog<(int)this<_T("Mp4ReadSample ")<dwFrmNo<_T(": dur = ")<dMsc<endl;
#endif	

		switch(m_stTrackInfo.stVideoBmpInfo.biCompression)
		{
		case 'SEMM':
			{
				CMpeg2InfoParser  mpeg2Parse;
				nRet = mpeg2Parse.GetFrameType(pBuffer,bytesRead, nFrameType);
			}
			break;
		case 'xvid':
			nFrameType = isSycSample;
			break;	
		}
		//pFrame = m_pBuffer;
		*pFrameBufSize += bytesRead ;	
	}
	else
	{
		switch(m_stTrackInfo.stVideoBmpInfo.biCompression)
		{
		case 'SEMM':
			{
				int nCount = m_arFrmType1.size(); //m_arFrmType1.GetSize();
				int nFrameNo = dwFrmNo - 1;
				int nNum = nFrameNo - ((nFrameNo/nCount)*nCount);
				nFrameType = m_arFrmType1[nNum];
			}
			break;
		case 'xvid':
			nFrameType = isSycSample;
			break;
		default:
			nFrameType = FRAMETYPE_ERROR;
		}

		*pFrameBufSize = m_nBufSize;
	}

	return TRUE;	
}

int CMPEG4Reader::StopRead()
{
	if(m_hFile)
	{
		MP4Close(m_hFile);
		m_hFile = NULL;
	}

	return TRUE;
}

int CMPEG4Reader::UnInitialize()
{
	StopRead();

	if (m_pBuffer != NULL)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}

	return TRUE;
}
