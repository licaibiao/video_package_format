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
Description:��h264�ļ�һ֡һ֡�Ķ�ȡ������
	Ȼ����һ֡һ֡�İ�����д�뵽����һ�ļ�
	��������ģ�����Ƶ���Ƿ���ȷ
Input:  None
OutPut: None
Return: 0: success��none 0:error
Others: 
Author: Wen Lee
Date:   2018.07.15
*********************************************************/
int TestH264ReadAndWrite(void)
{
	int l_s32Ret = 0;
	int l_DateLen = 0;
	
	NALU_S * l_pstNalu = NULL;

	/**����h264����ļ�**/
	l_s32Ret = CreatOuth264File();
	if(0!=l_s32Ret)
	{
		return -1;
	}

	/**��ʼ����Ƶ������**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}

	/**��ȡд�������ļ�**/
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
	
	/**�ر���Ƶ���ļ�**/
	CloseVideoStream(l_pstNalu);
	fclose(g_out_h264_fd);

	return 0;
}

/******************************************************** 
Function: TestH264ToAVI  
Description:AVI�ļ���ֻ��װ��Ƶ�ļ�
Input:  None
OutPut: None
Return: 0: success��none 0:error
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

	/**����AVI�ļ�**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		return -1;
	}
	
	/**��ʼ����Ƶ������**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}

	/**��Ƶ����**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**��Ƶ����**/
	stPara.bAenc 		= false; //��ʹ����Ƶ 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 8000;  
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;

	/**�����Ƿ�ʹ������**/
	stPara.s8HasIndex = 0; 

	/**��ʼ��AVI�ļ�ͷ�ṹ**/
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
			/**������Ƶ��ǩ**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**���ݶ���**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**���ó���**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**ʵ��Ӧ��д��ĳ���**/
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

	/**����AVI�ļ�ͷ�ṹ�еĳ���ֵ**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);
	
	/**RIFF�ļ��еĶ�**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/
	
	/**movi��ĳ���**/
	stPara.u32MoviLen = l_u32MoviLen;

	/**д�뵽ͷ�ļ���**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);
	
	return 0;
}

/******************************************************** 
Function: TestH264AndPCMToAVI  
Description:��h264��Ƶ����PCM��Ƶ����װ��AVI�ļ���
Input:  None
OutPut: None
Return: 0: success��none 0:error
Others: ����ʹ�õ���Ƶ��2Channel_44100_16bit.pcm
	��Ƶ������ 			 = 44100������ͨ�� = 2��λ��� = 16
	����������    		 = 20ms��Ҳ����ÿ��ɼ�50�� 
	һ�����ܵ������� =      44100 * 2*16/8 = 176400 �ֽ�
	����ÿ֡��Ƶ���ݴ�С = 176400/50 = 3528 �ֽ�
	��Ƶ֡����25 ��Ƶ����50��ģ��ý����ʱ�䲻׼ȷ��
	����ʹ�ô�һ֡��Ƶ�ٴ���֡��Ƶʹ����Ƶͬ��
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

	/**����AVI�ļ�**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		return -1;
	}
	
	/**��ʼ����Ƶ������**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		return -1;
	}



	/**����Ƶ��**/
	l_s32Ret = InitAudioStream();
	if(0!=l_s32Ret)
	{
		return -1;
	}
		
	/**��Ƶ����**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**��Ƶ����**/
	stPara.bAenc 		= true; //��ʹ����Ƶ 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 44100;  
	stPara.u32Channels  = 2;
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;
	stPara.u32AVChanNum = 2;

	/**�����Ƿ�ʹ������**/
	stPara.s8HasIndex = 0; 

	/**��ʼ��AVI�ļ�ͷ�ṹ**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	l_pu8Buff = (unsigned char*)malloc(MAX_VIDEO_SIZE);
	if(NULL==l_pu8Buff)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	while(1)
	{
		/**��һ֡��Ƶ����**/
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			/**������Ƶ��ǩ**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**���ݶ���**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**���ó���**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**ʵ��Ӧ��д��ĳ���**/
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
		
		/**����֡��Ƶ����**/
		for(int i=0;i<2;i++)
		{
			l_u32AudioLen = 3528;
			l_DateLen = ReadOneFrameAudio(&l_arru8Audio[8],l_u32AudioLen);
			if(l_DateLen > 0)
			{
				/**������Ƶ��ǩ**/
				memcpy(l_arru8Audio,AVI_AUDIO_FLAG,4);
				/**���ݶ���**/
				l_u32AudioLen = AVICalcAlign(l_DateLen,4);
				/**���ó���**/
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

	/**����AVI�ļ�ͷ�ṹ�еĳ���ֵ**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);

	stPara.u32MoviLen = l_u32AVIWriteLen;

	/**RIFF�ļ��еĶ�**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/
	

	/**д�뵽ͷ�ļ���**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	CloseVideoStream(l_pstNalu);
	CloseAudioStream();

	return 0;
}



/******************************************************** 
Function: TestH264AndPCMToAVI  
Description:�ϳ���Ƶ��Ƶ����ͬʱ���������
Input:  None
OutPut: None
Return: 0: success��none 0:error
Others: ��TestH264AndPCMToAVI�Ļ��������������
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
	
	/**����AVI�ļ�**/
	l_s32Ret = CreatAVIFile();
	if(0!=l_s32Ret)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}
	
	/**��ʼ����Ƶ������**/
	l_pstNalu = InitVideoStream(MAX_VIDEO_SIZE);
	if(NULL == l_pstNalu)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}



	/**����Ƶ��**/
	l_s32Ret = InitAudioStream();
	if(0!=l_s32Ret)
	{
		printf("%s %d init error \n",__FILE__,__LINE__);
		return -1;
	}

	/**Ϊ���������ռ�**/
	pstWriteAviIndex = (WRITE_AVI_INDEX_S *)malloc(sizeof(WRITE_AVI_INDEX_S));
	if(NULL==pstWriteAviIndex)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	/**��Ƶ����**/
	stPara.bVenc        = true;  
	stPara.u32Width     = 640;
	stPara.u32Height	= 360;	
	stPara.u32Gop		= 90;
	stPara.u64fps		= 25;
	stPara.pCompressor  = "h264";
	
	/**��Ƶ����**/
	stPara.bAenc 		= true; //��ʹ����Ƶ 
	stPara.u32Format 	= 1;
	stPara.u32Rate		= 44100;  
	stPara.u32Channels  = 2;
	stPara.u32Bits		= 16; 
	stPara.u32Mp3Rate	= 0;
	stPara.u32AVChanNum = 2;

	/**!!�����Ƿ�ʹ������ !!**/
	stPara.s8HasIndex = 1; 

	/**��ʼ��AVI�ļ�ͷ�ṹ**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	l_pu8Buff = (unsigned char*)malloc(MAX_VIDEO_SIZE);
	if(NULL==l_pu8Buff)
	{
		printf("%s %d malloc error \n",__FILE__,__LINE__);
		return -1;
	}
	
	while(1)
	{
		/**��һ֡��Ƶ����**/
		l_DateLen = ReadOneFrameVideo(l_pstNalu);
		if(l_DateLen > 0)
		{
			/**������Ƶ��ǩ**/
			memcpy(l_pu8Buff,AVI_VIDEO_FLAG,4);

			/**���ݶ���**/
			l_u32WriteLen = AVICalcAlign(l_DateLen,4);

			/**���ó���**/
			l_pu8Buff[4] = (l_u32WriteLen)&0xff;
			l_pu8Buff[5] = (l_u32WriteLen>> 8)&0xff;
			l_pu8Buff[6] = (l_u32WriteLen>>16)&0xff;
			l_pu8Buff[7] = (l_u32WriteLen>>24)&0xff;
			
			/**ʵ��Ӧ��д��ĳ���**/
			l_u32WriteLen += 8 ;   /**4ByteFlag + 4ByteLen**/
			memcpy(l_pu8Buff + 8,l_pstNalu->buf,l_DateLen);
			
			l_s32Ret = fwrite(l_pu8Buff,1,l_u32WriteLen,g_out_avi_fd);
			if(l_s32Ret!=l_u32WriteLen)
			{
				printf("%s %d write file error \n",__FILE__,__LINE__);
			}

			l_stIndexEntry.u32Flags = 0x00;
			/**�ж��Ƿ�Ϊ�ؼ�֡ �ؼ�֡Ϊ0x10 ����Ϊ0x00   **/
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
		
		/**����֡��Ƶ����**/
		for(int i=0;i<2;i++)
		{
			l_u32AudioLen = 3528;
			l_DateLen = ReadOneFrameAudio(&l_arru8Audio[8],l_u32AudioLen);
			if(l_DateLen > 0)
			{
				/**������Ƶ��ǩ**/
				memcpy(l_arru8Audio,AVI_AUDIO_FLAG,4);
				/**���ݶ���**/
				l_u32AudioLen = AVICalcAlign(l_DateLen,4);
				/**���ó���**/
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

	/**����AVI�ļ�ͷ�ṹ�еĳ���ֵ**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);

	stPara.u32MoviLen = l_u32AVIWriteLen;
	
	/**д������AVI�ļ���ȥ***/
	RIFF_WriteAviIndexToFile(pstWriteAviIndex,g_out_avi_fd);

	/**����AVI�ļ�ͷ�ṹ�еĳ���ֵ**/
	l_u32AVIWriteLen = ftell(g_out_avi_fd);
	/**RIFF�ļ��еĶ�**/
	stPara.u32AviLen = l_u32AVIWriteLen - 8; /**4ByteRIFF + 4ByteLen**/

	/**д�뵽ͷ�ļ���**/
	RIFF_WriteAviHeader(stPara,g_out_avi_fd);

	/**�ر���Դ**/
	CloseVideoStream(l_pstNalu);
	CloseAudioStream();

	return 0;
}



int main(void)
{
	int l_s32Ret = 0;
	
	printf("Hello World \n");

	/**h246�ļ�����**/
	//l_s32Ret = TestH264ReadAndWrite();

	/**ֻ��װ��Ƶ����*/
	//l_s32Ret = TestH264ToAVI();

	/**ͬʱ��װ��Ƶ����Ƶ����**/
	//l_s32Ret = TestH264AndPCMToAVI();

	/**�������**/
	l_s32Ret = TestAVIIndex();
	
	return 0;
}


