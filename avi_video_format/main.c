/************************************************************
*FileName: main.h
*Date:     2018-07-15
*Author:   Wen Lee
*Version:  V1.0
*Description: 
*Others:   
*History:   
***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "avilib.h"
#include "MediaStream.h"

#define WRITE_FILE_NAME		"./File/out.h264"
#define WRITE_AVI_NAME		"./File/test.avi"

#define AVI_VIDEO_FLAG			"00dc"
#define AVI_AUDIO_FLAG			"01wb"


static FILE *g_out_h264_fd = NULL;
static FILE *g_out_avi_fd = NULL;

int CreatOuth264File(void)
{
	g_out_h264_fd = fopen(WRITE_FILE_NAME,"w+");
	if(NULL==g_out_h264_fd)
	{
		printf("%s %d fopen error \n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

int CreatAVIFile(void)
{
	g_out_avi_fd = fopen(WRITE_AVI_NAME,"w+");
	if(NULL==g_out_avi_fd)
	{
		printf("%s %d fopen error \n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

/************************************************* 
Function:    ExportCalcAlign
Description: Data alignment
Input:  u32Data, u8Align
OutPut: 
Return: Aligned data
Others: 
Author: Caibiao Lee
Date:   2018-03-06
*********************************************************/
unsigned int AVICalcAlign(unsigned int u32Data,unsigned char u8Align)
{
	return ((u32Data + u8Align - 1) & (~(u8Align - 1)));
}


/******************************************************** 
Function: TestH264ReadAndWrite  
Description:将h264文件一帧一帧的读取出来，
	然后再一帧一帧的把数据写入到另外一文件
	用来测试模拟的视频流是否正确
Input:  None
OutPut: None
Return: 0: success，none 0:error
Others: 
Author: Wen Lee
Date:   2018.07.15
*********************************************************/
int TestH264ReadAndWrite(void)
{
	int l_s32Ret = 0;
	int l_DateLen = 0;
	
	NALU_S * l_pstNalu = NULL;

	/**创建h264输出文件**/
	l_s32Ret = CreatOuth264File();
	if(0!=l_s32Ret)
	{
		return -1;
	}

	/**初始化视频流数据**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}

	/**读取写入整个文件**/
	while(1)
	{
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			l_s32Ret = fwrite(l_pstNalu->buf,1,l_DateLen,g_out_h264_fd);
			if(l_s32Ret!=l_DateLen)
			{
				printf("%s %d write file error \n",__FILE__,__LINE__);
			}
		}else
		{
			break;
		}
	}
	
	/**关闭视频流文件**/
	CloseVideoStream(l_pstNalu);
	fclose(g_out_h264_fd);

	return 0;
}

