/************************************************************
*FileName: MediaStream.h
*Date:     2018-07-15
*Author:   Wen Lee
*Version:  V1.0
*Description: 模拟音频流和视频流数据
*Others:   
*History:   
***********************************************************/
#ifndef _MEDIA_STREAM_H_
#define _MEDIA_STREAM_H_

#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h> 

#define MAX_VIDEO_SIZE		(1024*512)
#define VIDEO_FILE_NAME		"./File/test.h264"
#define AUDIO_FILE_NAME		"./File/2Channel_44100_16bit.pcm"


typedef struct  
{  
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)  
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)  
	unsigned max_size;            //! Nal Unit Buffer size  
	int forbidden_bit;            //! should be always FALSE  
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx  
	int nal_unit_type;            //! NALU_TYPE_xxxx      
	char *buf;                    //! contains the first byte followed by the EBSP  
	unsigned short lost_packets;  //! true, if packet loss is detected  
} NALU_S; 


NALU_S *InitVideoStream(int s32Len);
int ReadOneFrameVideo(NALU_S *pstNalu);
int CloseVideoStream(NALU_S *pstNalu);


int InitAudioStream(void);
int CloseAudioStream(void);
int ReadOneFrameAudio(unsigned char *pu8AudioData,unsigned int u32Len);



#endif
