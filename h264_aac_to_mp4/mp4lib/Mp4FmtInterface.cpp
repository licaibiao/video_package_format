#include "Mp4FmtInterface.h"

#include "mp4av_adts.h"
#include "mp4av_aac.h"
#define MP4CREATOR_GLOBALS
#include "H264Creator.h"

#define ADTS_HEADER_MAX_SIZE  10

u_int8_t h263Profile = 0;
u_int8_t h263Level = 10;
u_int8_t H263CbrTolerance = 0;
bool setBitrates = false;
static bool allowVariableFrameRate = false;
static bool allowAvi = false;


#include <sys/types.h>
#include <sys/timeb.h>

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

CMp4FmtInterface::CMp4FmtInterface()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_nFrameRate = 0;
	m_nFrames = 0;
	m_nTimeScale = 0;
	m_sampleLenFieldSizeMinusOne = 3;   //缺省值为3
	m_nSamplePerSec = 0;
	m_nSamplePerFrame = 1024;
	m_nAudioChannels = 0;
	m_nAudioFmtTag = 0;
	m_nAudioBitsPerSample = 0;

	m_videoId = MP4_INVALID_TRACK_ID;
	m_audioId = MP4_INVALID_TRACK_ID;
	m_hMp4File = NULL ;

	m_curVideoTimeStamp = 0;
	m_curAudioTimeStamp = 0;

	m_nOpenModel = OPEN_MODEL_NA;

	//wirte
	m_pVideoWriter = NULL;
	m_pAudioWriter = NULL;

	//read
	m_pVideoReader = NULL;
	m_pAudioReader = NULL;

	m_isHaveAddSPS = false;
	m_isHaveAddPPS = false;
	m_isHaveAddAAC = false;

	m_pVideoFrameBuf = NULL;
	m_maxVideoFrameBufLen = 0;
}

CMp4FmtInterface::~CMp4FmtInterface()
{
	if ( m_nOpenModel == OPEN_MODEL_R )
	{
		CMP4Reader::DeleteReader( m_pVideoReader);
		CMP4Reader::DeleteReader( m_pAudioReader);
	}
	else if(m_nOpenModel == OPEN_MODEL_W )
	{
		//CMP4Writer::DeleteWriter( m_pMp4Writer );
	}

	if(m_pVideoFrameBuf!=NULL)
	{
		free(m_pVideoFrameBuf);
		m_pVideoFrameBuf==NULL;
	}
}

bool CMp4FmtInterface::OpenFile(const char* strPath, FILE_OPEN_MODEL nModel)
{
	//初始化之后，不能再次初始化。并且初始化时，必须指定读操作或者写操作。
	if(m_nOpenModel!=OPEN_MODEL_NA || nModel==OPEN_MODEL_NA)
	{
		return false;
	}
	m_nOpenModel = nModel;
	if( nModel == OPEN_MODEL_R )
	{ 
		if(!GetMP4FileInfo(strPath, m_lsTrackInfo))
		{
			return false;
		}
		AssignMp4Info();

		m_pVideoReader = CMP4Reader::CreateReader( MP4_VIDEOTYPE_H264 );
		m_pVideoReader->Initialize( strPath ,m_lsTrackInfo);

		m_pAudioReader = CMP4Reader::CreateReader( MP4_AUDIOTYPE_MP4 );
		m_pAudioReader->Initialize( strPath ,m_lsTrackInfo);

	}
	else if(nModel == OPEN_MODEL_W)
	{
		m_hMp4File = MP4CreateEx(strPath, MP4_DETAILS_ALL, 0, 1, 1, 0, 0, 0, 0);//创建mp4文件 
		if (m_hMp4File == MP4_INVALID_FILE_HANDLE) 
		{ 
			printf("creat mp4 file fail.\n"); 
			return false; 
		}
		//m_pMp4Writer = CMP4Writer::CreateWriter();
		//m_pMp4Writer->SetOutputFile( strPath );
	}

	return true;
}

