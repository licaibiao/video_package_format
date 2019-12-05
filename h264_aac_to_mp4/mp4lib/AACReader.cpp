#include "AACReader.h"
#include "mp4av_aac.h"
#include "mp4av_adts.h"

CAACReader::CAACReader(void)
	: m_hFile(NULL)
	, m_wfx()
	, m_nSamplePerFrm(0)
	, m_aacProfileLevel(0)
	, m_nSampleCount(0)
	, m_stTrackInfo()
	, m_nMaxAAcDecBufSize(0)
	, m_bprependADTS(FALSE)

	, m_strMP4File()
	, m_nTrackID(-1)

	, m_pAudioSampleBuf(NULL)
{
}

CAACReader::~CAACReader(void)
{
	UnInitialize();
}

int CAACReader::Initialize(const char* strPath,MP4_TRACKINFO_LIST lsTrackInfo)
{
	//UnInitialize();
	DWORD nTrackID = MP4_INVALID_TRACK_ID;
	SetInputFile(strPath);

	if(!getTrackID(2,lsTrackInfo,nTrackID))
	{
		return 0;
	}
	SetTrackID( nTrackID );

	return StartRead();
	//return 1;
}

int CAACReader::SetInputFile(const char* pMp4File)
{
	strcpy( m_strMP4File, pMp4File);
	return 1;
}

int CAACReader::SetTrackID(UINT nTrackID)
{
	m_nTrackID			= nTrackID;
	return 1;
}

int CAACReader::StartRead()
{
	StopRead();

	int	 nRet(0);
	char file[/*_MAX_PATH*/260];
	//wcstombs(file, m_strMP4File, _MAX_PATH);
	strcpy(file, m_strMP4File);
	m_hFile	= MP4Read(file);
	if(m_hFile == NULL)
		return 0;

	int	nTrackCount(MP4GetNumberOfTracks(m_hFile, MP4_AUDIO_TRACK_TYPE));
	if(nTrackCount <= 0)
		return 0;

	const char*	media_data_name(MP4GetTrackMediaDataName(m_hFile, m_nTrackID));
	if (strcmp(media_data_name, "mp4a") != 0)
		return 0;

	u_int8_t*			pConfig(NULL);
	u_int32_t			configLength;

	MP4GetTrackESConfiguration(m_hFile, m_nTrackID, &pConfig, &configLength);
	if (pConfig == NULL || configLength < 2 || configLength > 5)
		return 0;

	m_wfx.nSamplesPerSec	= MP4AV_AacConfigGetSamplingRate(pConfig);
	m_wfx.nChannels			= 2;//MP4AV_AacConfigGetChannels(pConfig);
	//需要修改
	m_nSamplePerFrm			= MP4AV_AacConfigGetSamplingWindow(pConfig);
	m_aacProfileLevel		= (pConfig[0] >> 3) - 1;
	CHECK_AND_FREE(pConfig);

	m_wfx.wBitsPerSample	= 16;
	m_wfx.nBlockAlign		= m_wfx.wBitsPerSample * m_wfx.nChannels / 8;
	m_wfx.nAvgBytesPerSec	= m_wfx.nBlockAlign * m_wfx.nSamplesPerSec;
	m_wfx.wFormatTag		= WAVE_FORMAT_AAC;
	m_nSampleCount			= MP4GetTrackNumberOfSamples(m_hFile, m_nTrackID);	//cal audio duration,ms

	double	dAudioDuration(1.0 * m_nSampleCount * m_nSamplePerFrm / m_wfx.nSamplesPerSec * 1000);
	double	dFrameRate(1.0 * m_nSamplePerFrm / m_wfx.nSamplesPerSec);

	m_stTrackInfo.stAudioInfo		= m_wfx;
	m_stTrackInfo.dwTrackID			= m_nTrackID;
	m_stTrackInfo.dAudioDuration	= dAudioDuration; //ms
	m_stTrackInfo.dwFrameCount		= m_nSampleCount;
	m_stTrackInfo.dFps				= dFrameRate;
	m_stTrackInfo.dwTrackType		= MP4_TRACKTYPE_AUDIO;
	m_stTrackInfo.dwAudioType		= MP4_AUDIOTYPE_MP4;

	int	nAACFrameSize = MP4GetTrackMaxSampleSize(m_hFile, m_nTrackID) ;

	m_nMaxAAcDecBufSize	= nAACFrameSize * 2;
	/*if(m_pAudioSampleBuf)
	{
		delete[] m_pAudioSampleBuf;
		m_pAudioSampleBuf	= NULL;
	}*/

	//m_pAudioSampleBuf	= new BYTE[m_nMaxAAcDecBufSize];
	m_bprependADTS		= FALSE;

	if (MP4_IS_AAC_AUDIO_TYPE(MP4GetTrackEsdsObjectTypeId(m_hFile, m_nTrackID))) 
		m_bprependADTS	= TRUE;

	return 1;
}

