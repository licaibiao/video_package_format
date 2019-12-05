#ifndef __MP4__FORMAT_INTERFACE_H__
#define __MP4__FORMAT_INTERFACE_H__
#include "MP4Writer.h"
#include "MP4Reader.h"

enum FILE_MP4_RESULT
{
	FILE_MP4_FAIL = 0,     //ʧ��
	FILE_MP4_SUCCESS = 1	// �ɹ�
};

// NALU��Ԫ
typedef struct _MP4ENC_NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}MP4ENC_NaluUnit;

enum FILE_OPEN_MODEL
{
	OPEN_MODEL_W,	// д�ļ�
	OPEN_MODEL_R,	// ���ļ� r+b
	OPEN_MODEL_NA =0xff  // δ��ʼ��
};

class CMp4FmtInterface
{
public:
	CMp4FmtInterface();
	~CMp4FmtInterface();

	/*
	 *	@mark ���ļ� 
	 *	@param[in] nModel�ļ��򿪷�ʽ�����nModelΪOPEN_MODEL_W �򴴽��ļ�
	 *  @param[in] nMediaType ֵΪ MP4_VIDEOTYPE_H264/MP4_VIDEOTYPE_MPEG4
	 *  @return �������
	 */
	bool OpenFile(const char* strPath, FILE_OPEN_MODEL nModel); 
	
	/*
	 *	@Name:Close
	 *  @mark: ��д��������֮�󣬱�����ô˺���
	 */
	bool Close();

public://д����
	/*
	 *	@mark: ��ʼ��mp4�ļ�
	 *  @param[in] width ��Ƶ���
	 *  @param[in] height  ��Ƶ�߶�
	 *  @param[in] frameRate  ��Ƶ֡��
	 *  @param[in] timeScale  ʱ��̶ȣ���������ָ����ֵԽ�߲��ŵ��ٶ�Խ��ȷ
	 *  @param[in] sampleLenFieldSizeMinusOne  ��Ƶ����д��ĸ�ʽ�Ƿ���г��ȱ�ʶ
	 *  @param[in] nSamplePerSec ��Ƶ����Ƶ��
	 *  @param[in] nSamplePerFrame ��Ƶ������
	 */
	bool SetMp4Param(
			u_int16_t width,
			u_int16_t height,
			double frameRate, 
			u_int32_t timeScale = 90000, 
			int nSamplePerSec = 0,
			int nSamplePerFrame = 1024);

	/*
	 *	@mark: дһ֡��Ƶ����
	 *  @param[in] pData Ҫд�������
	 *  @param[in] size  Ҫд������ݴ�С
	 */
	bool WriteVideoFrameData(const unsigned char* pFrameData,int size);

	bool WriteAudioFrameData(const unsigned char* pData,int size);

public://������
	/*
	 *	@mark: ��ȡ��Ƶ���
	 */
	u_int16_t GetVideoWidth();

	/*
	 *	@mark: ��ȡ��Ƶ�߶�
	 */
	u_int16_t GetVideoHeight();

	/*
	 *	@mark: ��ȡ��Ƶ֡��
	 */
	double GetVideoFrameRate();

	/*
	 *	@mark: ��ȡʱ��̶�
	 */
	u_int32_t GetTimeScale();
	
	/*
	 *	@mark: ��ȡ��Ƶ֡��Ŀ
	 */
	u_int32_t GetFramesCount();

	/*
	 *	@mark: ��ȡ����Ƶ��
	 */
	int GetAudioSamplePerSec();

	/*
	 *	@mark: ��ȡÿһ֡��������Ĭ��ֵ1024
	 */
	int GetAudioSamplePerFrame();

	/*
	 *	@mark: ��ȡ������
	 */
	int GetAudioChannels();

	/*
	 *	@mark: ��ȡ��Ƶ����
	 */
	unsigned short GetAudioFormatTag();

	/*
	 *	@mark: ��ȡ��Ƶÿ����������ռ������
	 */
	unsigned short GetAudioBitsPerSample();


	/*
	 *	@mark: ��ȡһ֡����Ƶ����
	 *  @param[in] dwFrmNo  Ҫ������֡�����к�(���к��Ǵ��㿪ʼ������)
	 *  @param[out] ppFrame ���ڶ�������Ƶ���ݴ�ŵ��ڴ��ַ
	 *  @param[int] frmBufLenth ��ʶ���ڴ�����ݵĻ��泤��
	 *  @param[out] pFrameBufSize  ������֡�Ĵ�С
	 *  @param[out] nFrameType  ������֡�����ͣ�Ĭ��Ϊ0��0��ͨ֡��1�ؼ�֡
	 */
	bool ReadVideoFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth,DWORD* pFrameSize,int& nFrameType);

	bool ReadAudioFrameData(DWORD dwFrmNo, BYTE* pFrame,DWORD frmBufLenth,DWORD* pFrameSize,int& nFrameType);

private:
	/*	@mark : ȫ�ֺ�������ȡ����MP4�ļ��ṹ��Ϣ��
	 *	�����ڲ���fopen/fclose���ú���Ӧ����OpenFile֮ǰ����
	 *  @param[in] strFile �ļ�·��
	 *  @param[out] lsTrackInfo ����ļ���Ϣ�ṹ
	 */
	static bool  GetMP4FileInfo( const char* strFile, MP4_TRACKINFO_LIST& lsTrackInfo );

	/*
	 *	@mark: д��mp4��ʽ����
	 */
	int WriteSampleData(const unsigned char*smapleData,int datalen,bool bIsSyncSample);

	/*
	 *	@mark: ��һ�������н�����һ��NAL����
	 */
	int ReadOneNaluFromBuf(unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu);

	/*
	 *	@mark: ��NALU��Ԫ�ָ���ʶ��00000001����дΪ����size
	 */
	void SetFrameDataLenth(unsigned char* pNaluDataStart,int size);

	/*
	 *	@mark: ��pData��end������ָ����������ݣ�������aac����ͷ
	 *  @param[in] pData ��ʼָ��
	 *  @param[in] end ����ָ��
	 *  @param[out] firstHeader ��������AAC����ͷ
	 */
	bool GetFirstHeader(const BYTE* const pData, const BYTE* const end, u_int8_t firstHeader[]);

	/*
	 *	@mark: ��ȡMP4�ļ���Ϣ
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

	unsigned char* m_pVideoFrameBuf;     //��ʱ�洢��ǰҪд���һ֡������
	unsigned long  m_maxVideoFrameBufLen;  //���һ֡���ݵ����ݴ�С
};
#endif