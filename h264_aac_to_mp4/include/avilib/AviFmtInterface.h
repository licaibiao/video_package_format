#ifndef __AVI_FORMAT_INTERFACE_H__
#define __AVI_FORMAT_INTERFACE_H__

//视频索引结构体
typedef struct
{
   long pos;
   long len;
   long flag;
} video_index_entry;

//音频索引结构体
typedef struct
{
   long pos;
   long len;
   long flag;
   long tot;
} audio_index_entry;


//存放AVI文件信息的结构体
typedef struct
{
   long   fdes;              /* File descriptor of AVI file */
   long   mode;              /* 0 for reading, 1 for writing */

   long   width;             /* Width  of a video frame */
   long   height;            /* Height of a video frame */
   double fps;               /* Frames per second */
   char   compressor[8];     /* Type of compressor, 4 bytes + padding for 0 byte */
   long   video_strn;        /* Video stream number */
   long   video_frames;      /* Number of video frames */
   char   video_tag[4];      /* Tag of video data */
   long   video_pos;         /* Number of next frame to be read
                                (if index present) */

   long   a_fmt;             /* Audio format, see #defines below */
   long   a_chans;           /* Audio channels, 0 for no audio */
   long   a_rate;            /* Rate in Hz */
   long   a_bits;            /* bits per audio sample */
   long   audio_strn;        /* Audio stream number */
   long   audio_bytes;       /* Total number of bytes of audio data */
   long   audio_chunks;      /* Chunks of audio data in the file */
   char   audio_tag[4];      /* Tag of audio data */
   long   audio_posc;        /* Audio position: chunk */
   long   audio_posb;        /* Audio position: byte within chunk */

   long   pos;               /* position in file */
   long   n_idx;             /* number of index entries actually filled */
   long   max_idx;           /* number of index entries actually allocated */
   unsigned char (*idx)[16]; /* index entries (AVI idx1 tag) */
   video_index_entry * video_index;
   audio_index_entry * audio_index;
   long   last_pos;          /* Position of last frame written */
   long   last_len;          /* Length of last frame written */
   int    must_use_index;    /* Flag if frames are duplicated */
   long   movi_start;
} avi_t;

#define AVI_MODE_WRITE  0
#define AVI_MODE_READ   1

/* The error codes delivered by avi_open_input_file */

#define AVI_ERR_SIZELIM      1     /* The write of the data would exceed
                                      the maximum size of the AVI file.
                                      This is more a warning than an error
                                      since the file may be closed safely */

#define AVI_ERR_OPEN         2     /* Error opening the AVI file - wrong path
                                      name or file nor readable/writable */

#define AVI_ERR_READ         3     /* Error reading from AVI File */

#define AVI_ERR_WRITE        4     /* Error writing to AVI File,
                                      disk full ??? */

#define AVI_ERR_WRITE_INDEX  5     /* Could not write index to AVI file
                                      during close, file may still be
                                      usable */

#define AVI_ERR_CLOSE        6     /* Could not write header to AVI file
                                      or not truncate the file during close,
                                      file is most probably corrupted */

#define AVI_ERR_NOT_PERM     7     /* Operation not permitted:
                                      trying to read from a file open
                                      for writing or vice versa */

#define AVI_ERR_NO_MEM       8     /* malloc failed */

#define AVI_ERR_NO_AVI       9     /* Not an AVI file */

#define AVI_ERR_NO_HDRL     10     /* AVI file has no has no header list,
                                      corrupted ??? */

#define AVI_ERR_NO_MOVI     11     /* AVI file has no has no MOVI list,
                                      corrupted ??? */

#define AVI_ERR_NO_VIDS     12     /* AVI file contains no video data */

#define AVI_ERR_NO_IDX      13     /* The file has been opened with
                                      getIndex==0, but an operation has been
                                      performed that needs an index */
#define AVI_ERR_INVALID_PARAM  14     /* The paragrams are not correct */


class CAviFmtInterface
{
public:
	CAviFmtInterface();
	~CAviFmtInterface();
	/*关闭avi文件的操作。打开avi文件后，需要调用此接口，来结束对文件操作*/
	bool  AVI_close();

public://写操作
	/*打开要写入音视频的AVI文件，与读方式打开接口相斥*/
	bool AVI_open_output_file(char * filename);

	/*功能：设置视频参数
	**参数(width：宽；height：高；fps：帧率；compressor：视频的压缩方式,如"h264"；)*/
	void AVI_set_video(int width, int height, double fps, char *compressor);

