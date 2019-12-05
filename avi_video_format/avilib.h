/************************************************************
*FileName: avilib.h
*Date:	   2018-07-15
*Author:   Wen Lee
*Version:  V1.0
*Description: AVI 音视频格式封装
*Others:   
*History:	
***********************************************************/

#ifndef HstAvilib_H
#define HstAvilib_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_AVI_CHNUM  			8
#define HEADERBYTES 			1024//8192//1024
#define MAX_VIDEOFILE_SIZE_KS 	((128*1024*1024)) 
#define MAX_VIDEOFILE_SIZE 		((64*1024*1024)) 
//#define FRAME_RATE_SCALE 1000000
#define FRAME_RATE_SCALE		1
#define AVI_MAX_TRACKS 			8
#define MAX_WAV_TRACKS 			8
#define	MAX_WAV_CHNUM			8
#define WAV_HEAD_SIZE 			80
#define WAV_FILE_HEAD_SIZE 		128
#define	MAX_AUDIOFILE_SIZE 		(5*1024*1024) 	//5M
#define WAV_HEAD_UPDATE 		1
#define WAV_WRITE_ERR_SPACE 	2

#define INDEX_MAX_NUM   		4096*40

#define AVIF_HASINDEX           0x00000010      /* Index at end of file */
#define AVIF_MUSTUSEINDEX       0x00000020
#define AVIF_ISINTERLEAVED      0x00000100
#define AVIF_TRUSTCKTYPE        0x00000800      /* Use CKType to find key frames */
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVIF_COPYRIGHTED        0x00020000
#define AVI_HEAD_UPDATE			1

typedef struct 
{
    unsigned int    u32AFmt;             /* Audio u32Format, see #defines below */
    unsigned int    u32AChans;           /* Audio u32Channels, 0 for no audio */
    unsigned int    u32ARate;            /* Rate in Hz */
    unsigned int    u32ABits;            /* u32Bits per audio sample */
    unsigned int    u32Mp3Rate;          /* mp3 bitrate kbs*/
    unsigned int    u32AudioStrNum;      /* Audio stream number */
    unsigned int    u32AudioBytes;       /* Total number of bytes of audio data */
    unsigned int    u32AudioChunks;      /* Chunks of audio data in the file */
    char            arrAudioTag[4];      /* Tag of audio data */
    unsigned int    u32AudioPosc;        /* Audio position: chunk */
    unsigned int    u32AudioPosb;        /* Audio position: byte within chunk */
    unsigned int    u32ACodeChOff;       /* absolut offset of audio codec information */
    unsigned int    u32ACodeCfOff;       /* absolut offset of audio codec information */
} TRACK_S;

typedef struct
{   
	FILE        	*f_fp;
	char        	arrCompressor[8];     /* Type of compressor, 4 bytes + padding for 0 byte */
	unsigned int	u32Pos;               /* position in file */
	unsigned int	u32MustUseIndex;      /* Flag if frames are duplicated */
	unsigned int    u32StreamNum;		  /* total number of stream(audio and video)*/
	unsigned int	u32ANum;              // total number of audio tracks
	unsigned int	u32APtr;              // current audio working track
	unsigned int	u32Mutimediaid;       // current audio working track
	unsigned int	u32IndexOffs;
	unsigned int	u32NIdx;
	unsigned int	u32BodyFf;
	unsigned int	u32Width;             /* Width  of a video frame */
	unsigned int	u32Height;            /* Height of a video frame */
	unsigned int	u32FDes;              /* File descriptor of AVI file */
	unsigned int	u32VideoStrNum;       /* Video stream number */
	unsigned int	u32VideoFrames;       /* Number of video frames */
	unsigned int	u32VideoPos;          /* Number of next frame to be read*/
	unsigned int	u32AudioFrames;       /* Number of audio frames  */
	unsigned int	u32MaxLen;            /* maximum video chunk present */
	double      	u64fps;               /* Frames per second */
	unsigned int	u32Gop;
	bool        	bUseing;
	unsigned int	u32Wtimes;
	TRACK_S     	stTrack[AVI_MAX_TRACKS];      // up to AVI_MAX_TRACKS audio tracks supported
	unsigned int	u32ClearFlag;		
	unsigned int    u32AviLen;			  /**add by Caibiao Lee**/
	unsigned int    u32MoviLen;			  /**add by Caibiao Lee**/
	char        	arrszIndex[64];
} AVI_S;


