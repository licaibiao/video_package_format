/*
*功能：从MP4文件中读取H264视频数据，从PCM文件中读取pcm音频数据，然后合成到AVI文件中
*说明：由于AVI可能不太支持AAC音频，所以没有直接从MP4文件中读取音频数据。AVI支持的音频格式，请参考目录下的说明文档
*/
#include "Mp4ToAvi.h"
#include "Mp4FmtInterface.h"
#include "AviFmtInterface.h"

int mp4ToAvi(char *mp4FilePath,char *aviFilePath)
{

	CAviFmtInterface aviFormatInstance;
	aviFormatInstance.AVI_open_output_file(aviFilePath);

	CMp4FmtInterface mp4FormatInstance;
	mp4FormatInstance.OpenFile(mp4FilePath,OPEN_MODEL_R);

	//音频参数配置
	/*这里根据实际的pcm文件情况填写相应的参数*/
	int samplePerSec = /*mp4FormatInstance.GetAudioSamplePerSec()*/8000;
	int audioChannels = /*mp4FormatInstance.GetAudioChannels()*/1;
	unsigned short audioFmtTag = /*mp4FormatInstance.GetAudioFormatTag()*/0x0001;
	unsigned short audioBitsPerSample = /*mp4FormatInstance.GetAudioBitsPerSample()*/16;
	if(samplePerSec==0 || audioChannels==0 || audioFmtTag==0)
	{
		return -1;
	}
	aviFormatInstance.AVI_set_audio(audioChannels,samplePerSec,audioBitsPerSample,audioFmtTag);

	DWORD audioBufLength = 256;//设为8的倍数，读取一段音频数据
	char *pPcmData = (char*)malloc(audioBufLength);
	int idx = 0;
	FILE *in_pcm = NULL;
#ifdef WIN32
	if((in_pcm = fopen("..\\..\\saima.pcm","rb")) == NULL)
#else
	if((in_pcm = fopen("/home/windychan/saima.pcm","rb")) == NULL)
#endif
	{
		printf("Can't open the pcm file\n");
		return -1;
	}
	fseek(in_pcm,0,SEEK_END);
	int file_pcm_length = ftell(in_pcm);
	fseek(in_pcm,0,SEEK_SET);
	if(file_pcm_length == 0)
	{
		return -1;
	}

	//视频参数配置
	u_int32_t framesCount = mp4FormatInstance.GetFramesCount();
	u_int16_t videoWidth = mp4FormatInstance.GetVideoWidth();
	u_int16_t videoHeight = mp4FormatInstance.GetVideoHeight();
	double frmRate = mp4FormatInstance.GetVideoFrameRate();

	if(framesCount==0 || videoWidth==0 || videoHeight==0 || frmRate==0.0)
	{
		return -1;
	}
	aviFormatInstance.AVI_set_video(videoWidth,videoHeight,frmRate,"h264");

	DWORD videoBufLength = 512*1024;
	char *pVideoFrmBuf = new char[videoBufLength];
	if(pVideoFrmBuf==NULL) return -1;


	//写数据
	long frms = 0;
	bool bIsAacEnd = false;
	bool bIsH264End = false;
	while(!bIsH264End || !bIsAacEnd)
	{
		if(frms==framesCount)
		{
			bIsH264End = true;
		}
		if(frms<framesCount && !bIsH264End)
		{
			char *pVideoFrm=pVideoFrmBuf;
			memset(pVideoFrm,0,videoBufLength);
			DWORD videoFrmSize = 0;
			int frmType = 0; //0 for normal frame;1 for key frame;
			

			if(!mp4FormatInstance.ReadVideoFrameData(frms,(BYTE*)(pVideoFrm),videoBufLength,&videoFrmSize,frmType) || videoFrmSize<=0)
			{
				printf("Get a video frame from the mp4 file fail.The frame index is:%05d.\n",frms);
				bIsH264End = true;
			}

			bool bIsKeyFrame = (frmType==0)?false:true;
			if(!aviFormatInstance.AVI_write_frame(pVideoFrm,videoFrmSize,bIsKeyFrame))
			{
				printf("Write avi video frame fail.The frame index is:%05d.\n",frms);
				return -1;
			}
			frms++;
		}

		if(!bIsAacEnd)
		{
			memset(pPcmData,0,audioBufLength);
			if(idx==1921)
			{
				printf("The last idx!");//测试最后一帧
			}
			int readLen = fread(pPcmData,1,audioBufLength,in_pcm);
			while(feof(in_pcm)==0)
			{
				if(readLen==audioBufLength)
				{
					break;
				}
				else
				{
					readLen += fread(pPcmData+readLen,1,256-readLen,in_pcm);
				}
			}

			if(feof(in_pcm)!=0)
			{
				printf("Reach to the end of pcm file!\n");
				bIsAacEnd = true;
			}

			if(!aviFormatInstance.AVI_write_audio(pPcmData,readLen,1))
			{
				printf("error!\n");
				return -1;
			}
			idx++;
		}
	}

	if(pPcmData!=NULL)
	{
		free(pPcmData);
		pPcmData = NULL;
	}
	
	if(pVideoFrmBuf!=NULL)
	{
		free(pVideoFrmBuf);
		pVideoFrmBuf = NULL;
	}
	mp4FormatInstance.Close();
	aviFormatInstance.AVI_close();
       
    return 1;
}
