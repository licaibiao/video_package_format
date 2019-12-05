/************************************************************
*FileName: MediaStream.c
*Date:     2018-07-15
*Author:   Wen Lee
*Version:  V1.0
*Description: 模拟音频流和视频流数据
*Others:   
*History:   
***********************************************************/

#include "MediaStream.h"

static int g_s32Info2 = 0;
static int g_s32Info3 = 0; 

static FILE *g_vedio_fd = NULL;
static FILE *g_audio_fd = NULL;

NALU_S *AllocNALU(int s32Buffersize)  
{  
	NALU_S *l_pstNalu;  
	
	if ((l_pstNalu = (NALU_S*)calloc (1, sizeof (NALU_S))) == NULL)  
	{  
		printf("AllocNALU: n");  
		exit(0);  
	} 
	
	l_pstNalu->max_size = s32Buffersize;  
	if((l_pstNalu->buf = (char*)calloc (s32Buffersize, sizeof (char))) == NULL)  
	{  
		free (l_pstNalu);  
		printf ("AllocNALU: n->buf");  
		exit(0);  
	} 
	
	return l_pstNalu;  
}  

void FreeNALU(NALU_S *pstNalu)  
{  
	if (pstNalu)  
	{  
		if (pstNalu->buf)  
		{  
			free(pstNalu->buf);  
			pstNalu->buf=NULL;  
		}  
		free (pstNalu);  
	}  
}  

/**判断是否为0x000001,如果是返回1**/  
static int FindStartCode2 (unsigned char *pu8Buf)  
{  
	if(pu8Buf[0]!=0 || pu8Buf[1]!=0 || pu8Buf[2] !=1)
	{
		return 0;
	} 
	else
	{
		return 1;
	}  
}  

/**判断是否为0x00000001,如果是返回1**/ 
static int FindStartCode3 (unsigned char *pu8Buf)  
{  
	if(pu8Buf[0]!=0 || pu8Buf[1]!=0 || pu8Buf[2] !=0 || pu8Buf[3] !=1)
	{
		return 0; 
	}
	else
	{
		return 1;
	}  
} 