bool CMp4FmtInterface::Close()
{
	if ( m_nOpenModel == OPEN_MODEL_R )
	{
		return (m_pVideoReader->UnInitialize() && m_pAudioReader->UnInitialize());
	}
	else if( m_nOpenModel == OPEN_MODEL_W )
	{
		if(m_hMp4File)
		{
			MP4Close(m_hMp4File);
		}
		//return m_pMp4Writer->StopWrite();
		return true;
	}
	return false;
}

bool CMp4FmtInterface::SetMp4Param(
				u_int16_t width,
				u_int16_t height,
				double frameRate, 
				u_int32_t timeScale /*= 90000*/, 
				int nSamplePerSec /*= 0*/,
				int nSamplePerFrame /*= 1024*/)
{
	if(m_nOpenModel!=OPEN_MODEL_W)
	{
		return false;
	}
	m_nWidth = width;
	m_nHeight = height;
	m_nFrameRate = frameRate;
	m_nTimeScale = timeScale;
	m_nSamplePerSec = nSamplePerSec;
	m_nSamplePerFrame = nSamplePerFrame;

	if(!MP4SetTimeScale(m_hMp4File, m_nTimeScale))
	{
		return false;
	}

	if(nSamplePerSec>0 && nSamplePerFrame>0)
	{
		//m_audioId = MP4AddAudioTrack(file, 48000, /*2048*/ /*3029*/ 1616, MP4_MP3_AUDIO_TYPE); 
		//m_audioId = MP4AddAudioTrack(m_hMP4File, nSamplePerSec, 1024, audioType) ;
		//MP4AV_AacGetConfiguration();
		//if ( m_audioId == MP4_INVALID_TRACK_ID ){return false;}
		
	}

	return true;
}


bool CMp4FmtInterface::WriteVideoFrameData(const unsigned char* pFrameData,int size)
{
	if(m_nOpenModel!=OPEN_MODEL_W || pFrameData == NULL || size <= 0)
	{
		return false;
	}

	if(size>m_maxVideoFrameBufLen)
	{
		if(m_pVideoFrameBuf!=NULL){free(m_pVideoFrameBuf);}
		m_pVideoFrameBuf = (unsigned char*)malloc(size);
		if(m_pVideoFrameBuf==NULL) {return false;}
		m_maxVideoFrameBufLen = size;
	}
	memset(m_pVideoFrameBuf,0,size);
	memcpy(m_pVideoFrameBuf,pFrameData,size);
	unsigned char *pData =  m_pVideoFrameBuf;

	bool ret = false;
	MP4ENC_NaluUnit nalu;
	int pos = 0, len = 0;
	unsigned char* pSampleNaluStart = NULL;
	unsigned long sampleNaluLenth = 0;
	bool bIsKeySample = false;
	while(len = ReadOneNaluFromBuf(pData,size,pos,nalu))
	{
		if(nalu.type == 0x07 && !m_isHaveAddSPS) // sps
		{
			// 添加h264 track    
			m_videoId = MP4AddH264VideoTrack
				(m_hMp4File, 
				m_nTimeScale, 
				m_nTimeScale / m_nFrameRate, 
				m_nWidth,     // width
				m_nHeight,    // height
				nalu.data[1], // sps[1] AVCProfileIndication
				nalu.data[2], // sps[2] profile_compat
				nalu.data[3], // sps[3] AVCLevelIndication
				3);           // 4 bytes length before each NAL unit
			if (m_videoId == MP4_INVALID_TRACK_ID)
			{
				printf("add video track failed.\n");
				return false;
			}
			MP4SetVideoProfileLevel(m_hMp4File, 0xf); //  Simple Profile @ Level 3

			MP4AddH264SequenceParameterSet(m_hMp4File,m_videoId,nalu.data,nalu.size);
			m_isHaveAddSPS = true;
		}
		else if(nalu.type == 0x08 && !m_isHaveAddPPS) // pps
		{
			MP4AddH264PictureParameterSet(m_hMp4File,m_videoId,nalu.data,nalu.size);
			m_isHaveAddPPS = true;
		}

		//未加头信息时，直接跳过该帧
		if(!m_isHaveAddSPS && !m_isHaveAddPPS)
		{
			pData += len;
		    size -= len;
			continue;
		}

		if(nalu.type == 0x07 || nalu.type == 0x08 || nalu.type == 0x06 || nalu.type == 0x05 || nalu.type == 0x01)
		{
			if(pSampleNaluStart==NULL){pSampleNaluStart=nalu.data-4;}
			sampleNaluLenth += len;
			if(nalu.type == 0x07)
			{
				//m_curVideoTimeStamp += m_nTimeScale / m_nFrameRate;
			}
			else if(nalu.type == 0x08)
			{
			}
			else if(nalu.type == 0x05)
			{
				bIsKeySample = true;
				m_curVideoTimeStamp += m_nTimeScale / m_nFrameRate;
			}
			else if(nalu.type == 0x01)
			{
				bIsKeySample = false;
				m_curVideoTimeStamp += m_nTimeScale / m_nFrameRate;
			}
			else if(nalu.type == 0x06)
			{
				printf("nalu type:0x06\n");
			}
			
			ret = true;
		}
		else
		{
			return false;
		}
		pData += len;
		size -= len;
	}

	if(pSampleNaluStart!=NULL && sampleNaluLenth!=0)
	{
		//m_curVideoTimeStamp += m_nTimeScale / m_nFrameRate;
		WriteSampleData(pSampleNaluStart,sampleNaluLenth,bIsKeySample);
	}
	

	return ret;
}

