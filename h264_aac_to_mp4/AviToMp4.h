#ifndef __AVI_TO_MP4_H__
#define __AVI_TO_MP4_H__

int aviToMp4(char *aviFilePath, char* mp4FilePath);

typedef struct
{
	char arrs8AACFileName[128];
	char arrs8H264FileName[128];
	unsigned int u32VideoFrameNum;
	unsigned int u32VideoFrameH;
	unsigned int u32VideoFrameW;
	unsigned int u32VideoFrameRate;
}H264_AAC_PARA;

int H264AacToMp4(H264_AAC_PARA stPara,char* mp4FilePath);

#endif