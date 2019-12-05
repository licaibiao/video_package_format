#include "MP4Reader.h"
#include "H264Reader.h"
#include "MPEG4Reader.h"
#include "AACReader.h"

CMP4Reader* CMP4Reader::CreateReader(const DWORD dwMediaType)
{
	switch( dwMediaType )
	{
	case MP4_VIDEOTYPE_H264:
		return new CH264Reader();
	case MP4_VIDEOTYPE_MPEG4:
		return new CMPEG4Reader();
	case MP4_AUDIOTYPE_MP4:
		return new CAACReader();
	default:
		return NULL;
	}
}

void CMP4Reader::DeleteReader(CMP4Reader* pReader)
{
	if( pReader )
		delete pReader;
}

bool CMP4Reader::getTrackID(int searchCon,MP4_TRACKINFO_LIST lsTrackInfo,DWORD &trackID)
{
	bool bFound = false;
	for(int i=0;i<lsTrackInfo.nTrackListNum;i++)
	{
		if(lsTrackInfo.stuTrackList[i].dwTrackType==MP4_TRACKTYPE_VIDEO && searchCon ==1)
		{
			trackID = lsTrackInfo.stuTrackList[i].dwTrackID;
			bFound = true;
			break;
		}

		if(lsTrackInfo.stuTrackList[i].dwTrackType==MP4_TRACKTYPE_AUDIO && searchCon ==2)
		{
			trackID = lsTrackInfo.stuTrackList[i].dwTrackID;
			bFound = true;
			break;
		}
	}
	return bFound;
}

CMP4Reader::CMP4Reader()
{

}

CMP4Reader::~CMP4Reader()
{

}