int CMp4FmtInterface::WriteSampleData(const unsigned char*smapleData,int datalen,bool bIsSyncSample)
{
	if(!MP4WriteSample(m_hMp4File, m_videoId, smapleData, datalen,m_curVideoTimeStamp,MP4_INVALID_DURATION, 0, bIsSyncSample))
	{
		return false;
	}
}

int CMp4FmtInterface::ReadOneNaluFromBuf(unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu)
{
	int i = offSet;
	while(i<nBufferSize)
	{
		if(buffer[i] == 0x00 &&
			buffer[i+1] == 0x00 &&
			buffer[i+2] == 0x00 &&
			buffer[i+3] == 0x01
			)
		{
			int pos = i+4;
			while (pos<=nBufferSize-4)
			{
				if(buffer[pos] == 0x00 &&
					buffer[pos+1] == 0x00 &&
					buffer[pos+2] == 0x00 &&
					buffer[pos+3] == 0x01
					)
				{
					break;
				}
				pos++;
			}
			if(pos > nBufferSize-4)
			{
				nalu.size = nBufferSize-i-4;	
			}
			else
			{
				nalu.size = pos-i-4;
			}

			nalu.type = buffer[i+4]&0x1f;
			nalu.data =(unsigned char*)&buffer[i+4]; 
			SetFrameDataLenth(buffer,nalu.size);
			return (nalu.size+i+4-offSet);
		}

		i++;
	}
	return 0;
}

void CMp4FmtInterface::SetFrameDataLenth(unsigned char* pNaluDataStart,int size)
{
	pNaluDataStart[0] = size>>24;
	pNaluDataStart[1] = size>>16;
	pNaluDataStart[2] = size>>8;
	pNaluDataStart[3] = size&0xff;		
}