/******************************************************** 
Function: TestH264ToAVI  
Description:AVI文件里只封装视频文件
Input:  None
OutPut: None
Return: 0: success，none 0:error
Others: 
Author: Caibiao Lee
Date:   2017-12-14
*********************************************************/
int TestH264ToAVI(void)
{
	int l_s32Ret = 0;
	int l_DateLen = 0;
	int l_u32WriteLen = 0;
	unsigned int l_u32MoviLen = 0;
	unsigned int l_u32AVIWriteLen = 0;
	AVI_PARA_S stPara;	
	NALU_S * l_pstNalu = NULL;
	unsigned char *l_pu8Buff = NULL;

	/**创建AVI文件**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		return -1;
	}
	
	/**初始化视频流数据**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}

	/**视频参数**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**音频参数**/
	stPara.bAenc 		= false; //不使用音频 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 8000;  
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;

	/**设置是否使用索引**/
	stPara.s8HasIndex = 0; 

	/**初始化AVI文件头结构**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	l_pu8Buff = (unsigned char*)malloc(MAX_VIDEO_SIZE);
	if(NULL==l_pu8Buff)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	while(1)
	{
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			/**设置视频标签**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**数据对齐**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**设置长度**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**实际应该写入的长度**/
			l_u32WriteLen += 8 ;   /**4ByteFlag + 4ByteLen**/
			memcpy(l_pu8Buff + 8,l_pstNalu->buf,l_DateLen);
			
			l_s32Ret = fwrite(l_pu8Buff,1,l_u32WriteLen,g_out_avi_fd);
			if(l_s32Ret!=l_u32WriteLen)
			{
				printf("%s %d write file error \n",__FILE__,__LINE__);
			}
			l_u32MoviLen +=l_u32WriteLen;
		}
		else
		{
			break ;
		}
	}

	/**更新AVI文件头结构中的长度值**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);
	
	/**RIFF文件中的度**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/
	
	/**movi块的长度**/
	stPara.u32MoviLen = l_u32MoviLen;

	/**写入到头文件中**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);
	
	return 0;
}

/******************************************************** 
Function: TestH264AndPCMToAVI  
Description:将h264视频流和PCM音频流封装到AVI文件中
Input:  None
OutPut: None
Return: 0: success，none 0:error
Others: 测试使用的音频是2Channel_44100_16bit.pcm
	音频采样率 			 = 44100，采样通道 = 2，位深度 = 16
	假设采样间隔    		 = 20ms，也就是每秒采集50次 
	一秒钟总的数据量 =      44100 * 2*16/8 = 176400 字节
	所以每帧音频数据大小 = 176400/50 = 3528 字节
	视频帧率是25 音频率是50，模拟媒体流时间不准确，
	所以使用存一帧视频再存两帧音频使音视频同步
Author: Wen Lee
Date:   2018-07-20
*********************************************************/
int TestH264AndPCMToAVI(void)
{
	int l_s32Ret = 0;
	int l_DateLen = 0;
	int l_u32WriteLen = 0;
	int l_u32AudioLen = 0;
	unsigned int l_u32AVIWriteLen = 0;
	AVI_PARA_S stPara;	
	NALU_S * l_pstNalu = NULL;
	unsigned char *l_pu8Buff = NULL;
	unsigned char l_arru8Audio[1024*4] = {0};
		AVI_INDEX_ENTRY_S l_stIndexEntry;

	/**创建AVI文件**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		return -1;
	}
	
	/**初始化视频流数据**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}



	/**打开音频流**/
	l_s32Ret = InitAudioStream();
	if(0!=l_s32Ret)
	{
		return -1;
	}
		
	/**视频参数**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**音频参数**/
	stPara.bAenc 		= true; //不使用音频 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 44100;  
	stPara.u32Channels  = 2;
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;
	stPara.u32AVChanNum = 2;

	/**设置是否使用索引**/
	stPara.s8HasIndex = 0; 

	/**初始化AVI文件头结构**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	l_pu8Buff = (unsigned char*)malloc(MAX_VIDEO_SIZE);
	if(NULL==l_pu8Buff)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	while(1)
	{
		/**读一帧视频数据**/
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			/**设置视频标签**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**数据对齐**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**设置长度**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**实际应该写入的长度**/
			l_u32WriteLen += 8 ;   /**4ByteFlag + 4ByteLen**/
			memcpy(l_pu8Buff + 8,l_pstNalu->buf,l_DateLen);
			
			l_s32Ret = fwrite(l_pu8Buff,1,l_u32WriteLen,g_out_avi_fd);
			if(l_s32Ret!=l_u32WriteLen)
			{
				printf("%s %d write file error \n",__FILE__,__LINE__);
			}

		}
		else
		{
			break ;
		}
		
		/**读两帧音频数据**/
		for(int i=0;i<2;i++)
		{
			l_u32AudioLen = 3528;
			l_DateLen = ReadOneFrameAudio(&l_arru8Audio[8],l_u32AudioLen);
			if(l_DateLen > 0)
			{
				/**设置视频标签**/
				memcpy(l_arru8Audio,AVI_AUDIO_FLAG,4);
				/**数据对齐**/
				l_u32AudioLen = AVICalcAlign(l_DateLen,4);
				/**设置长度**/
				l_arru8Audio[4] = (l_u32AudioLen)&0xff;
				l_arru8Audio[5] = (l_u32AudioLen>> 8)&0xff;
				l_arru8Audio[6] = (l_u32AudioLen>>16)&0xff;
				l_arru8Audio[7] = (l_u32AudioLen>>24)&0xff;	

				l_u32AudioLen += 8;
				l_s32Ret = fwrite(l_arru8Audio,1,l_u32AudioLen,g_out_avi_fd);
				if(l_s32Ret!=l_u32AudioLen)
				{
					printf("%s %d write file error \n",__FILE__,__LINE__);
				}

			}
		}
	}

	/**更新AVI文件头结构中的长度值**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);

	stPara.u32MoviLen = l_u32AVIWriteLen;

	/**RIFF文件中的度**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/
	

	/**写入到头文件中**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	CloseVideoStream(l_pstNalu);
	CloseAudioStream();

	return 0;
}



/******************************************************** 
Function: TestH264AndPCMToAVI  
Description:合成音频视频流，同时添加索引表
Input:  None
OutPut: None
Return: 0: success，none 0:error
Others: 在TestH264AndPCMToAVI的基础上添加索引表
Author: Wen Lee
Date:   2018-07-20
*********************************************************/
int TestAVIIndex(void)
{
	int l_s32Ret = 0;
	int l_DateLen = 0;
	int l_u32WriteLen = 0;
	int l_u32AudioLen = 0;
	unsigned int l_u32AVIWriteLen = 0;
	AVI_PARA_S stPara;	
	NALU_S * l_pstNalu = NULL;
	unsigned char *l_pu8Buff = NULL;
	unsigned char l_arru8Audio[1024*4] = {0};
	AVI_INDEX_ENTRY_S l_stIndexEntry;
	WRITE_AVI_INDEX_S *pstWriteAviIndex;
	
	/**创建AVI文件**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}
	
	/**初始化视频流数据**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}



	/**打开音频流**/
	l_s32Ret = InitAudioStream();
	if(0!=l_s32Ret)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}

	/**为索引表分配空间**/
	pstWriteAviIndex = (WRITE_AVI_INDEX_S *)malloc(sizeof(WRITE_AVI_INDEX_S));
	if(NULL==pstWriteAviIndex)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	/**视频参数**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**音频参数**/
	stPara.bAenc 		= true; //不使用音频 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 44100;  
	stPara.u32Channels  = 2;
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;
	stPara.u32AVChanNum = 2;

	/**!!设置是否使用索引 !!**/
	stPara.s8HasIndex = 1; 

	/**初始化AVI文件头结构**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	l_pu8Buff = (unsigned char*)malloc(MAX_VIDEO_SIZE);
	if(NULL==l_pu8Buff)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	while(1)
	{
		/**读一帧视频数据**/
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			/**设置视频标签**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**数据对齐**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**设置长度**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**实际应该写入的长度**/
			l_u32WriteLen += 8 ;   /**4ByteFlag + 4ByteLen**/
			memcpy(l_pu8Buff + 8,l_pstNalu->buf,l_DateLen);
			
			l_s32Ret = fwrite(l_pu8Buff,1,l_u32WriteLen,g_out_avi_fd);
			if(l_s32Ret!=l_u32WriteLen)
			{
				printf("%s %d write file error \n",__FILE__,__LINE__);
			}

			l_stIndexEntry.u32Flags = 0x00;
			/**判断是否为关键帧 关键帧为0x10 其它为0x00   **/
			if((0==l_pu8Buff[8])&&(0==l_pu8Buff[9])&&(1==l_pu8Buff[10])&&(0x05==(l_pu8Buff[11]&0x1F)))
			{
				l_stIndexEntry.u32Flags = 0x10;
			}
			if((0==l_pu8Buff[8])&&(0==l_pu8Buff[9])&&(0==l_pu8Buff[10])&&(1==l_pu8Buff[11])
				&&(0x05==(l_pu8Buff[12]&0x1F)))
			{
				l_stIndexEntry.u32Flags = 0x10;
			}
			
			l_stIndexEntry.u32ChunkId = 0x63643030; /**'00dc'**/
			l_stIndexEntry.u32Size    = l_u32WriteLen - 8;
			RIFF_WriteAviIndex(l_stIndexEntry,pstWriteAviIndex);

		}
		else
		{
			break ;
		}
		
		/**读两帧音频数据**/
		for(int i=0;i<2;i++)
		{
			l_u32AudioLen = 3528;
			l_DateLen = ReadOneFrameAudio(&l_arru8Audio[8],l_u32AudioLen);
			if(l_DateLen > 0)
			{
				/**设置视频标签**/
				memcpy(l_arru8Audio,AVI_AUDIO_FLAG,4);
				/**数据对齐**/
				l_u32AudioLen = AVICalcAlign(l_DateLen,4);
				/**设置长度**/
				l_arru8Audio[4] = (l_u32AudioLen)&0xff;
				l_arru8Audio[5] = (l_u32AudioLen>> 8)&0xff;
				l_arru8Audio[6] = (l_u32AudioLen>>16)&0xff;
				l_arru8Audio[7] = (l_u32AudioLen>>24)&0xff;	

				l_u32AudioLen += 8;
				l_s32Ret = fwrite(l_arru8Audio,1,l_u32AudioLen,g_out_avi_fd);
				if(l_s32Ret!=l_u32AudioLen)
				{
					printf("%s %d write file error \n",__FILE__,__LINE__);
				}

				l_stIndexEntry.u32ChunkId = 0x64773130; /**'01wd'**/
				l_stIndexEntry.u32Flags = 0x00;
				l_stIndexEntry.u32Size = l_u32AudioLen - 8;
				RIFF_WriteAviIndex(l_stIndexEntry,pstWriteAviIndex);
			}
		}
	}

	/**更新AVI文件头结构中的长度值**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);

	stPara.u32MoviLen = l_u32AVIWriteLen;
	
	/**写索引表到AVI文件中去***/
	RIFF_WriteAviIndexToFile(pstWriteAviIndex,g_out_avi_fd);

	/**更新AVI文件头结构中的长度值**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);
	/**RIFF文件中的度**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/

	/**写入到头文件中**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	/**关闭资源**/
	CloseVideoStream(l_pstNalu);
	CloseAudioStream();

	return 0;
}



int main(void)
{
	int l_s32Ret = 0;
	
	printf("Hello World \n");

	/**h246文件测试**/
	//l_s32Ret = TestH264ReadAndWrite();

	/**只封装视频数据*/
	//l_s32Ret = TestH264ToAVI();

	/**同时封装音频和视频数据**/
	//l_s32Ret = TestH264AndPCMToAVI();

	/**添加索引**/
	l_s32Ret = TestAVIIndex();
	
	return 0;
}