/**
这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，
获取他的长度，填充F,IDC,TYPE位。并且返回两个开始字符之间间隔的字节数，
即包含有前缀的NALU的长度前缀之后的第一个字节为 NALU_HEADER 
**/
int GetAnnexbNALU (NALU_S *nalu)  
{  
	int l_s32Pos = 0;  
	int l_s32Rewind; 
	int l_s32StartCodeFound;
	unsigned char *l_pu8Buf;  

	l_pu8Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char));
	if (NULL==l_pu8Buf)
	{
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");  
	}
		
	//printf("nalu->max_size=%d\n",(int)nalu->max_size);
	
	memset(l_pu8Buf,0,nalu->max_size);

	/**初始化码流序列的开始字符为3个字节  **/
	nalu->startcodeprefix_len = 3;
	
	/**从码流中读3个字节  **/
	if (3 != fread (l_pu8Buf, 1, 3, g_vedio_fd))
	{  
		free(l_pu8Buf);  
		return 0;  
	} 
	
	/**判断是否为0x000001   **/
	g_s32Info2 = FindStartCode2 (l_pu8Buf);

	/**如果不是，再读一个字节  **/
	if(g_s32Info2 != 1)   
	{  
		/**读一个字节**/
		if(1 != fread(l_pu8Buf+3, 1, 1, g_vedio_fd))  
		{  
			free(l_pu8Buf);  
			return 0;  
		}

		/**判断是否为0x00000001  **/
		g_s32Info3 = FindStartCode3 (l_pu8Buf);
		
		/**如果不是，返回-1**/  
		if (g_s32Info3 != 1)
		{   
			free(l_pu8Buf);  
			return -1;  
		}  
		else   
		{  
			/**如果是0x00000001,得到开始前缀为4个字节**/  
			l_s32Pos = 4;  
			nalu->startcodeprefix_len = 4;  
		}  
	}   
	else  
	{  
		/**如果是0x000001,得到开始前缀为3个字节**/  
		nalu->startcodeprefix_len = 3;  
		l_s32Pos = 3;  
	} 
	
	/**查找下一个开始字符的标志位**/  
	l_s32StartCodeFound = 0;  
	g_s32Info2 = 0;  
	g_s32Info3 = 0;      
	
	while (!l_s32StartCodeFound)  
	{  
		/**判断是否到了文件尾**/ 
		if (feof (g_vedio_fd)) 
		{  
			nalu->len = (l_s32Pos-1);  
			printf("nalu->len1=%d\n",nalu->len );			
			/**拷贝一个完整NALU，*/
			memcpy (nalu->buf, l_pu8Buf, nalu->len);       
			free(l_pu8Buf);  
			return nalu->len;  
		}  

		/**读一个字节到BUF中**/
		l_pu8Buf[l_s32Pos++] = fgetc (g_vedio_fd);  

		/**判断是否为0x00000001 **/
		g_s32Info3 = FindStartCode3(&l_pu8Buf[l_s32Pos-4]); 
		if(g_s32Info3 != 1) 
		{
			/**判断是否为0x000001**/
			g_s32Info2 = FindStartCode2(&l_pu8Buf[l_s32Pos-3]);  	
		}	
		
		l_s32StartCodeFound = (g_s32Info2 == 1 || g_s32Info3 == 1);  
	}  
	
	l_s32Rewind = (g_s32Info3 == 1) ? -4 : -3;  

	/**把文件指针指向前一个NALU的末尾**/
	if (0 != fseek (g_vedio_fd, l_s32Rewind, SEEK_CUR))  
	{  
		free(l_pu8Buf);  
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");  
	}  

	nalu->len = l_s32Pos+l_s32Rewind; 
	memcpy (nalu->buf, l_pu8Buf, nalu->len);
	free(l_pu8Buf); 
	
	/**返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度**/  
	return (l_s32Pos+l_s32Rewind);
}  

int OpenVideoStream(void)
{
	g_vedio_fd = fopen(VIDEO_FILE_NAME,"r");
	if(NULL==g_vedio_fd)
	{
		printf("%s %d fopen error \n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

int OpenAudioStream(void)
{
	g_audio_fd = fopen(AUDIO_FILE_NAME,"r");
	if(NULL==g_vedio_fd)
	{
		printf("%s %d fopen error \n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

int ReadOneFrameVideo(NALU_S *pstNalu)
{
	int l_s32Ret = 0;
		
	l_s32Ret = GetAnnexbNALU (pstNalu);

	if(feof(g_vedio_fd))
	{
		l_s32Ret = 0;
		printf("read end of file: %s\n",VIDEO_FILE_NAME);
	}
	
	return l_s32Ret;
}

NALU_S *InitVideoStream(int s32Len)
{
	int l_s32Ret = 0;
	NALU_S *l_pstNalu = NULL;  
	
	l_s32Ret = OpenVideoStream();
	if(0!=l_s32Ret)
	{
		return NULL;
	}

	l_pstNalu = AllocNALU(s32Len);
	
	return l_pstNalu;
}



int CloseVideoStream(NALU_S *pstNalu)
{
	if(NULL!=g_vedio_fd)
	{
		fclose(g_vedio_fd);
	}

	FreeNALU(pstNalu);
	
	return 0;
}


int InitAudioStream(void)
{
	return OpenAudioStream();
}

int CloseAudioStream()
{
	if(NULL!=g_audio_fd)
	{
		fclose(g_audio_fd);
	}

	return 0;
}


int ReadOneFrameAudio(unsigned char *pu8AudioData,unsigned int u32Len)
{
	unsigned int l_u32Len = 0;
	
	if(feof (g_audio_fd))
	{
		printf("audio file read end \n");
		return -1;
	}

	l_u32Len = fread(pu8AudioData, 1, u32Len, g_audio_fd);

	return l_u32Len;		
}

