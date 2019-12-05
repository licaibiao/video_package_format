#ifndef __AVI_FORMAT_INTERFACE_H__
#define __AVI_FORMAT_INTERFACE_H__

//��Ƶ�����ṹ��
typedef struct
{
   long pos;
   long len;
   long flag;
} video_index_entry;

//��Ƶ�����ṹ��
typedef struct
{
   long pos;
   long len;
   long flag;
   long tot;
} audio_index_entry;


//���AVI�ļ���Ϣ�Ľṹ��
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
	/*�ر�avi�ļ��Ĳ�������avi�ļ�����Ҫ���ô˽ӿڣ����������ļ�����*/
	bool  AVI_close();

public://д����
	/*��Ҫд������Ƶ��AVI�ļ��������ʽ�򿪽ӿ����*/
	bool AVI_open_output_file(char * filename);

	/*���ܣ�������Ƶ����
	**����(width����height���ߣ�fps��֡�ʣ�compressor����Ƶ��ѹ����ʽ,��"h264"��)*/
	void AVI_set_video(int width, int height, double fps, char *compressor);

	/*���ܣ�������Ƶ����
	**������channels��ͨ������rate����bits����format��*/
	void AVI_set_audio(int channels, long rate, int bits, int format);

	/*���ܣ�д��һ֡��Ƶ����
	**������data����Ƶ֡���ݵ�ַ��bytes����Ƶ֡���ݳ��ȣ�*/
	bool  AVI_write_frame(const char *data, long bytes,bool bIsKeyFrame);

	bool  AVI_dup_frame();

	bool  AVI_write_audio(const char *data, long bytes,bool bIsKeyFrame =false);
	long AVI_bytes_remain();

	int FindStartCode3 (char *Buf) ;
	int GetAnnexbNALU (FILE *pFd, char* nalu) ;

public://������
	/*���ܣ���Ҫ��������Ƶ��AVI�ļ�����д��ʽ�򿪽ӿ���⡣�����ô˷����������ȥ����AVI_open_output_file������õ�ʧ�ܵĽ��
	**������filename���򿪵��ļ�����getIndex���Ƿ����������1�ǣ�0��*/
	bool AVI_open_input_file(const char *filename);
	/*���ܣ���ȡ��Ƶ֡����*/
	long AVI_video_frames();
	/*���ܣ���ȡ��Ƶ֡���*/
	int  AVI_video_width();
	/*���ܣ���ȡ��Ƶ֡�߶�*/
	int  AVI_video_height();
	/*���ܣ���ȡ��Ƶ֡��*/
	double AVI_video_frame_rate();
	/*���ܣ���ȡ��Ƶѹ����ʽ*/
	char* AVI_video_compressor();

	/*���ܣ���ȡ��Ƶͨ����*/
	int  AVI_audio_channels();
	/*���ܣ���ȡ��Ƶ֡���*/
	int  AVI_audio_bits();
	int  AVI_audio_format();
	long AVI_audio_rate();
	long AVI_audio_bytes();

	/*���ܣ���ȡһ����Ƶ֡����Ƶ���ݴ�С
	**������frame��֡��ţ�֡����Ǵ��㿪ʼ����ģ�*/
	long AVI_frame_size(long frame);

	/*���ܣ���������Ƶ��дλ��Ϊ��ʼֵ*/
	bool  AVI_seek_start();

	/*���ܣ�������Ƶ��дλ�õ�ָ������֡��λ��
	**������frame��֡��ţ�frame_len��ȡ����Ӧ֡�����ݴ�С��ָ��*/
	bool  AVI_set_video_position(long frame, long *frame_len);

	/*���ܣ���ȡ��ǰλ�õ���Ƶ֡����
	**������vidbuf�������Ƶ֡���ݵ��ڴ��ָ��*/
	long AVI_read_frame(char *vidbuf,unsigned long bufLength,bool &bIsKeyFrame);


	bool  AVI_set_audio_position(long byte);
	bool  AVI_set_audio_frame(long frame, long *frame_len);
	long AVI_read_audio(char *audbuf, long bytes);

	/*���ܣ���ȡ��ǰλ�õ�����Ƶ֡����
	**������vidbuf�������Ƶ֡���ݵ��ڴ��ָ�룻max_vidbuf�����ڴ�Ż�ȡ��Ƶ֡���ݵ��ڴ�ռ��С��
	        audbuf�������Ƶ֡���ݵ��ڴ��ָ�룻max_audbuf�����ڴ�Ż�ȡ��Ƶ֡���ݵ��ڴ�ռ��С��
			len����ȡ������Ƶ������Ƶ֡�����ݵĴ�С
			bIsKeyFrame������Ƶ�����Ƿ��ǹؼ�֡
	**����ֵ��*    1 = video data read
			 *    2 = audio data read
			 *    0 = reached EOF
			 *   -1 = video buffer too small
			 *   -2 = audio buffer too small*/
	int  AVI_read_data(char *vidbuf, long max_vidbuf,
								   char *audbuf, long max_audbuf,
								   long *len,bool &bIsKeyFrame);

private:
	/*���ܣ���ȡһ����Ƶ֡�Ĵ�С*/
	int avi_sampsize();

	/*���ܣ�д��һ֡��Ƶ֡�����ݿ鵽�ļ���������*/
	bool avi_add_chunk(char *tag, const unsigned char *data, int length);

	/*���ܣ�д��һ֡��Ƶ֡�������鵽�ļ���������*/
	bool avi_add_index_entry(char *tag, long flags, long pos, long len);

	/*���ܣ�д��AVI�ļ���ͷβ��Ϣ*/
	bool avi_close_output_file();

	/*���ܣ�д��һ֡��Ƶ������Ƶ����
	**������data��������ݵ�ָ�룻length����ŵ����ݵĴ�С��audio����ʶ����Ƶ֡������Ƶ֡��1��Ƶ��0��Ƶ��bIsKeyFrame���Ƿ��ǹؼ�֡*/
	bool avi_write_data(const char *data, long length, int audio,bool bIsKeyFrame /*= false*/);
private:
	avi_t *m_avi;

	/*�������ͱ�ʶ*/
	long AVI_errno;

	/*���ڴ�ŵ�ǰҪд�������֡�Ļ���*/
	char *m_pAviFormatFrameBuf;
	long m_MaxAviFormatFrameLen;

};
#endif