//#include "stdafx.h"
#include "MP4Analyze.h"
#include "mp4.h"
#include "mp4av_aac.h"
#include "mp4util.h"

bool CMP4Analyze::GetMP4FileInfo(const char* strFile, MP4_TRACKINFO_LIST& lsTrackInfo, char* pstrDesc , const int szDesc )
{
	char filepath[/*_MAX_PATH*/260];
	MP4FileHandle	fh = NULL;

	strcpy(filepath, strFile);		//wcstombs(filepath, strFile, _MAX_PATH);
	fh	= MP4Read (filepath);

	if(fh == NULL)
		return FALSE;

	u_int32_t numTracks = MP4GetNumberOfTracks(fh);
	for (u_int32_t i = 0; i < numTracks; i++) 
	{
		MP4TrackId	trackId(MP4FindTrackId(fh, i));
		const char*	trackType(MP4GetTrackType(fh, trackId));

		if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) 
		{
			lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum].dwTrackID	= trackId;
			lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum].dwTrackType	= MP4_TRACKTYPE_AUDIO;

			GetAudioTrackInfo(fh, trackId, lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum]);

			lsTrackInfo.nTrackListNum++;
		} 
		else if ( !strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) 
		{
			lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum].dwTrackID	= trackId;
			lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum].dwTrackType	= MP4_TRACKTYPE_VIDEO;

			GetVideoTrackInfo(fh, trackId, lsTrackInfo.stuTrackList[lsTrackInfo.nTrackListNum]);

			lsTrackInfo.nTrackListNum++;
		}
	}

	if ( pstrDesc )
	{
		char* pInfo = MP4Info(fh) ;
		strncpy(pstrDesc, pInfo, szDesc - 1);
		MP4Free(pInfo);
	}

	MP4Close(fh);
	return TRUE;
}

bool CMP4Analyze::GetAudioTrackInfo(void* fh, int trackId, MP4_TRACK_INFO& info)
{
	u_int8_t type(0);
	const char* media_data_name;

	media_data_name	= MP4GetTrackMediaDataName(fh, trackId);
	if (media_data_name == NULL) 
	{
		info.dwAudioType = MP4_AUDIOTYPE_UNKNOWN;
		return FALSE;
	} 
	else if (strcasecmp(media_data_name, "samr") == 0) 
	{
		info.dwAudioType = MP4_AUDIOTYPE_AMR;
	} 
	else if (strcasecmp(media_data_name, "sawb") == 0) 
	{
		info.dwAudioType= MP4_AUDIOTYPE_AMRWB;
	} 
	else if (strcasecmp(media_data_name, "mp4a") == 0) 
	{
		type  = MP4GetTrackEsdsObjectTypeId(fh, trackId);
		info.dwAudioType 	= MP4_AUDIOTYPE_MP4;

		u_int8_t*  pConfig(NULL);
		u_int32_t  configLength;

		MP4GetTrackESConfiguration(fh, trackId, &pConfig, &configLength);
		if (pConfig == NULL || configLength < 2 || configLength > 5)
			return 0;

		info.stAudioInfo.nSamplesPerSec	= MP4AV_AacConfigGetSamplingRate(pConfig);
		info.stAudioInfo.nChannels		= 2;//MP4AV_AacConfigGetChannels(pConfig);
		
		//  需要修改
		info.stAudioInfo.wBitsPerSample	= 16;
		info.stAudioInfo.nBlockAlign	= info.stAudioInfo.wBitsPerSample * info.stAudioInfo.nChannels / 8;
		info.stAudioInfo.nAvgBytesPerSec= info.stAudioInfo.nBlockAlign * info.stAudioInfo.nSamplesPerSec;
		info.stAudioInfo.wFormatTag		= WAVE_FORMAT_AAC;
		
		int  nSampleCount = MP4GetTrackNumberOfSamples(fh, trackId);	//cal audio duration,ms
		int  nSamplePerFrm = MP4AV_AacConfigGetSamplingWindow(pConfig);
		
		info.dAudioDuration	= 1.0 * nSampleCount * nSamplePerFrm / info.stAudioInfo.nSamplesPerSec * 1000;
	} 
	else 
	{
		info.dwAudioType	= MP4_AUDIOTYPE_UNKNOWN;
		return FALSE;
	}

	return TRUE;
}