bool CMp4FmtInterface::WriteAudioFrameData(const unsigned char* pData,int size)
{
	if(!m_isHaveAddAAC)
	{
		u_int32_t	nSamplePerSec =0;
		u_int8_t	nMpegVersion=0;
		u_int8_t	nProfile = 0;
		u_int8_t	nChannelConfig= 0;
		u_int8_t	firstHeader[ADTS_HEADER_MAX_SIZE];

		if ( !GetFirstHeader(pData, pData + size, firstHeader))
			return false;

		nSamplePerSec = MP4AV_AdtsGetSamplingRate(firstHeader);
		nMpegVersion  = MP4AV_AdtsGetVersion(firstHeader);
		nProfile	  = MP4AV_AdtsGetProfile(firstHeader);
		nChannelConfig= MP4AV_AdtsGetChannels(firstHeader);

		m_nSamplePerSec = nSamplePerSec;

		u_int8_t audioType(MP4_INVALID_AUDIO_TYPE);
		switch ( nMpegVersion ) 
		{
		case 0:
			audioType = MP4_MPEG4_AUDIO_TYPE;
			break;
		case 1:
			{
				switch ( nProfile ) 
				{
				case 0:
					audioType	= MP4_MPEG2_AAC_MAIN_AUDIO_TYPE;
					break;
				case 1:
					audioType	= MP4_MPEG2_AAC_LC_AUDIO_TYPE;
					break;
				case 2:
					audioType	= MP4_MPEG2_AAC_SSR_AUDIO_TYPE;
					break;
				case 3:
					return false;
				}
			}
			break;
		default:
			return false;
		}

		m_audioId = MP4AddAudioTrack(m_hMp4File, nSamplePerSec, /*1024*/m_nSamplePerFrame, audioType) ;
		if ( m_audioId == MP4_INVALID_TRACK_ID )
			return false;

		if( MP4GetNumberOfTracks(m_hMp4File, MP4_AUDIO_TRACK_TYPE) == 1
			&& !MP4SetAudioProfileLevel(m_hMp4File, /*0x0F*/nProfile+1)//Noted by cwl,2014.6.6
			)
		{
			//Noted by cwl,2014.6.6
			//MP4DeleteTrack(m_hMp4File, m_audioId);
			//return false;
		}

		u_int8_t*	pConfig(NULL);
		u_int32_t	configLength(0);

		MP4AV_AacGetConfiguration( &pConfig, &configLength, nProfile, nSamplePerSec, nChannelConfig );
		
		if( !MP4SetTrackESConfiguration( m_hMp4File, m_audioId, pConfig, configLength) ) 
		{
			MP4DeleteTrack(m_hMp4File, m_audioId);
			if(pConfig!=NULL){free(pConfig);}
			return false;
		}
		m_isHaveAddAAC = true;
		if(pConfig!=NULL){free(pConfig);}
	}
	
	m_curAudioTimeStamp += (1024*m_nTimeScale)/m_nSamplePerSec;
	return MP4WriteSample( m_hMp4File, m_audioId, pData+7, size-7,m_curAudioTimeStamp/*, m_nTimeScale / m_nFrameRate ,m_nTimeScale / m_nFrameRate,1*/);

}

bool CMp4FmtInterface::GetFirstHeader(const BYTE* const pData, const BYTE* const end, u_int8_t firstHeader[])
{
	if(firstHeader[0] == 0xff)
		return true;
	
	const BYTE*	pInBuf = pData ;
	u_int	state(0);
	u_int	dropped(0);
	u_int	hdrByteSize(ADTS_HEADER_MAX_SIZE);

	while (1) 
	{
		u_int8_t b(0);

		if(pInBuf == end)
			return false;
		
		b = *pInBuf ++;
		if(state == hdrByteSize - 1) 
		{
			firstHeader[state]	= b;
			return true;
		}

		if (state >= 2) 
		{
			firstHeader[state++] = b;
		} 
		else 
		{
			if (state == 1) 
			{
				if ((b & 0xF6) == 0xF0)
				{
					firstHeader[state]	= b;
					state	= 2;
					hdrByteSize	= MP4AV_AdtsGetHeaderByteSize(firstHeader);
				} 
				else 
					state = 0;
			}
			if (state == 0) 
			{
				if (b == 0xFF) 
				{
					firstHeader[state] = b;
					state = 1;
				} 
				else 
					dropped++;
			}
		}
	}

	return false;
}