	/*功能：设置音频参数
	**参数：channels：通道数；rate：；bits：；format：*/
	void AVI_set_audio(int channels, long rate, int bits, int format);

	/*功能：写入一帧视频数据
	**参数：data：视频帧数据地址；bytes：视频帧数据长度；*/
	bool  AVI_write_frame(const char *data, long bytes,bool bIsKeyFrame);

	bool  AVI_dup_frame();

	bool  AVI_write_audio(const char *data, long bytes,bool bIsKeyFrame =false);
	long AVI_bytes_remain();

	int FindStartCode3 (char *Buf) ;
	int GetAnnexbNALU (FILE *pFd, char* nalu) ;

public://读操作
	/*功能：打开要读出音视频的AVI文件，与写方式打开接口相斥。即调用此方法后，如果再去调用AVI_open_output_file，将会得到失败的结果
	**参数：filename：打开的文件名；getIndex：是否读出索引，1是，0否*/
	bool AVI_open_input_file(const char *filename);
	/*功能：获取视频帧总数*/
	long AVI_video_frames();
	/*功能：获取视频帧宽度*/
	int  AVI_video_width();
	/*功能：获取视频帧高度*/
	int  AVI_video_height();
	/*功能：获取视频帧率*/
	double AVI_video_frame_rate();
	/*功能：获取视频压缩方式*/
	char* AVI_video_compressor();

	/*功能：获取音频通道数*/
	int  AVI_audio_channels();
	/*功能：获取视频帧宽度*/
	int  AVI_audio_bits();
	int  AVI_audio_format();
	long AVI_audio_rate();
	long AVI_audio_bytes();

	/*功能：获取一个视频帧的视频数据大小
	**参数：frame：帧序号（帧序号是从零开始计算的）*/
	long AVI_frame_size(long frame);

	/*功能：设置音视频读写位置为初始值*/
	bool  AVI_seek_start();

	/*功能：设置视频读写位置到指定序列帧的位置
	**参数：frame：帧序号；frame_len：取得相应帧的数据大小的指针*/
	bool  AVI_set_video_position(long frame, long *frame_len);

	/*功能：获取当前位置的视频帧数据
	**参数：vidbuf：存放视频帧数据的内存的指针*/
	long AVI_read_frame(char *vidbuf,unsigned long bufLength,bool &bIsKeyFrame);


	bool  AVI_set_audio_position(long byte);
	bool  AVI_set_audio_frame(long frame, long *frame_len);
	long AVI_read_audio(char *audbuf, long bytes);

	/*功能：获取当前位置的音视频帧数据
	**参数：vidbuf：存放视频帧数据的内存的指针；max_vidbuf：用于存放获取视频帧数据的内存空间大小；
	        audbuf：存放音频帧数据的内存的指针；max_audbuf：用于存放获取音频帧数据的内存空间大小；
			len：获取到的音频或者视频帧的数据的大小
			bIsKeyFrame：音视频数据是否是关键帧
	**返回值：*    1 = video data read
			 *    2 = audio data read
			 *    0 = reached EOF
			 *   -1 = video buffer too small
			 *   -2 = audio buffer too small*/
	int  AVI_read_data(char *vidbuf, long max_vidbuf,
								   char *audbuf, long max_audbuf,
								   long *len,bool &bIsKeyFrame);

private:
	/*功能：获取一个音频帧的大小*/
	int avi_sampsize();

	/*功能：写入一帧音频帧的数据块到文件的数据区*/
	bool avi_add_chunk(char *tag, const unsigned char *data, int length);

	/*功能：写入一帧音频帧的索引块到文件的索引区*/
	bool avi_add_index_entry(char *tag, long flags, long pos, long len);

	/*功能：写入AVI文件的头尾信息*/
	bool avi_close_output_file();

	/*功能：写入一帧音频或者视频数据
	**参数：data：存放数据的指针；length：存放的数据的大小；audio：标识是音频帧还是视频帧，1音频，0视频；bIsKeyFrame：是否是关键帧*/
	bool avi_write_data(const char *data, long length, int audio,bool bIsKeyFrame /*= false*/);
private:
	avi_t *m_avi;

	/*错误类型标识*/
	long AVI_errno;

	/*用于存放当前要写入的数据帧的缓存*/
	char *m_pAviFormatFrameBuf;
	long m_MaxAviFormatFrameLen;

};
#endif