#ifndef __MP4__FORMAT_INTERFACE_H__
#define __MP4__FORMAT_INTERFACE_H__
#include "MP4Writer.h"
#include "MP4Reader.h"

enum FILE_MP4_RESULT
{
	FILE_MP4_FAIL = 0,     //失败
	FILE_MP4_SUCCESS = 1	// 成功
};

// NALU单元
typedef struct _MP4ENC_NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}MP4ENC_NaluUnit;

enum FILE_OPEN_MODEL
{
	OPEN_MODEL_W,	// 写文件
	OPEN_MODEL_R,	// 读文件 r+b
	OPEN_MODEL_NA =0xff  // 未初始化
};

class CMp4FmtInterface
{
public:
	CMp4FmtInterface();
	~CMp4FmtInterface();

	/*
	 *	@mark 打开文件 
	 *	@param[in] nModel文件打开方式，如果nModel为OPEN_MODEL_W 则创建文件
	 *  @param[in] nMediaType 值为 MP4_VIDEOTYPE_H264/MP4_VIDEOTYPE_MPEG4
	 *  @return 操作结果
	 */
	bool OpenFile(const char* strPath, FILE_OPEN_MODEL nModel); 
	
	/*
	 *	@Name:Close
	 *  @mark: 读写操作结束之后，必须调用此函数
	 */
	bool Close();

public://写操作
	/*
	 *	@mark: 初始化mp4文件
	 *  @param[in] width 视频宽度
	 *  @param[in] height  视频高度
	 *  @param[in] frameRate  视频帧率
	 *  @param[in] timeScale  时间刻度，可以任意指定，值越高播放的速度越精确
	 *  @param[in] sampleLenFieldSizeMinusOne  视频数据写入的格式是否具有长度标识
	 *  @param[in] nSamplePerSec 音频采样频率
	 *  @param[in] nSamplePerFrame 音频比特率
	 */
	bool SetMp4Param(
			u_int16_t width,
			u_int16_t height,
			double frameRate, 
			u_int32_t timeScale = 90000, 
			int nSamplePerSec = 0,
			int nSamplePerFrame = 1024);

	/*
	 *	@mark: 写一帧视频数据
	 *  @param[in] pData 要写入的数据
	 *  @param[in] size  要写入的数据大小
	 */
	bool WriteVideoFrameData(const unsigned char* pFrameData,int size);

	bool WriteAudioFrameData(const unsigned char* pData,int size);

public://读操作
	/*
	 *	@mark: 获取视频宽度
	 */
	u_int16_t GetVideoWidth();

	/*
	 *	@mark: 获取视频高度
	 */
	u_int16_t GetVideoHeight();

	/*
	 *	@mark: 获取视频帧率
	 */
	double GetVideoFrameRate();

	/*
	 *	@mark: 获取时间刻度
	 */
	u_int32_t GetTimeScale();
	
	/*
	 *	@mark: 获取视频帧数目
	 */
	u_int32_t GetFramesCount();

	/*
	 *	@mark: 获取采样频率
	 */
	int GetAudioSamplePerSec();

	/*
	 *	@mark: 获取每一帧采样数，默认值1024
	 */
	int GetAudioSamplePerFrame();

	/*
	 *	@mark: 获取声道数
	 */
	int GetAudioChannels();

	/*
	 *	@mark: 获取音频类型
	 */
	unsigned short GetAudioFormatTag();

	/*
	 *	@mark: 获取音频每个采样点所占比特数
	 */
	unsigned short GetAudioBitsPerSample();


	/*
	 *	@mark: 读取一帧音视频数据
	 *  @param[in] dwFrmNo  要读出的帧的序列号(序列号是从零开始计数的)
	 *  @param[out] ppFrame 用于读出音视频数据存放的内存地址
	 *  @param[int] frmBufLenth 标识用于存放数据的缓存长度
	 *  @param[out] pFrameBufSize  读出的帧的大小
	 *  @param[out] nFrameType  读出的帧的类型，默认为0；0普通帧，1关键帧
	 */
	bool ReadVideoFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth,DWORD* pFrameSize,int& nFrameType);

	bool ReadAudioFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth,DWORD* pFrameSize,int& nFrameType);

private:
	/*	@mark : 全局函数，读取整个MP4文件结构信息，
	 *	函数内部会fopen/fclose，该函数应当在OpenFile之前调用
	 *  @param[in] strFile 文件路径
	 *  @param[out] lsTrackInfo 输出文件信息结构
	 */
	static bool  GetMP4FileInfo( const char* strFile, MP4_TRACKINFO_LIST& lsTrackInfo );

	/*
	 *	@mark: 写入mp4格式数据
	 */
	int WriteSampleData(const unsigned char*smapleData,int datalen,bool bIsSyncSample);

	/*
	 *	@mark: 从一段数据中解析出一个NAL盒子
	 */
	int ReadOneNaluFromBuf(unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu);

	/*
	 *	@mark: 将NALU单元分隔标识（00000001），写为长度size
	 */
	void SetFrameDataLenth(unsigned char* pNaluDataStart,int size);

	/*
	 *	@mark: 从pData到end这两个指针间的这段数据，解析出aac数据头
	 *  @param[in] pData 起始指针
	 *  @param[in] end 结束指针
	 *  @param[out] firstHeader 解析到的AAC数据头
	 */
	bool GetFirstHeader(const BYTE* const pData, const BYTE* const end, u_int8_t firstHeader[]);

	/*
	 *	@mark: 获取MP4文件信息
	 */
	void AssignMp4Info();
private:
	u_int16_t m_nWidth;
	u_int16_t m_nHeight;
	double m_nFrameRate;
	u_int32_t m_nFrames;
	int m_nTimeScale;
	int m_sampleLenFieldSizeMinusOne;
	int m_nSamplePerSec;
	int m_nSamplePerFrame;
	int m_nAudioChannels;
	unsigned short m_nAudioFmtTag;
	unsigned short m_nAudioBitsPerSample;


	MP4TrackId m_videoId;
	MP4TrackId m_audioId;
	MP4FileHandle m_hMp4File;

	MP4Timestamp m_curVideoTimeStamp;
	MP4Timestamp m_curAudioTimeStamp;

	int m_nOpenModel;

	//wirte
	CMP4Writer * m_pVideoWriter;
	CMP4Writer * m_pAudioWriter;

	//read
	CMP4Reader * m_pVideoReader;
	CMP4Reader * m_pAudioReader;

	bool m_isHaveAddSPS;
	bool m_isHaveAddPPS;

	bool m_isHaveAddAAC;

	MP4_TRACKINFO_LIST m_lsTrackInfo;

	unsigned char* m_pVideoFrameBuf;     //临时存储当前要写入的一帧的数据
	unsigned long  m_maxVideoFrameBufLen;  //最大一帧数据的数据大小
};
#endif