int CAACReader::GetTrackInfo(MP4_TRACK_INFO* pstTrackInfo)
{
	if (pstTrackInfo != NULL)
		memcpy(pstTrackInfo, &m_stTrackInfo, sizeof(m_stTrackInfo));

	return 1;
}

//外部提供的pFrame的空间必须大于m_nMaxAAcDecBufSize，否则就有可能出现写入越界
int CAACReader::GetFrame(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth, DWORD* pFrameBufSize,int& nFrameType)
{
	int	nRet(0);
	int	nAudioSamplSize(0);
	nFrameType = 0;

	if(frmBufLenth<m_nMaxAAcDecBufSize)
	{
		printf("The buf length of read aac frame is too short!\n");
		return 0;
	}

	bool rc(false);
	BYTE* pAudioSample(NULL);

	if(m_bprependADTS) 
	{
		// need some very specialized work for these
		rc =MP4AV_AdtsMakeFrameFromMp4Sample(m_hFile, m_nTrackID, (dwFrmNo+1), m_aacProfileLevel, (u_int8_t**)&pAudioSample, (u_int32_t*)&nAudioSamplSize);
		if(rc && pAudioSample)
		{
			memcpy(pFrame, pAudioSample, nAudioSamplSize);
			nRet					= 1;
		}

		CHECK_AND_FREE(pAudioSample);
		pAudioSample = pFrame;
	}
	else 
	{
		pAudioSample	= pFrame;
		// read the sample
		if (dwFrmNo == 1) 
		{
			u_int8_t*	pConfig(NULL);
			u_int32_t	configSize(0);

			if (MP4GetTrackESConfiguration(m_hFile, m_nTrackID, &pConfig, &configSize)) 
			{
				if (configSize != 0) 
				{
					memcpy(pAudioSample, pConfig, configSize);
					pAudioSample	+= configSize;
					nAudioSamplSize += configSize;
				}
				CHECK_AND_FREE(pConfig);
			}
		}

		int nReadSize = 0;
		rc = MP4ReadSample(m_hFile, m_nTrackID, (dwFrmNo + 1), (u_int8_t**)&pAudioSample, (u_int32_t*)&nReadSize);

		if (rc) 
		{
			nAudioSamplSize += nReadSize;
			nRet = 1;
		}
	}

	//*ppFrame	= m_pAudioSampleBuf;
	*pFrameBufSize	= nAudioSamplSize;

	return (rc ? 1 : 0);
}

int CAACReader::StopRead()
{
	if(m_hFile)
	{
		MP4Close(m_hFile);
		m_hFile	= NULL;
	}
	return TRUE;
}

int CAACReader::UnInitialize()
{
	StopRead();
	/*if(m_pAudioSampleBuf)
	{
		delete[] m_pAudioSampleBuf;
		m_pAudioSampleBuf	= NULL;
	}*/

	return TRUE;
}