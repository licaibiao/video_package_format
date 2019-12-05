/*功能：从外部的AVI文件读取H264视频数据，以及从AAC文件读取音频数据，然后合成到MP4文件中*/
#include "AviToMp4.h"
#include "Mp4FmtInterface.h"
#include "AviFmtInterface.h"
#include "type.h"


typedef struct
{
	uint8 *buf;
	uint8 *buf_start;
	uint8 *buf_end;
} AACBUFFER;

typedef struct
{
	uint8 aac_profile;
	uint32 aac_sampling_frequency;
	uint8 aac_channel_configuration;
	uint32 aac_frame_length;
	uint8 aac_num_of_datablock;
	uint8 *aac_frame_start;
	//uint8 *aac_frame_end;
} ADTS_HEADER;

typedef struct
{
	uint32 frame_num;
	uint32 aac_length;
	uint8 *file_aac_end;
	uint16 nchannels;//声道
	uint32 nSamplesPerSec;//采样率
	//uint16 nBlockAlign;//采样大小
	bool flag;

} AAC;

bool read_aac_frame(AACBUFFER *aacbuffer,ADTS_HEADER *adts_header);
void init_adts_header(ADTS_HEADER *adts_header);
int Find_syncword(AACBUFFER *aacbuffer);
void init_aac_buf(AACBUFFER* aacbuffer);
void init_aac(AAC *aac);
void parse_adts_header(AACBUFFER *aacbuffer,ADTS_HEADER *adts_header);
long file_aac_length;

int aviToMp4(char *aviFilePath, char* mp4FilePath)
{
	int VIDEO_BUF_LEN = 327680;
	int AUDIO_BUF_LEN = 100000;
	char * vidbuf = (char*)malloc(VIDEO_BUF_LEN);
    char * audbuf = (char*)malloc(AUDIO_BUF_LEN);
	if(vidbuf==NULL || audbuf==NULL){return -1;}

	CAviFmtInterface aviFormatInsatance;
	aviFormatInsatance.AVI_open_input_file(aviFilePath);
	long frames = aviFormatInsatance.AVI_video_frames();
	int framew = aviFormatInsatance.AVI_video_width();
	int frameh = aviFormatInsatance.AVI_video_height();
	double framerate = aviFormatInsatance.AVI_video_frame_rate();
	if(!aviFormatInsatance.AVI_seek_start()){return -1;}

	printf("frames = %d framew = %d frameh = %d framerate = 0x%x \n",
		frames,framew,frameh,framerate);

	CMp4FmtInterface mp4FormatInsatance;
	mp4FormatInsatance.OpenFile(mp4FilePath,OPEN_MODEL_W);
	mp4FormatInsatance.SetMp4Param((u_int16_t)framew,(u_int16_t)frameh,framerate);

	FILE *in_aac = NULL;
#ifdef WIN32
	if((in_aac = fopen("..\\..\\csst.aac","rb")) == NULL)
#else
	//if((in_aac = fopen("/home/biao/test/AVI2Mp4/csst.aac","rb")) == NULL)
	if((in_aac = fopen("./test.aac","rb")) == NULL)
#endif
	{
		printf("Can't open the aac file\n");
		return -1;
	}

	//音频
	fseek(in_aac,0,SEEK_END);
	file_aac_length = ftell(in_aac);
	fseek(in_aac,0,SEEK_SET);
	if(file_aac_length == 0)
	{
		return -1;
	}

	//音频
	AACBUFFER aacbuffer;
	init_aac_buf(&aacbuffer);
	fread(aacbuffer.buf,1,AACBUFSIZE,in_aac);
	ADTS_HEADER adts_header;


	int idx = 0;
	int idxAudio = 0;
	FILE *l_pFpH264 = NULL;
	l_pFpH264 = fopen("./test.h264","w+");
	if(NULL==l_pFpH264)
	{
		printf("%s %d open h264 data error \n",__FUNCTION__,__LINE__);
		return -1;
	}
	while (1)//read and write avi per fream
	{
		init_adts_header(&adts_header);
		long readLen = 0;
		bool bIsKeyFrame = false;
		int bResult = aviFormatInsatance.AVI_read_data(vidbuf,VIDEO_BUF_LEN,audbuf,AUDIO_BUF_LEN,&readLen,bIsKeyFrame);

		if(bResult==1)
		{
			//fwrite((unsigned char*)vidbuf,1,readLen,l_pFpH264);
			if(!mp4FormatInsatance.WriteVideoFrameData((unsigned char*)vidbuf,readLen))
			{
				printf("Write video frame data fail. The frame index is:%05d.\n",idx);
			}
			idx++;
		}
		else if(bResult==2)
		{
			idxAudio++;
		}
		else if(bResult==0)
			break;

		
	}
	if(NULL!=l_pFpH264)
	{
		fclose(l_pFpH264);
		l_pFpH264 = NULL;
	}
	
	idx = 0;
	while(1)
	{
		if(read_aac_frame(&aacbuffer,&adts_header))
		{
			if(!mp4FormatInsatance.WriteAudioFrameData((const unsigned char*)(adts_header.aac_frame_start-7),adts_header.aac_frame_length+7))//此接口需要的数据信息，需要包含7个字节的头信息
			{
				printf("error!\n");
				break;
			}
			idx++;
		}
		else
		{
			printf("Reach to the end of aac file!\n");
			break;
		}
	}

	mp4FormatInsatance.Close(); 
	aviFormatInsatance.AVI_close();   

	free(vidbuf);
	free(audbuf);
	return 0;
}



