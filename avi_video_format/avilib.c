/************************************************************
*FileName: avilib.c
*Date:	   2018-07-15
*Author:   Wen Lee
*Version:  V1.0
*Description: AVI 音视频格式封装
*Others:   
*History:	
***********************************************************/
#include "avilib.h"

static AVI_S g_stAVIPara;
static WAV_S g_stWAVPara;

static void Int2Str(unsigned char *dst, int n)
{
    dst[0] = (n)&0xff;
    dst[1] = (n>> 8)&0xff;
    dst[2] = (n>>16)&0xff;
    dst[3] = (n>>24)&0xff;
    return;
}

#define OUT4CC(s) \
    if(s64Nhb<=HEADERBYTES-4) memcpy(arrAVI_header+s64Nhb,s,4); s64Nhb += 4

#define OUTLONG(n) \
    if(s64Nhb<=HEADERBYTES-4) Int2Str(arrAVI_header+s64Nhb,n); s64Nhb += 4

#define OUTSHRT(n) \
    if(s64Nhb<=HEADERBYTES-2) { \
      arrAVI_header[s64Nhb  ] = (n   )&0xff; \
      arrAVI_header[s64Nhb+1] = (n>>8)&0xff; \
    } \
    s64Nhb += 2

typedef struct IDRIndex
{
	short s16IdrNo;
	char  arrBCDTime[6];
	int   s32OffsetAddr;
	char  arrResv[4];
}__attribute__ ((packed))IDR_INDEX_S;