typedef struct
{
    bool         bVenc;
    unsigned int u32Width; 
    unsigned int u32Height; 
    double       u64fps; 
    const char  *pCompressor;
    bool         bAenc;
    unsigned int u32Channels;
	unsigned int u32AVChanNum;
    unsigned int u32Rate; 
    unsigned int u32Bits; 
    unsigned int u32Format;
    unsigned int u32Mp3Rate;
    unsigned int u32Gop;
	unsigned int u32AviLen;
	unsigned int u32MoviLen;
	char		 s8HasIndex;
}AVI_PARA_S;



typedef struct
{
	unsigned int u32ChunkId; /**Four-character code characterizing this block '00dc' '01wb'**/
	unsigned int u32Flags;   /**Description This data block is not a key frame IDR = 0x10 other = 0x00**/
	unsigned int u32Offset;  /**The offset of this data block After the header file**/
	unsigned int u32Size;    /**The size of this data block**/
}AVI_INDEX_ENTRY_S;


typedef struct 
{
	unsigned int 	    u32Fcc;				/**"idx1" 0x69647831**/
	unsigned int  		u32Indexlen;		/**The len of this Index **/
	AVI_INDEX_ENTRY_S   arrIndex[INDEX_MAX_NUM];
}AVI_INDEX_S;

typedef struct 
{
	AVI_INDEX_S stAviIndex;
	unsigned int IndexNum;  /**the serial number of the index**/
}WRITE_AVI_INDEX_S;



typedef struct
{   
    char   			arrs8Compressor[8];   /* Type of compressor, 4 bytes + padding for 0 byte */
    unsigned int 	u32Pos;               /* position in file */
    unsigned int 	u32HeaderPos;                     
    unsigned int 	u32DataLen;                       
    unsigned int 	u32Must_use_index;    /* Flag if frames are duplicated */
    unsigned int 	u32ANum;              // total number of audio tracks
    unsigned int 	u32Aptr;              // current audio working track
    unsigned int 	u32Mutimediaid;       // current audio working track
    unsigned int    u32Indexoffs;
    unsigned int    u32N_idx;
    unsigned int    u32Body_ff;
    unsigned int   	u32Audio_frames;       /* Number of video frames */
    bool 			bUseing;
    unsigned int 	u32Wtimes;
    TRACK_S 		stTrack[MAX_WAV_TRACKS];        // up to MAX_WAV_TRACKS audio tracks supported
} WAV_S;

typedef struct
{
    const char 		*pu8Compressor;
    bool 			bAenc;
    unsigned int 	u32Channels;
    unsigned int 	u32Rate; 
    unsigned int 	u32Bits; 
    unsigned int 	u32Format;
    unsigned int 	u32Mp3rate;
}WAVPARA_S;

int RIFF_WriteAviHeader(AVI_PARA_S stPara,FILE *fp);
int RIFF_WriteAviFile(unsigned char *ptData,unsigned int u32Len,unsigned char u8Type);
int RIFF_WriteAviIndex(AVI_INDEX_ENTRY_S stIndexEntry, WRITE_AVI_INDEX_S *pstWriteAviIndex);
int RIFF_WriteAviIndexToFile(WRITE_AVI_INDEX_S *pstWriteAviIndex,FILE* pFp);

int RIFF_WavHeaderWrite(FILE *pf,unsigned int u32DataLen);
int RIFF_InitWavWriteStruct(WAVPARA_S stPara, unsigned char u8FileHeadFlag);

#endif