int H264AacToMp4(H264_AAC_PARA stPara,char* mp4FilePath)
{
	int VIDEO_BUF_LEN = 327680;
	int AUDIO_BUF_LEN = 100000;
	CAviFmtInterface aviFormatInsatance;
	CMp4FmtInterface mp4FormatInsatance;
	int framew = stPara.u32VideoFrameW;
	int frameh = stPara.u32VideoFrameH;
	long framerate = stPara.u32VideoFrameRate;

	char * vidbuf = (char*)malloc(VIDEO_BUF_LEN);
    char * audbuf = (char*)malloc(AUDIO_BUF_LEN);
	if(vidbuf==NULL || audbuf==NULL)
	{
		return -1;
	}

	mp4FormatInsatance.OpenFile(mp4FilePath,OPEN_MODEL_W);
	mp4FormatInsatance.SetMp4Param((u_int16_t)framew,(u_int16_t)frameh,framerate);

	FILE *in_aac = NULL;
	FILE *in_h264 = NULL;

	in_aac = fopen(stPara.arrs8AACFileName,"rb");
	if(in_aac == NULL)
	{
		printf("Can't open the aac file\n");
		return -1;
	}
	
	in_h264 = fopen(stPara.arrs8H264FileName,"rb") ;
	if(in_h264== NULL)
	{
		printf("Can't open the h264 file\n");
		return -1;
	}

	//音频
	fseek(in_aac,0,SEEK_END);
	file_aac_length = ftell(in_aac);
	fseek(in_aac,0,SEEK_SET);
	if(file_aac_length == 0)
	{
		return -1;
	}

	//音频
	AACBUFFER aacbuffer;
	init_aac_buf(&aacbuffer);
	fread(aacbuffer.buf,1,AACBUFSIZE,in_aac);
	ADTS_HEADER adts_header;


	int idx = 0;
	int idxAudio = 0;

	while (1)//read and write avi per fream
	{
		init_adts_header(&adts_header);

		long readLen = 0;
		readLen = aviFormatInsatance.GetAnnexbNALU(in_h264, vidbuf);
		if(readLen<=0)
		{
			break;
		};
		
		if(!mp4FormatInsatance.WriteVideoFrameData((unsigned char*)vidbuf,readLen))
		{
			printf("Write video frame data fail. The frame index is:%05d.\n",idx);
		}
		idx++;	
	}
	
	idx = 0;
	while(1)
	{
		if(read_aac_frame(&aacbuffer,&adts_header))
		{
			if(!mp4FormatInsatance.WriteAudioFrameData((const unsigned char*)(adts_header.aac_frame_start-7),adts_header.aac_frame_length+7))//此接口需要的数据信息，需要包含7个字节的头信息
			{
				printf("error!\n");
				break;
			}
			idx++;
		}
		else
		{
			printf("Reach to the end of aac file!\n");
			break;
		}
	}

	mp4FormatInsatance.Close(); 
	
	if(NULL!=in_h264)
	{
		fclose(in_h264);
		in_h264 = NULL;
	}

	if(NULL!=in_aac)
	{
		fclose(in_aac);
		in_h264 = NULL;
	}

	free(vidbuf);
	free(audbuf);
	return 0;
}
//以下接口为AAC文件数据分析接口
bool read_aac_frame(AACBUFFER *aacbuffer,ADTS_HEADER *adts_header)
{
	int aac_ayncword = 0;
	while(aacbuffer->buf <= aacbuffer->buf_start+file_aac_length)
	{
		aac_ayncword = Find_syncword(aacbuffer);
		if(aac_ayncword == 1)
		{
				//分析adts的帧头
			parse_adts_header(aacbuffer,adts_header);
			aacbuffer->buf++;
	        adts_header->aac_frame_start = aacbuffer->buf;
			aacbuffer->buf = adts_header->aac_frame_start + adts_header->aac_frame_length;
			return true;
		}
	}
	return false;
}