int HstAviHeaderWrite(void)
{
	long s64Nhb;
	//int s32Strl_start;
	int s32Hdrl_start, s32Audio_strl_start;
	int  s32Njunk, s32Sampsize, s32HasIndex, s32Ms_per_frame, s32Frate, s32Idxerror, s32Flag;
	unsigned char arrAVI_header[HEADERBYTES];
	unsigned int  l_u32AVILen = 0;
	unsigned int j = 0;
	
    if(g_stAVIPara.u32AviLen > 0)
    {
	    l_u32AVILen = g_stAVIPara.u32AviLen;
    }  
    else
    {
		l_u32AVILen = 0;
	}
        
    s32Idxerror = 0;
    s32HasIndex =0;
    
    if(g_stAVIPara.u32MustUseIndex ==1)
    {
        s32HasIndex =1;
    }
    
    if(g_stAVIPara.u64fps < 0.001)
    {
        s32Frate=0;
        s32Ms_per_frame=0;
    }
    else
    {
        s32Frate = (int) (FRAME_RATE_SCALE*g_stAVIPara.u64fps + 0.5);
        s32Ms_per_frame=(int) (1000000/g_stAVIPara.u64fps + 0.5);
    }
    s64Nhb = 0;
    
    /* The RIFF header */
    OUT4CC ("RIFF");
    OUTLONG(l_u32AVILen);    			/* # of bytes to follow */
    OUT4CC ("AVI ");
	
    /* Start the header list */
    OUT4CC ("LIST");
    OUTLONG(0);        					/* Length of list in bytes, don't know yet */
    s32Hdrl_start = s64Nhb;  			/* Store start position */
    OUT4CC ("hdrl");

    /* The main AVI header */
    /* The Flags in AVI File header*/
    OUT4CC ("avih");
    OUTLONG(56);                 		/* # of bytes to follow */
    OUTLONG(s32Ms_per_frame);           /* Microseconds per frame */
    OUTLONG(0);
	/**Specifies the alignment for data, in bytes.
	Pad the data to multiples of this value.**/ 
    OUTLONG(g_stAVIPara.u32Gop);       
	s32Flag = AVIF_ISINTERLEAVED;
    if(s32HasIndex) s32Flag |= AVIF_HASINDEX;
    if(s32HasIndex && g_stAVIPara.u32MustUseIndex) s32Flag |= AVIF_TRUSTCKTYPE;
    OUTLONG(s32Flag);                    /* Flags */
	OUTLONG(g_stAVIPara.u32VideoFrames); /* Vedio TotalFrames */
    OUTLONG(0);                  		 /* InitialFrames */
    OUTLONG(g_stAVIPara.u32StreamNum);	 /**Be Careful ,streams = 1**/
	OUTLONG(0);                  		 /* SuggestedBufferSize */
    OUTLONG(g_stAVIPara.u32Width);       /* Width */
    OUTLONG(g_stAVIPara.u32Height);      /* Height */
                                 		/* MS calls the following 'reserved': */
    OUTLONG(0);                  		/* TimeScale:  Unit used to measure time */
    OUTLONG(0);                  		/* DataRate:   Data u32Rate of playback     */
    OUTLONG(0);                  		/* StartTime:  Starting time of AVI data */
    OUTLONG(0);                  		/* DataLength: Size of AVI data chunk    */

    OUT4CC ("LIST");
    OUTLONG(116);                		/* Length of list in bytes */
    //s32Strl_start = s64Nhb;      		/* Store start position */
    OUT4CC ("strl");
    OUT4CC ("strh");
    OUTLONG(56);                 		/* # of bytes to follow */
    OUT4CC ("vids");             		/* Type */
    OUT4CC (g_stAVIPara.arrCompressor); /* Handler */
    OUTLONG(0);                			/* Flags */
    OUTLONG(0);                  		/* Reserved, MS says: wPriority, wLanguage */
    OUTLONG(0);                  		/* InitialFrames */
    OUTLONG(FRAME_RATE_SCALE);   		/* Scale */
    OUTLONG(s32Frate);           		/* Rate: Rate/Scale == samples/second */
	OUTLONG(0);                  		/* Start */
    OUTLONG(g_stAVIPara.u32VideoFrames); /* Length */
    OUTLONG(0);                  		/* SuggestedBufferSize */
    OUTLONG(-1);                 		/* Quality */
    OUTLONG(0);                  		/* SampleSize */
    OUTLONG(0);                  		/* Frame */
    OUTLONG(0);                  		/* Frame */                            
	/* The video stream u32Format */
    OUT4CC ("strf");
    OUTLONG(40);                 		/* # of bytes to follow */
    OUTLONG(40);                 		/* Size */
    OUTLONG(g_stAVIPara.u32Width);      /* Width */
    OUTLONG(g_stAVIPara.u32Height);     /* Height */
    OUTSHRT(1);
    OUTSHRT(24);                 		/* Planes, Count */
    OUT4CC (g_stAVIPara.arrCompressor); /* Compression */
                                       
    OUTLONG(g_stAVIPara.u32Width*g_stAVIPara.u32Height);  /* SizeImage */
    OUTLONG(0);                  /* XPelsPerMeter */
    OUTLONG(0);                  /* YPelsPerMeter */
    OUTLONG(0);                  /* ClrUsed: Number of colors used */
    OUTLONG(0);                  /* ClrImportant: Number of colors important */

    for(j=0; (j+1)<=g_stAVIPara.u32ANum;j++)
    {
        //printf("音频设置\n");
        s32Sampsize = g_stAVIPara.stTrack[j].u32AFmt==0x1?s32Sampsize*4:s32Sampsize;

        OUT4CC ("LIST");
        OUTLONG(88);                 /* Length of list in bytes, don't know yet */
        s32Audio_strl_start = s64Nhb;      /* Store start position */
        OUT4CC ("strl");

        OUT4CC ("strh");
        OUTLONG(56);                 /* # of bytes to follow */
        OUT4CC ("auds");             // 4字节，表示数据流的种类,vids表示视频数据流，auds音频数据流
        OUT4CC(" ");                 // 4字节，表示数据流解压缩的驱动程序代号
      	//OUT4CC("0000"); 
        OUTLONG(0);                  //数据流属性
        OUTLONG(0);                  //此数据流的播放优先级
        OUTLONG(0);               	 //说明在开始播放前需要多少帧
        OUTLONG(1);               //数据量，视频每帧的大小或者音频的采样大小
        OUTLONG(g_stAVIPara.stTrack[j].u32ARate);         //dwScale/dwRate=每秒的采样数

		OUTLONG(0);                                           //数据流开始播放的位置，以dwScale为单位
        OUTLONG(g_stAVIPara.u32AudioFrames);                  //数据流的数据量，以dwScale为单位
        OUTLONG(12288);                                      /* SuggestedBufferSize */
        OUTLONG(-1);                                         //解压缩质量参数，值越大，质量越好
        OUTLONG(2);
        OUTLONG(0);                                          //音频的采样大小
        OUTLONG(0);                							 /* Frame */

        OUT4CC ("strf");
        OUTLONG(16);                            			/* # of bytes to follow */
        OUTSHRT(g_stAVIPara.stTrack[j].u32AFmt);            /* Format */
        OUTSHRT(g_stAVIPara.stTrack[j].u32AChans);          /* Number of u32Channels */
        OUTLONG(g_stAVIPara.stTrack[j].u32ARate);           /* SamplesPerSec */
        OUTLONG(g_stAVIPara.stTrack[j].u32ARate*g_stAVIPara.stTrack[j].u32ABits/8*g_stAVIPara.stTrack[j].u32AChans);
        OUTSHRT(4);                             			/* BlockAlign */
        OUTSHRT(g_stAVIPara.stTrack[j].u32ABits);           /* BitsPerSample */
        Int2Str(arrAVI_header+s32Audio_strl_start-4,s64Nhb-s32Audio_strl_start);

    }
    Int2Str(arrAVI_header+s32Hdrl_start-4,s64Nhb-s32Hdrl_start);
    s32Njunk = HEADERBYTES - s64Nhb - 8 - 12;

    if(s32Njunk<=0)
    {
        return -1;
    }

    OUT4CC ("JUNK");
    OUTLONG(s32Njunk);
	
	memset(arrAVI_header+s64Nhb,0,s32Njunk);
    s64Nhb += s32Njunk;

    OUT4CC ("LIST");
    
    if(g_stAVIPara.u32MoviLen >= HEADERBYTES)
    {
        OUTLONG(g_stAVIPara.u32MoviLen-HEADERBYTES+4);
    }
    else
    {
        OUTLONG(0);
    }
    OUT4CC ("movi");
    if(g_stAVIPara.f_fp ==NULL)
        return -1;
    if(g_stAVIPara.f_fp!= NULL)
    {
        fseek(g_stAVIPara.f_fp,0,SEEK_SET);
        if(1!=fwrite ((char *)arrAVI_header,HEADERBYTES,1,g_stAVIPara.f_fp))
        {
            return -1;
        }
    }
    if(s32Idxerror) 
    {
      return -1;
    }
    return 0;
}


