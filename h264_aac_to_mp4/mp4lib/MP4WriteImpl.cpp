//#include "StdAfx.h"
#include "MP4WriteImpl.h"
#include <math.h>
#include "mp4av_adts.h"
#include "mp4av_aac.h"

#define ADTS_HEADER_MAX_SIZE  10

CMP4WriteImpl::CMP4WriteImpl(void)
	:m_hMP4File(NULL)
{
	memset( m_strMP4File, 0, 255);
}

CMP4WriteImpl::~CMP4WriteImpl(void)
{
	Uninitialize();
}

void CMP4WriteImpl::Uninitialize()
{
	if( m_hMP4File )
		MP4Close(m_hMP4File);

	m_hMP4File	= NULL;
}

BOOL CMP4WriteImpl::Initialize()
{
	Uninitialize();

	return TRUE;
}

BOOL CMP4WriteImpl::SetOutputFile( const char* szMp4File)
{
	if( m_hMP4File != NULL )
		return FALSE;

	int	nRet = 0;
	strcpy( m_strMP4File, szMp4File );

	m_hMP4File	= MP4CreateEx( szMp4File );
	if(m_hMP4File == NULL)
		return FALSE;

	m_mapIdx2Track.clear();

	return TRUE;
}

BOOL CMP4WriteImpl::AddMPEG4Track(int& nTrackId, const int nWidth,const int nHeight,const double fps )
{
	TrackInfo	info;
	mp4v2_ismacrypParams params;

	memset(&params, 0, sizeof(params));
	CalcEditunit(fps, info.editunit, info.timescale);
	info.status		= TrackInfo::Created;
	info.tracktype	= MP4_MPEG4_VIDEO_TYPE;
	info.trackid	= MP4AddEncVideoTrack(m_hMP4File, info.timescale, info.editunit, nWidth, nHeight, &params);
	if(info.trackid == MP4_INVALID_TRACK_ID)return FALSE;

	nTrackId = m_mapIdx2Track.size();
	m_mapIdx2Track.insert(std::make_pair(nTrackId, info));
	return TRUE;
}

//h264
BOOL CMP4WriteImpl::AddH264Track(int& nTrackId, const int nWidth,const int nHeight,const double fps )
{
	TrackInfo	info;

	CalcEditunit(fps, info.editunit, info.timescale);
	info.status		= TrackInfo::Created;
	info.tracktype	= MP4_MPEG4_VIDEO_TYPE;
	info.trackid = MP4AddH264VideoTrack(m_hMP4File, info.timescale, info.editunit, nWidth, nHeight, 0, 0, 0, 0);
	
	if( info.trackid == MP4_INVALID_TRACK_ID )
		return FALSE;
		
	nTrackId = m_mapIdx2Track.size();
	m_mapIdx2Track.insert( std::make_pair(nTrackId, info) );
	return TRUE;
}

BOOL CMP4WriteImpl::AddAACTrack(int& nTrackId, const int nSamplePerSec, const int nSamplePerFrame )
{
	TrackInfo info;

	info.editunit	= 1;
	info.timescale	= nSamplePerSec;
	info.status		= TrackInfo::Pending;
	info.tracktype	= MP4_MPEG4_AUDIO_TYPE;
	
	nTrackId = m_mapIdx2Track.size();
	m_mapIdx2Track.insert(std::make_pair(nTrackId, info));
	
	return TRUE;
}

BOOL CMP4WriteImpl::StartWrite()
{
	if( m_hMP4File == NULL)
		return FALSE;

	if(m_mapIdx2Track.size() < 1)
		return FALSE;

	return TRUE;
}

BOOL CMP4WriteImpl::WriteFrame(const int nTrackId, 
	const BYTE* const lpData, 
	const int nSize,
	MP4Timestamp nTimestamp,
	const MP4Duration nDuration 
	)
{
	if( m_hMP4File == NULL)
		return FALSE;

	std::map<int, TrackInfo>::iterator it = m_mapIdx2Track.find(nTrackId);
	bool bResult(FALSE);

	if( it == m_mapIdx2Track.end())
		return FALSE;

	if( it->second.status == TrackInfo::Pending && !CreatePendingTrack(lpData, nSize, it->second) )
		return FALSE;
	
	bResult	= MP4WriteSample( m_hMP4File, it->second.trackid, lpData, nSize,nTimestamp, nDuration );
	
	if(!bResult)
		return FALSE;

	return TRUE;
}

BOOL CMP4WriteImpl::CreatePendingTrack(const BYTE* const pData, const int size, TrackInfo& track)
{
	switch(track.tracktype)
	{
	case MP4_MPEG4_AUDIO_TYPE:
		return CreateAACTrack(pData, size, track);
	default:
		return FALSE;
	}

	return TRUE;
}


bool CMP4WriteImpl::GetFirstHeader(const BYTE* const pData, const BYTE* const end, u_int8_t firstHeader[])
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


BOOL CMP4WriteImpl::CreateAACTrack(const BYTE* const pData, const int size, TrackInfo& track)
{
	u_int32_t	nSamplePerSec =0;
	u_int8_t	nMpegVersion=0;
	u_int8_t	nProfile = 0;
	u_int8_t	nChannelConfig= 0;
	u_int8_t	firstHeader[ADTS_HEADER_MAX_SIZE];

	if ( !GetFirstHeader(pData, pData + size, firstHeader))
		return FALSE;

	nSamplePerSec = MP4AV_AdtsGetSamplingRate(firstHeader);
	nMpegVersion  = MP4AV_AdtsGetVersion(firstHeader);
	nProfile	  = MP4AV_AdtsGetProfile(firstHeader);
	nChannelConfig= MP4AV_AdtsGetChannels(firstHeader);

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
				return FALSE;
			}
		}
		break;
	default:
		return FALSE;
	}

	MP4TrackId	nTrackId = MP4AddAudioTrack(m_hMP4File, nSamplePerSec, 1024, audioType) ;
	if ( nTrackId == MP4_INVALID_TRACK_ID )
		return FALSE;

	if( MP4GetNumberOfTracks(m_hMP4File, MP4_AUDIO_TRACK_TYPE) == 1
		&& !MP4SetAudioProfileLevel(m_hMP4File, 0x0F)
		)
	{
		MP4DeleteTrack(m_hMP4File, nTrackId);
		return FALSE;
	}

	u_int8_t*	pConfig(NULL);
	u_int32_t	configLength(0);

	MP4AV_AacGetConfiguration( &pConfig, &configLength, nProfile, nSamplePerSec, nChannelConfig );
	
	if( !MP4SetTrackESConfiguration( m_hMP4File, nTrackId, pConfig, configLength) ) 
	{
		MP4DeleteTrack(m_hMP4File, nTrackId);
		return FALSE;
	}

	track.trackid	= nTrackId;
	track.status	= TrackInfo::Created;

	return TRUE;
}

BOOL CMP4WriteImpl::StopWrite()
{
	Uninitialize();

	return TRUE;
}

void CMP4WriteImpl::CalcEditunit( double fps, int& editunit, int& timescale)
{
	if( fabs(fps - 15) < 0.01)
	{
		editunit	= 1;
		timescale	= 15;
	}
	else if( fabs(fps - 30) < 0.01)
	{
		editunit	= 1;
		timescale	= 30;
	}
	else if(fabs(fps - 25) < 0.01)
	{
		editunit	= 1;
		timescale	= 25;
	}
	else
	{
		editunit	= 10000;
		timescale	= (int)(fps * 10000);
	}
}