int Find_syncword(AACBUFFER *aacbuffer)
{
	if(*(aacbuffer->buf++) != 0xff)
	{
		return 0;
	}
	 
	//aacbuffer->buf++;
	if((*(aacbuffer->buf)&0xf6) != 0xf0)
	{
		return 0;
	}
	return 1;
}


void init_aac_buf(AACBUFFER* aacbuffer)
{
	aacbuffer->buf = (uint8 *)malloc(AACBUFSIZE*sizeof(uint8));
	if(aacbuffer->buf == NULL)
	{
		printf("error!");
		return;
	}
	memset(aacbuffer->buf,'\0',AACBUFSIZE*sizeof(uint8));//
	aacbuffer->buf_start = aacbuffer->buf;
	aacbuffer->buf_end = aacbuffer->buf_start + AACBUFSIZE - 1;
}

void init_adts_header(ADTS_HEADER *adts_header)
{
	adts_header->aac_channel_configuration = 0;
	adts_header->aac_frame_length = 0;
	adts_header->aac_num_of_datablock = 0;
	adts_header->aac_profile = 0;
	adts_header->aac_sampling_frequency = 0;
	adts_header->aac_frame_start = NULL;
}

void init_aac(AAC *aac)
{
	aac->aac_length = 0;
	aac->frame_num = 0;
	aac->file_aac_end = NULL;
	aac->nchannels = 0;
	aac->nSamplesPerSec = 0;
	aac->flag = false;
}

void parse_adts_header(AACBUFFER *aacbuffer,ADTS_HEADER *adts_header)
{
	uint8 aac_sampling_frequency_index = 0;
	aacbuffer->buf++;
	adts_header->aac_profile = ((*aacbuffer->buf)&0xc0) >> 6;
	aac_sampling_frequency_index = ((*aacbuffer->buf)&0x3c) >> 2;
	switch(aac_sampling_frequency_index)
	{
		case 0x0:
			adts_header->aac_sampling_frequency = 96000;
			break;
		case 0x1:
			adts_header->aac_sampling_frequency = 88200;
			break;
		case 0x2:
			adts_header->aac_sampling_frequency = 64000;
			break;
		case 0x3:
			adts_header->aac_sampling_frequency = 48000;
			break;
		case 0x4:
			adts_header->aac_sampling_frequency = 44100;
			break;
		case 0x5:
			adts_header->aac_sampling_frequency = 32000;
			break;
		case 0x6:
			adts_header->aac_sampling_frequency = 24000;
			break;
		case 0x7:
			adts_header->aac_sampling_frequency = 22050;
			break;
		case 0x8:
			adts_header->aac_sampling_frequency = 16000;
			break;
		case 0x9:
			adts_header->aac_sampling_frequency = 12000;
			break;
		case 0xa:
			adts_header->aac_sampling_frequency = 11025;
			break;
		case 0xb:
			adts_header->aac_sampling_frequency = 8000;
			break;
		default:
			return;

	}

	uint8 a1 = ((*aacbuffer->buf++)&0x01) << 2;
	uint8 a2 = ((*aacbuffer->buf)&0xc0) >>6;
	adts_header->aac_channel_configuration = a1|a2;
	//adts_header->aac_channel_configuration = (((*aacbuffer->buf++)&0x01) << 2)
		                                     //|(((*aacbuffer->buf)&0xc0) >>6);
	int a = (((*aacbuffer->buf++)&0x03) << 11);
	int b = (((*aacbuffer->buf++)&0xff) << 3);
	int c = (((*aacbuffer->buf++)&0xe0) >> 5);
	adts_header->aac_frame_length = (a|b|c) - 7;
	adts_header->aac_num_of_datablock = ((*aacbuffer->buf)&0x03) + 1;
}