/*static*/ bool CMp4FmtInterface::GetMP4FileInfo(const char* strFile, MP4_TRACKINFO_LIST& lsTrackInfo )
{
	return CMP4Analyze::GetMP4FileInfo(strFile,lsTrackInfo );
}

u_int16_t CMp4FmtInterface::GetVideoWidth()
{
	return m_nWidth;
}

u_int16_t CMp4FmtInterface::GetVideoHeight()
{
	return m_nHeight;
}

double CMp4FmtInterface::GetVideoFrameRate()
{
	return m_nFrameRate;
}

u_int32_t CMp4FmtInterface::GetFramesCount()
{
	return m_nFrames;
}

u_int32_t CMp4FmtInterface::GetTimeScale()
{
	return m_nTimeScale;
}

int CMp4FmtInterface::GetAudioSamplePerSec()
{
	return m_nSamplePerSec;
}

int CMp4FmtInterface::GetAudioSamplePerFrame()
{
	return m_nSamplePerFrame;
}

int CMp4FmtInterface::GetAudioChannels()
{
	return m_nAudioChannels;
}

unsigned short CMp4FmtInterface::GetAudioFormatTag()
{
	return m_nAudioFmtTag;
}

unsigned short CMp4FmtInterface::GetAudioBitsPerSample()
{
	return m_nAudioBitsPerSample;
}

bool CMp4FmtInterface::ReadVideoFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameSize,int& nFrameType)
{
	if(m_nOpenModel!=OPEN_MODEL_R)
	{
		return false;
	}

	return (m_pVideoReader->GetFrame( dwFrmNo, pFrame,frmBufLenth ,pFrameSize, nFrameType)==0)?false:true;
}

bool CMp4FmtInterface::ReadAudioFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameSize,int& nFrameType)
{
	if(m_nOpenModel!=OPEN_MODEL_R)
	{
		return false;
	}
	return (m_pAudioReader->GetFrame( dwFrmNo, pFrame,frmBufLenth, pFrameSize, nFrameType )==0)?false:true;
}


void CMp4FmtInterface::AssignMp4Info()
{
	if(m_nOpenModel != OPEN_MODEL_R)
	{
		return;
	}
	MP4_TRACK_INFO videoTrackInfo,audioTrackInfo;

	for(int i=0;i<m_lsTrackInfo.nTrackListNum;i++)
	{
		if(m_lsTrackInfo.stuTrackList[i].dwTrackType==MP4_TRACKTYPE_VIDEO)
		{
			videoTrackInfo = m_lsTrackInfo.stuTrackList[i];
			m_nWidth = videoTrackInfo.stVideoBmpInfo.biWidth;
			m_nHeight = videoTrackInfo.stVideoBmpInfo.biHeight;
			m_nFrameRate = videoTrackInfo.dFps;
			m_nFrames = videoTrackInfo.dwFrameCount;
			//m_nTimeScale = videoTrackInfo.stMp4GOPInfo;
			m_videoId = videoTrackInfo.dwTrackID;
		}

		if(m_lsTrackInfo.stuTrackList[i].dwTrackType==MP4_TRACKTYPE_AUDIO)
		{
			audioTrackInfo = m_lsTrackInfo.stuTrackList[i];
			m_nSamplePerSec = audioTrackInfo.stAudioInfo.nSamplesPerSec;
			m_nSamplePerFrame = audioTrackInfo.stAudioInfo.cbSize;
			m_nAudioChannels = audioTrackInfo.stAudioInfo.nChannels;
			m_nAudioFmtTag = audioTrackInfo.stAudioInfo.wFormatTag;
			m_nAudioBitsPerSample = audioTrackInfo.stAudioInfo.wBitsPerSample;
			m_audioId = audioTrackInfo.dwTrackID;
		}
	}

	return;
}