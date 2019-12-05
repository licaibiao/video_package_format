#ifndef __MP4_WRITE_IMPL__
#define __MP4_WRITE_IMPL__


#include "MP4Writer.h"
#include "mp4.h"
#include <map>
#include "Utls.h"

class CMP4WriteImpl : public CMP4Writer
{
public:
	CMP4WriteImpl(void);
	virtual ~CMP4WriteImpl(void);

public:
	BOOL Initialize();

	BOOL SetOutputFile( const char* lpszMp4File);
	
	BOOL AddMPEG4Track(int& nTrackId, 
		const int nWidth, 
		const int nHeight, 
		const double fps
		);
	
	BOOL AddH264Track(int& nTrackId,
		const int nWidth, 
		const int nHeight, 
		const double fps
		);
	
	BOOL AddAACTrack(int& nTrackId, 
		const int nSamplePerSec,
		const int nSamplePerFrame = -1
		);
	
	BOOL WriteFrame(const int nTrackId, 
		const BYTE* 
		const lpData, 
		const int nSize, 
		const MP4Timestamp nTimestamp,
		const MP4Duration nDuration = -1
		);

	BOOL StartWrite();
	BOOL StopWrite();

protected:
	struct TrackInfo
	{
		enum{ Created = 0, Pending = 1};
		TrackInfo() : status(Pending)
			, trackid(0), tracktype(0), editunit(1), timescale(1)
		{
		}
		TrackInfo& operator=(const TrackInfo& src)
		{
			status	= src.status;
			trackid	= src.trackid;
			tracktype = src.tracktype;
			editunit  = src.editunit;
			timescale = src.timescale;
		}

		int	  status;
		DWORD tracktype;
		int	  editunit;
		int	  timescale;
		MP4TrackId	trackid;
	};

protected:
	void Uninitialize();
	void CalcEditunit(double fps, int& editunit, int& timescale);
	BOOL CreatePendingTrack(const BYTE* const data, const int size, TrackInfo& track);
	BOOL CreateAACTrack(const BYTE* const data, const int size, TrackInfo& track);

protected:
	bool GetFirstHeader(const BYTE* const data, const BYTE* const end, u_int8_t firstHeader[]);

private:
	char m_strMP4File[255];
	MP4FileHandle	m_hMP4File;
	std::map<int, TrackInfo>m_mapIdx2Track;
};

#endif//__MP4_WRITE_IMPL__