bool CMP4Analyze::GetVideoTrackInfo(void* fh, int trackId, MP4_TRACK_INFO& info)
{
	const char* media_data_name;
	uint8_t type(0);
	char 	typebuffer[80];

	media_data_name	= MP4GetTrackMediaDataName(fh, trackId);
	
	if (media_data_name == NULL) 
	{
		info.dwVideoType = MP4_VIDEOTYPE_UNKNOWN;
		return FALSE;
	} 
	else if (strcasecmp(media_data_name, "avc1") == 0) 
	{
		// avc
		uint8_t profile;
		uint8_t level;
		char profileb[20];
		char levelb[20];

		if (MP4GetTrackH264ProfileLevel(fh, trackId, &profile, &level)) 
		{
			if (profile == 66) 
			{
				strcpy(profileb, "Baseline");
			} else if (profile == 77) 
			{
				strcpy(profileb, "Main");
			} else if (profile == 88)
			{
				strcpy(profileb, "Extended");
			} else 
			{
				sprintf(profileb, "Unknown Profile %x", profile);
			} 
			switch (level) 
			{
			case 10: 
			case 20: 
			case 30: 
			case 40: 
			case 50:
				sprintf(levelb, "%u", level / 10);
				break;
			case 11: 
			case 12: 
			case 13:
			case 21: 
			case 22:
			case 31: 
			case 32:
			case 41: 
			case 42:
			case 51:
				sprintf(levelb, "%u.%u", level / 10, level % 10);
				break;
			default:
				sprintf(levelb, "unknown level %x", level);
				break;
			}
			sprintf(typebuffer, "H264 %s@%s", profileb, levelb);
		} 
		else 
		{
		}

		info.dwVideoType= MP4_VIDEOTYPE_H264;
		info.stVideoBmpInfo.biCompression = '462H';
	} 
	else if (strcasecmp(media_data_name, "s263") == 0)
	{
		// 3gp h.263
		info.dwVideoType= MP4_VIDEOTYPE_H263;
		info.stVideoBmpInfo.biCompression= '362H';
	} 
	else if ((strcasecmp(media_data_name, "mp4v") == 0) || (strcasecmp(media_data_name, "encv") == 0) )
	{
		// note encv might needs it's own field eventually.
		type = MP4GetTrackEsdsObjectTypeId(fh, trackId);
		if (type == MP4_MPEG4_VIDEO_TYPE) 
			type = MP4GetVideoProfileLevel(fh);

		info.dwVideoType = MP4_VIDEOTYPE_MPEG4;
	} 
	else 
	{
		info.dwVideoType = MP4_VIDEOTYPE_UNKNOWN;
		return FALSE;
	}

	info.stVideoBmpInfo.biPlanes	= 1;
	info.stVideoBmpInfo.biBitCount	= 24;
	info.dwFrameCount				= MP4GetTrackNumberOfSamples(fh, trackId);
	info.stVideoBmpInfo.biSize		= sizeof(info.stVideoBmpInfo);
	info.stVideoBmpInfo.biWidth		= MP4GetTrackVideoWidth(fh, trackId);
	info.stVideoBmpInfo.biHeight	= MP4GetTrackVideoHeight(fh, trackId);

	info.stVideoBmpInfo.biSizeImage	= info.stVideoBmpInfo.biWidth * info.stVideoBmpInfo.biHeight * info.stVideoBmpInfo.biBitCount / 8;

	//m_stTrackInfo.dFps = MP4GetTrackVideoFrameRate(m_hFile,m_nTrackID);
	//开源代码中存在精度丢失，在外部计算帧率
	u_int32_t	nTimeScale = MP4GetTrackTimeScale(fh, trackId);
	MP4Duration	nDur = MP4GetTrackDuration(fh, trackId) ;
	info.dFps	= 1.0 * info.dwFrameCount * nTimeScale / nDur;

	return TRUE;
}