int RIFF_WriteAviHeader(AVI_PARA_S stPara,FILE *fp)
{
	int l_ret = 0;

	g_stAVIPara.f_fp	    	= fp;
	g_stAVIPara.u32AviLen  		= stPara.u32AviLen;
	g_stAVIPara.u32MoviLen 		= stPara.u32MoviLen;
	g_stAVIPara.u32Width   		= stPara.u32Width;
	g_stAVIPara.u32Height 		= stPara.u32Height;
	g_stAVIPara.u64fps     		= stPara.u64fps;
	g_stAVIPara.u32Gop     		= stPara.u32Gop;

	/**audio and video stream num **/
	if((true==stPara.bAenc)&&(true==stPara.bVenc))
	{
		g_stAVIPara.u32StreamNum = stPara.u32Channels;
	}else
	{
		g_stAVIPara.u32StreamNum = 1;
	}

	g_stAVIPara.u32ANum  = 1;    /**It must be careful here**/

	g_stAVIPara.u32APtr  = 0;
	g_stAVIPara.stTrack[g_stAVIPara.u32APtr].u32AChans  = stPara.u32Channels;
	g_stAVIPara.stTrack[g_stAVIPara.u32APtr].u32ARate   = stPara.u32Rate;
	g_stAVIPara.stTrack[g_stAVIPara.u32APtr].u32ABits   = stPara.u32Bits;
	g_stAVIPara.stTrack[g_stAVIPara.u32APtr].u32AFmt    = stPara.u32Format;
	g_stAVIPara.stTrack[g_stAVIPara.u32APtr].u32Mp3Rate = stPara.u32Mp3Rate;

	g_stAVIPara.u32MustUseIndex = stPara.s8HasIndex;    /**must use index**/
	 if(strncmp(stPara.pCompressor, "RGB", 3)==0)
    {
        memset(g_stAVIPara.arrCompressor, 0, 4);
    }
    else
    {
        memcpy(g_stAVIPara.arrCompressor,stPara.pCompressor,4);
    }

	l_ret = HstAviHeaderWrite();

	return l_ret;
}


