// FormatConverter.cpp : Defines the entry point for the console application.
//
#include <string.h>
#include "stdio.h"
#include "AviToMp4.h"
#include "Mp4ToAvi.h"


int main(int argc, char** argv)
{
	//aviToMp4("..\\..\\music.avi","..\\..\\aviTOmp4.mp4");
#if 0

	if(3!=argc)
	{
		printf("=====ERROR!======\n");
		printf("usage: %s input file name   output file name \n", argv[0]);
		printf("eg 1: %s  test.avi  test.mp4 \n", argv[0]);
		//return 0;
	}
		
	aviToMp4(argv[1],argv[2]);
#else
	//mp4ToAvi(".mp4","/home/windychan/mp4TOavi.avi");
	H264_AAC_PARA stPara = {0};
	stPara.u32VideoFrameW = 1280;
	stPara.u32VideoFrameH = 720;
	stPara.u32VideoFrameRate = 15;
	strcpy(stPara.arrs8AACFileName,"./test.aac");
	strcpy(stPara.arrs8H264FileName,"./ExportChannel0Video.h264");

	H264AacToMp4(stPara, argv[1]);
#endif
	return 0;
}