int RIFF_WriteAviIndex(AVI_INDEX_ENTRY_S stIndexEntry, WRITE_AVI_INDEX_S *pstWriteAviIndex)
{
	static unsigned int l_staticu32Offset = 0;
	
	if(0 == pstWriteAviIndex->IndexNum)
	{
		pstWriteAviIndex->stAviIndex.u32Fcc = 0x69647831;      /**idx1**/
		pstWriteAviIndex->stAviIndex.u32Indexlen = 0;
		l_staticu32Offset = 4;  /***idx1 Use 4 bytes **/
		g_stAVIPara.u32VideoFrames = 0;
		g_stAVIPara.u32AudioFrames = 0;
	}

	if(0x63643030==stIndexEntry.u32ChunkId) /**video**/
	{
		g_stAVIPara.u32VideoFrames++;
	}
	else if(0x64773130==stIndexEntry.u32ChunkId)/**audio**/
	{
		g_stAVIPara.u32AudioFrames++;
	}

	/**4Byte ID + 4ByteFlags + 4ByteOffset + 4ByteSize**/
	pstWriteAviIndex->stAviIndex.u32Indexlen += 16;	
	if(pstWriteAviIndex->IndexNum < INDEX_MAX_NUM)
	{
		pstWriteAviIndex->stAviIndex.arrIndex[pstWriteAviIndex->IndexNum].u32ChunkId = stIndexEntry.u32ChunkId;
		pstWriteAviIndex->stAviIndex.arrIndex[pstWriteAviIndex->IndexNum].u32Flags	 = stIndexEntry.u32Flags;
		pstWriteAviIndex->stAviIndex.arrIndex[pstWriteAviIndex->IndexNum].u32Offset  = l_staticu32Offset;
		pstWriteAviIndex->stAviIndex.arrIndex[pstWriteAviIndex->IndexNum].u32Size	 = stIndexEntry.u32Size;
		pstWriteAviIndex->IndexNum ++;

		/**Offset Value = u32Size Value + u32ChunkId 4Byte and u32Size 4Byte !!**/
		l_staticu32Offset += stIndexEntry.u32Size + 8;
	}else
	{
		printf("%s %d Index table is full IndexNum = %d \n",__FILE__,__LINE__,pstWriteAviIndex->IndexNum);
		return -1;
	}

	return 0;

}

int RIFF_WriteAviIndexToFile(WRITE_AVI_INDEX_S *pstWriteAviIndex,FILE* pFp)
{
	int 		  l_s32Ret = 0;
	unsigned int  l_u32WriteLen = 0;
	unsigned char l_arrBuf[4] = {0};

	/**Fill Fcc**/
	l_arrBuf[3]	 = (pstWriteAviIndex->stAviIndex.u32Fcc)&0xff;
	l_arrBuf[2]	 = (pstWriteAviIndex->stAviIndex.u32Fcc>>8)&0xff;
	l_arrBuf[1]	 = (pstWriteAviIndex->stAviIndex.u32Fcc>>16)&0xff;
	l_arrBuf[0]	 = (pstWriteAviIndex->stAviIndex.u32Fcc>>24)&0xff;
	l_u32WriteLen = 4;
	l_s32Ret = fwrite(l_arrBuf,1,l_u32WriteLen,pFp);
	if((unsigned int)l_s32Ret!=l_u32WriteLen)
	{
		printf("%s %d Write File Error \n",__FILE__,__LINE__);
		return -1;
	}

	/**Fill Len**/
	l_arrBuf[0]	 = (pstWriteAviIndex->stAviIndex.u32Indexlen)&0xff;
	l_arrBuf[1]	 = (pstWriteAviIndex->stAviIndex.u32Indexlen>>8)&0xff;
	l_arrBuf[2]	 = (pstWriteAviIndex->stAviIndex.u32Indexlen>>16)&0xff;
	l_arrBuf[3]	 = (pstWriteAviIndex->stAviIndex.u32Indexlen>>24)&0xff;
	l_u32WriteLen = 4;
	l_s32Ret = fwrite(l_arrBuf,1,l_u32WriteLen,pFp);
	if((unsigned int)l_s32Ret!=l_u32WriteLen)
	{
		printf("%s %d Write File Error \n",__FILE__,__LINE__);
		return -1;
	}

	/**Fill Index**/
	l_u32WriteLen = sizeof(AVI_INDEX_ENTRY_S)* (pstWriteAviIndex->IndexNum);
	l_s32Ret = fwrite(pstWriteAviIndex->stAviIndex.arrIndex,1,l_u32WriteLen,pFp);
	if((unsigned int)l_s32Ret!=l_u32WriteLen)
	{
		printf("%s %d Write File Error \n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

