#include <iostream>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#define read(fd,buf,count)  fread((void*)(buf),(size_t)(1),(size_t)(count),(FILE*)(fd))
#define write(fd,buf,count)  fwrite((const void*)(buf),(size_t)(1),(size_t)(count),(FILE*)(fd))
#define lseek(fildes,offset,whence)  fseek((FILE*)(fildes),(long)(offset),(int)(whence))
#define close(fd)  fclose((FILE*)(fd))
#define strncasecmp(str1,str2,maxlen) strnicmp((char*)(str1),(char*)(str2),(unsigned int)(maxlen))
#else
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#define strncasecmp(str1,str2,maxlen) strncasecmp((const char*)(str1),(const char*)(str2),(unsigned int)(maxlen))
#endif

#include <string.h>
#include "AviFmtInterface.h"

/*******************************************************************
 *                                                                 *
 *    Utilities for writing an AVI File                            *
 *                                                                 *
 *******************************************************************/

/* AVI_MAX_LEN: The maximum length of an AVI file, we stay a bit below
    the 2GB limit (Remember: 2*10^9 is smaller than 2 GB) */

#define AVI_MAX_LEN 2000000000

/* HEADERBYTES: The number of bytes to reserve for the header */

#define HEADERBYTES 2048

#define AVIIF_KEYFRAME  0x00000010     // 索引里面的关键帧标志 

#define PAD_EVEN(x) ( ((x)+1) & ~1 )


/* Copy n into dst as a 4 byte, little endian number.
   Should also work on big endian machines */

static void long2str(unsigned char *dst, int n)
{
   dst[0] = (n    )&0xff;
   dst[1] = (n>> 8)&0xff;
   dst[2] = (n>>16)&0xff;
   dst[3] = (n>>24)&0xff;
}

/* Convert a string of 4 or 2 bytes to a number,
   also working on big endian machines */

static unsigned long str2ulong(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24) );
}
static unsigned long str2ushort(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) );
}

CAviFmtInterface::CAviFmtInterface()
{
	m_avi = NULL;
	AVI_errno = 0;
	m_pAviFormatFrameBuf = NULL;
	m_MaxAviFormatFrameLen = 0;
}

CAviFmtInterface::~CAviFmtInterface()
{
	if(m_pAviFormatFrameBuf!=NULL)
	{
		free(m_pAviFormatFrameBuf);
		m_pAviFormatFrameBuf=NULL;
	}
}

/* Calculate audio sample size from number of bits and number of channels.
   This may have to be adjusted for eg. 12 bits and stereo */

int CAviFmtInterface::avi_sampsize()
{
   int s;
   s = ((m_avi->a_bits+7)/8)*m_avi->a_chans;
   if(s==0) s=1; /* avoid possible zero divisions */
   return s;
}

/* Add a chunk (=tag and data) to the AVI file,
   returns -1 on write error, 0 on success */

bool CAviFmtInterface::avi_add_chunk(char *tag, const unsigned char *data, int length)
{
   unsigned char c[8];

   /* Copy tag and length int c, so that we need only 1 write system call
      for these two values */

   //Noted by cwl,2014.5.27
   //写入的长度是真实值，但写入数据需要对齐
   memcpy(c,tag,4);
   long2str(c+4,length);

   /* Output tag, length and data, restore previous position
      if the write fails */

   length = PAD_EVEN(length);

   if( write(m_avi->fdes,c,8) != 8 ||
       write(m_avi->fdes,data,length) != length )
   {
      lseek(m_avi->fdes,m_avi->pos,SEEK_SET);
      AVI_errno = AVI_ERR_WRITE;
      return false;
   }

   /* Update file position */

   m_avi->pos += 8 + length;

   return true;
}

bool CAviFmtInterface::avi_add_index_entry(char *tag, long flags, long pos, long len)
{
   void *ptr;

   if(m_avi->n_idx>=m_avi->max_idx)
   {
      ptr = realloc((void *)m_avi->idx,(m_avi->max_idx+4096)*16);
      if(ptr == 0)
      {
         AVI_errno = AVI_ERR_NO_MEM;
         return false;
      }
      m_avi->max_idx += 4096;
      m_avi->idx = (unsigned char((*)[16]) ) ptr;
   }

   /* Add index entry */

   memcpy(m_avi->idx[m_avi->n_idx],tag,4);
   long2str(m_avi->idx[m_avi->n_idx]+ 4,flags);
   long2str(m_avi->idx[m_avi->n_idx]+ 8,pos);
   long2str(m_avi->idx[m_avi->n_idx]+12,len);

   /* Update counter */

   m_avi->n_idx++;

   return true;
}

bool CAviFmtInterface::AVI_open_output_file(char * filename)
{
   int i;
   unsigned char AVI_header[HEADERBYTES];

   /* Allocate the avi_t struct and zero it */

   m_avi = (avi_t *) malloc(sizeof(avi_t));
   if(m_avi==0)
   {
      AVI_errno = AVI_ERR_NO_MEM;
      return false;
   }
   memset((void *)m_avi,0,sizeof(avi_t));

   /* Since Linux needs a long time when deleting big files,
      we do not truncate the file when we open it.
      Instead it is truncated when the AVI file is closed */

#ifdef WIN32
   m_avi->fdes = (long)fopen(filename,"wb");
#else
   m_avi->fdes = open(filename,O_RDWR|O_CREAT,0600);
#endif
   if (m_avi->fdes < 0)
   {
      AVI_errno = AVI_ERR_OPEN;
      free(m_avi);
      return false;
   }

   /* Write out HEADERBYTES bytes, the header will go here
      when we are finished with writing */

   for (i=0;i<HEADERBYTES;i++) AVI_header[i] = 0;
   i = write(m_avi->fdes,AVI_header,HEADERBYTES);
   if (i != HEADERBYTES)
   {
      close(m_avi->fdes);
      AVI_errno = AVI_ERR_WRITE;
      free(m_avi);
      return false;
   }

   m_avi->pos  = HEADERBYTES;
   m_avi->mode = AVI_MODE_WRITE; /* open for writing */

   return true;
}

void CAviFmtInterface::AVI_set_video(int width, int height, double fps, char *compressor)
{
   /* may only be called if file is open for writing */

   if(m_avi->mode==AVI_MODE_READ) return;

   m_avi->width  = width;
   m_avi->height = height;
   m_avi->fps    = fps;
   memcpy(m_avi->compressor,compressor,4);
   m_avi->compressor[4] = 0;

}

void CAviFmtInterface::AVI_set_audio(int channels, long rate, int bits, int format)
{
   /* may only be called if file is open for writing */

   if(m_avi->mode==AVI_MODE_READ) return;

   m_avi->a_chans = channels;
   m_avi->a_rate  = rate;
   m_avi->a_bits  = bits;
   m_avi->a_fmt   = format;
}

#define OUT4CC(s) \
   if(nhb<=HEADERBYTES-4) memcpy(AVI_header+nhb,s,4); nhb += 4

#define OUTLONG(n) \
   if(nhb<=HEADERBYTES-4) long2str(AVI_header+nhb,n); nhb += 4

#define OUTSHRT(n) \
   if(nhb<=HEADERBYTES-2) { \
      AVI_header[nhb  ] = (n   )&0xff; \
      AVI_header[nhb+1] = (n>>8)&0xff; \
   } \
   nhb += 2

/*
  Write the header of an AVI file and close it.
  returns 0 on success, -1 on write error.
*/

bool CAviFmtInterface::avi_close_output_file()
{

   int ret, njunk, sampsize, hasIndex, ms_per_frame, idxerror, flag;
   int movi_len, hdrl_start, strl_start;
   unsigned char AVI_header[HEADERBYTES];
   long nhb;

   /* Calculate length of movi list */

   movi_len = m_avi->pos - HEADERBYTES + 4;

   /* Try to ouput the index entries. This may fail e.g. if no space
      is left on device. We will report this as an error, but we still
      try to write the header correctly (so that the file still may be
      readable in the most cases */

   idxerror = 0;
   ret = avi_add_chunk("idx1",(unsigned char*)m_avi->idx,m_avi->n_idx*16)?0:1; //modify by cwl,2014.5.29
   hasIndex = (ret==0);
   if(ret)
   {
      idxerror = 1;
      AVI_errno = AVI_ERR_WRITE_INDEX;
   }

   /* Calculate Microseconds per frame */

   if(m_avi->fps < 0.001)
      ms_per_frame = 0;
   else
      ms_per_frame = 1000000./m_avi->fps + 0.5;

   /* Prepare the file header */

   nhb = 0;

   /* The RIFF header */

   OUT4CC ("RIFF");
   OUTLONG(m_avi->pos - 8);    /* # of bytes to follow */
   OUT4CC ("AVI ");

   /* Start the header list */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   hdrl_start = nhb;  /* Store start position */
   OUT4CC ("hdrl");

   /* The main AVI header */

   /* The Flags in AVI File header */

#define AVIF_HASINDEX           0x00000010      /* Index at end of file */
#define AVIF_MUSTUSEINDEX       0x00000020
#define AVIF_ISINTERLEAVED      0x00000100
#define AVIF_TRUSTCKTYPE        0x00000800      /* Use CKType to find key frames */
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVIF_COPYRIGHTED        0x00020000

   OUT4CC ("avih");
   OUTLONG(56);                 /* # of bytes to follow */
   OUTLONG(ms_per_frame);       /* Microseconds per frame */
   OUTLONG(10000000);           /* MaxBytesPerSec, I hope this will never be used */
   OUTLONG(0);                  /* PaddingGranularity (whatever that might be) */
                                /* Other sources call it 'reserved' */
   flag = AVIF_WASCAPTUREFILE;
   if(hasIndex) flag |= AVIF_HASINDEX;
   if(hasIndex && m_avi->must_use_index) flag |= AVIF_MUSTUSEINDEX;
   OUTLONG(flag);               /* Flags */
   OUTLONG(m_avi->video_frames);  /* TotalFrames */
   OUTLONG(0);                  /* InitialFrames */
   if (m_avi->audio_bytes)
      { OUTLONG(2); }           /* Streams */
   else
      { OUTLONG(1); }           /* Streams */
   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(m_avi->width);         /* Width */
   OUTLONG(m_avi->height);        /* Height */
                                /* MS calls the following 'reserved': */
   OUTLONG(0);                  /* TimeScale:  Unit used to measure time */
   OUTLONG(0);                  /* DataRate:   Data rate of playback     */
   OUTLONG(0);                  /* StartTime:  Starting time of AVI data */
   OUTLONG(0);                  /* DataLength: Size of AVI data chunk    */


   /* Start the video stream list ---------------------------------- */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   strl_start = nhb;  /* Store start position */
   OUT4CC ("strl");

   /* The video stream header */

   OUT4CC ("strh");
   OUTLONG(64);                 /* # of bytes to follow */
   OUT4CC ("vids");             /* Type */
   OUT4CC (m_avi->compressor);    /* Handler */
   OUTLONG(0);                  /* Flags */
   OUTLONG(0);                  /* Reserved, MS says: wPriority, wLanguage */
   OUTLONG(0);                  /* InitialFrames */
   OUTLONG(ms_per_frame);       /* Scale */
   OUTLONG(1000000);            /* Rate: Rate/Scale == samples/second */
   OUTLONG(0);                  /* Start */
   OUTLONG(m_avi->video_frames);  /* Length */
   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(-1);                 /* Quality */
   OUTLONG(0);                  /* SampleSize */
   OUTLONG(0);                  /* Frame */
   OUTLONG(0);                  /* Frame */
   OUTLONG(0);                  /* Frame */
   OUTLONG(0);                  /* Frame */

   /* The video stream format */

   OUT4CC ("strf");
   OUTLONG(40);                 /* # of bytes to follow */
   OUTLONG(40);                 /* Size */
   OUTLONG(m_avi->width);         /* Width */
   OUTLONG(m_avi->height);        /* Height */
   OUTSHRT(1); OUTSHRT(24);     /* Planes, Count */
   OUT4CC (m_avi->compressor);    /* Compression */
   OUTLONG(m_avi->width*m_avi->height);  /* SizeImage (in bytes?) */
   OUTLONG(0);                  /* XPelsPerMeter */
   OUTLONG(0);                  /* YPelsPerMeter */
   OUTLONG(0);                  /* ClrUsed: Number of colors used */
   OUTLONG(0);                  /* ClrImportant: Number of colors important */

   /* Finish stream list, i.e. put number of bytes in the list to proper pos */

   long2str(AVI_header+strl_start-4,nhb-strl_start);

   if (m_avi->a_chans && m_avi->audio_bytes)
   {

   sampsize = avi_sampsize();

   /* Start the audio stream list ---------------------------------- */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   strl_start = nhb;  /* Store start position */
   OUT4CC ("strl");

   /* The audio stream header */

   OUT4CC ("strh");
   OUTLONG(64);            /* # of bytes to follow */
   OUT4CC ("auds");
   OUT4CC ("\0\0\0\0");
   OUTLONG(0);             /* Flags */
   OUTLONG(0);             /* Reserved, MS says: wPriority, wLanguage */
   OUTLONG(0);             /* InitialFrames */
   OUTLONG(sampsize);      /* Scale */
   OUTLONG(sampsize*m_avi->a_rate); /* Rate: Rate/Scale == samples/second */
   OUTLONG(0);             /* Start */
   OUTLONG(m_avi->audio_bytes/sampsize);   /* Length */
   OUTLONG(0);             /* SuggestedBufferSize */
   OUTLONG(-1);            /* Quality */
   OUTLONG(sampsize);      /* SampleSize */
   OUTLONG(0);             /* Frame */
   OUTLONG(0);             /* Frame */
   OUTLONG(0);             /* Frame */
   OUTLONG(0);             /* Frame */

   /* The audio stream format */

   OUT4CC ("strf");
   OUTLONG(16);                   /* # of bytes to follow */
   OUTSHRT(m_avi->a_fmt);           /* Format */
   OUTSHRT(m_avi->a_chans);         /* Number of channels */
   OUTLONG(m_avi->a_rate);          /* SamplesPerSec */
   OUTLONG(sampsize*m_avi->a_rate); /* AvgBytesPerSec */
   OUTSHRT(sampsize);             /* BlockAlign */
   OUTSHRT(m_avi->a_bits);          /* BitsPerSample */

   /* Finish stream list, i.e. put number of bytes in the list to proper pos */

   long2str(AVI_header+strl_start-4,nhb-strl_start);

   }

   /* Finish header list */

   long2str(AVI_header+hdrl_start-4,nhb-hdrl_start);

   /* Calculate the needed amount of junk bytes, output junk */

   njunk = HEADERBYTES - nhb - 8 - 12;

   /* Safety first: if njunk <= 0, somebody has played with
      HEADERBYTES without knowing what (s)he did.
      This is a fatal error */

   if(njunk<=0)
   {
      fprintf(stderr,"AVI_close_output_file: # of header bytes too small\n");
      exit(1);
   }

   OUT4CC ("JUNK");
   OUTLONG(njunk);
   memset(AVI_header+nhb,0,njunk);
   nhb += njunk;

   /* Start the movi list */

   OUT4CC ("LIST");
   OUTLONG(movi_len); /* Length of list in bytes */
   OUT4CC ("movi");

   /* Output the header, truncate the file to the number of bytes
      actually written, report an error if someting goes wrong */

   if ( lseek(m_avi->fdes,0,SEEK_SET)<0 )
   {
	   AVI_errno = AVI_ERR_CLOSE;
		return false;
    }
   if(write(m_avi->fdes,AVI_header,HEADERBYTES)!=HEADERBYTES)
   {
		AVI_errno = AVI_ERR_CLOSE;
		return false;
	}

#ifdef _WIN32
    long fileLen = 0;
   if ( lseek(m_avi->fdes,0,SEEK_END)>=0 )
   {
	  fileLen = ftell((FILE*)m_avi->fdes);
    }
#endif

#if 0
#ifndef _WIN32
    if(ftruncate(AVI->fdes,AVI->pos)<0)
#else
	if(_chsize(AVI->fdes,AVI->pos)<0 )
#endif
	{
		AVI_errno = AVI_ERR_CLOSE;
		return -1;
	}
#endif
   if(idxerror) return false;

   return true;
}
/*
   AVI_write_data:
   Add video or audio data to the file;

   Return values:
    0    No error;
   -1    Error, AVI_errno is set appropriatly;

*/

bool CAviFmtInterface::avi_write_data(const char *data, long length, int audio ,bool bIsKeyFrame /*= false*/)
{
   bool bRet;
   long keyFlag = bIsKeyFrame?0x00000010:0x00000000;

   /* Check for maximum file length */

   if ( (m_avi->pos + 8 + length + 8 + (m_avi->n_idx+1)*16) > AVI_MAX_LEN )
   {
      AVI_errno = AVI_ERR_SIZELIM;
      return false;
   }

   /* Add index entry */

   if(audio)
      bRet = avi_add_index_entry("01wb",keyFlag,m_avi->pos,length);
   else
      bRet = avi_add_index_entry("00db",keyFlag,m_avi->pos,length);

   if(!bRet) return false;

   /* Output tag and data */

   if(audio)
      bRet = avi_add_chunk("01wb",(const unsigned char*)data,length);
   else
      bRet = avi_add_chunk("00db",(const unsigned char*)data,length);

   if (!bRet) return false;

   return true;
}


bool CAviFmtInterface::AVI_write_frame(const char *data, long bytes,bool bIsKeyFrame)
{
   long pos;

   if(m_avi->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return false; }
   if(data==NULL || bytes<=4) { AVI_errno = AVI_ERR_INVALID_PARAM; return false; }

   //char *pAviFormatFrameBuf = NULL; 
   if(memcmp(data,"\00\00\00\01",4)!=0)
   {
	    int alignLen = PAD_EVEN(bytes); //分配对齐后所需的空间，并初始化为零
		if(alignLen>m_MaxAviFormatFrameLen)
		{
			if(m_pAviFormatFrameBuf==NULL)
			{
				m_pAviFormatFrameBuf = (char*)malloc(alignLen);
			}
			else
			{
				m_pAviFormatFrameBuf = (char*)realloc(m_pAviFormatFrameBuf,alignLen);
			}
			if(m_pAviFormatFrameBuf==NULL) { AVI_errno = AVI_ERR_NO_MEM; return false; }
			m_MaxAviFormatFrameLen = alignLen;
		}
		memset(m_pAviFormatFrameBuf,0,alignLen);
		//pAviFormatFrameBuf = (char*)malloc(alignLen);
		//if(pAviFormatFrameBuf==NULL) { AVI_errno = AVI_ERR_NO_MEM; return false; }
		//memset(pAviFormatFrameBuf,0,alignLen);

		memcpy(m_pAviFormatFrameBuf,"\00\00\00\01",4);
		memcpy(m_pAviFormatFrameBuf+4,data+4,bytes-4);

		pos = m_avi->pos;
	    if( !avi_write_data(m_pAviFormatFrameBuf,bytes,0,bIsKeyFrame) ) return false;
	    m_avi->last_pos = pos;
	    m_avi->last_len = bytes;
	    m_avi->video_frames++;

		/*if(pAviFormatFrameBuf!=NULL) 
		{
			free(pAviFormatFrameBuf);
			pAviFormatFrameBuf=NULL;
		}*/
   }
   else
   {
	   pos = m_avi->pos;
	   if( !avi_write_data(data,bytes,0,bIsKeyFrame) ) return false;
	   m_avi->last_pos = pos;
	   m_avi->last_len = bytes;
	   m_avi->video_frames++;
   }

   return true;
}

bool CAviFmtInterface::AVI_dup_frame()
{
   if(m_avi->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return false; }

   if(m_avi->last_pos==0) return 0; /* No previous real frame */
   if(!avi_add_index_entry("00db",0x10,m_avi->last_pos,m_avi->last_len)) return false;
   m_avi->video_frames++;
   m_avi->must_use_index = 1;
   return true;
}

bool CAviFmtInterface::AVI_write_audio(const char *data, long bytes,bool bIsKeyFrame /*=false*/)
{
   if(m_avi->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return false; }

   if( !avi_write_data(data,bytes,1,bIsKeyFrame) ) return false;
   m_avi->audio_bytes += bytes;
   return true;
}

long CAviFmtInterface::AVI_bytes_remain()
{
   if(m_avi->mode==AVI_MODE_READ) return 0;

   return ( AVI_MAX_LEN - (m_avi->pos + 8 + 16*m_avi->n_idx));
}

bool CAviFmtInterface::AVI_close()
{
   bool ret = false;

   /* If the file was open for writing, the header and index still have
      to be written */

   if(m_avi->mode == AVI_MODE_WRITE)
      ret = avi_close_output_file();
   else
      ret = true;

   /* Even if there happened a error, we first clean up */

   close(m_avi->fdes);
   if(m_avi->idx) free(m_avi->idx);
   if(m_avi->video_index) free(m_avi->video_index);
   if(m_avi->audio_index) free(m_avi->audio_index);
   free(m_avi);

   return ret;
}

#define ERR_EXIT(x) \
{ \
   AVI_close(); \
   AVI_errno = x; \
   return false; \
}

bool CAviFmtInterface::AVI_open_input_file(const char *filename)
{
   long i, n, rate, scale, idx_type;
   unsigned char *hdrl_data;
   long hdrl_len = 0;
   long nvi, nai, ioff;
   long tot;
   int lasttag = 0;
   int vids_strh_seen = 0;
   int vids_strf_seen = 0;
   int auds_strh_seen = 0;
   int auds_strf_seen = 0;
   int num_stream = 0;
   char data[256];
   int getIndex = 1;

   /* Create avi_t structure */

   m_avi = (avi_t *) malloc(sizeof(avi_t));
   if(m_avi==NULL)
   {
      AVI_errno = AVI_ERR_NO_MEM;
      return false;
   }
   memset((void *)m_avi,0,sizeof(avi_t));

   m_avi->mode = AVI_MODE_READ; /* open for reading */

   /* Open the file */

#ifdef WIN32
   m_avi->fdes = (long)fopen(filename,"rb");
#else
   m_avi->fdes = open(filename,O_RDONLY);
#endif
   if(m_avi->fdes < 0)
   {
      AVI_errno = AVI_ERR_OPEN;
      free(m_avi);
      return false;
   }

   /* Read first 12 bytes and check that this is an AVI file */

   if( read(m_avi->fdes,data,12) != 12 ) ERR_EXIT(AVI_ERR_READ)

   if( strncasecmp(data  ,"RIFF",4) !=0 ||
       strncasecmp(data+8,"AVI ",4) !=0 ) ERR_EXIT(AVI_ERR_NO_AVI)

   /* Go through the AVI file and extract the header list,
      the start position of the 'movi' list and an optionally
      present idx1 tag */

   hdrl_data = 0;

   while(1)
   {
      if( read(m_avi->fdes,data,8) != 8 ) break; /* We assume it's EOF */

      n = str2ulong((unsigned char*)(data+4));
      n = PAD_EVEN(n);

      if(strncasecmp(data,"LIST",4) == 0)
      {
         if( read(m_avi->fdes,data,4) != 4 ) ERR_EXIT(AVI_ERR_READ)
         n -= 4;
         if(strncasecmp(data,"hdrl",4) == 0)
         {
            hdrl_len = n;
            hdrl_data = (unsigned char *) malloc(n);
            if(hdrl_data==0) ERR_EXIT(AVI_ERR_NO_MEM)
            if( read(m_avi->fdes,hdrl_data,n) != n ) ERR_EXIT(AVI_ERR_READ)
         }
         else if(strncasecmp(data,"movi",4) == 0)
         {
#ifdef WIN32
			m_avi->movi_start = ftell((FILE*)m_avi->fdes);
#else
            m_avi->movi_start = lseek(m_avi->fdes,0,SEEK_CUR);
#endif
            lseek(m_avi->fdes,n,SEEK_CUR);
         }
         else
            lseek(m_avi->fdes,n,SEEK_CUR);
      }
      else if(strncasecmp(data,"idx1",4) == 0)
      {
         /* n must be a multiple of 16, but the reading does not
            break if this is not the case */

         m_avi->n_idx = m_avi->max_idx = n/16;
         m_avi->idx = (unsigned  char((*)[16]) ) malloc(n);
         if(m_avi->idx==0) ERR_EXIT(AVI_ERR_NO_MEM)
         if( read(m_avi->fdes,m_avi->idx,n) != n ) ERR_EXIT(AVI_ERR_READ)
      }
      else
         lseek(m_avi->fdes,n,SEEK_CUR);
   }

   if(!hdrl_data      ) ERR_EXIT(AVI_ERR_NO_HDRL)
   if(!m_avi->movi_start) ERR_EXIT(AVI_ERR_NO_MOVI)

   /* Interpret the header list */

   for(i=0;i<hdrl_len;)
   {
      /* List tags are completly ignored */

      if(strncasecmp((char*)(hdrl_data+i),"LIST",4)==0) { i+= 12; continue; }

      n = str2ulong(hdrl_data+i+4);
      n = PAD_EVEN(n);

      /* Interpret the tag and its args */

      if(strncasecmp((char*)(hdrl_data+i),"strh",4)==0)
      {
         i += 8;
         if(strncasecmp((char*)(hdrl_data+i),"vids",4) == 0 && !vids_strh_seen)
         {
            memcpy(m_avi->compressor,hdrl_data+i+4,4);
            m_avi->compressor[4] = 0;
            scale = str2ulong(hdrl_data+i+20);
            rate  = str2ulong(hdrl_data+i+24);
            if(scale!=0) m_avi->fps = (double)rate/(double)scale;
            m_avi->video_frames = str2ulong(hdrl_data+i+32);
            m_avi->video_strn = num_stream;
            vids_strh_seen = 1;
            lasttag = 1; /* vids */
         }
         else if (strncasecmp ((char*)(hdrl_data+i),"auds",4) ==0 && ! auds_strh_seen)
         {
            m_avi->audio_bytes = str2ulong(hdrl_data+i+32)*avi_sampsize();
            m_avi->audio_strn = num_stream;
            auds_strh_seen = 1;
            lasttag = 2; /* auds */
         }
         else
            lasttag = 0;
         num_stream++;
      }
      else if(strncasecmp((char*)(hdrl_data+i),"strf",4)==0)
      {
         i += 8;
         if(lasttag == 1)
         {
            m_avi->width  = str2ulong(hdrl_data+i+4);
            m_avi->height = str2ulong(hdrl_data+i+8);
            vids_strf_seen = 1;
         }
         else if(lasttag == 2)
         {
            m_avi->a_fmt   = str2ushort(hdrl_data+i  );
            m_avi->a_chans = str2ushort(hdrl_data+i+2);
            m_avi->a_rate  = str2ulong (hdrl_data+i+4);
            m_avi->a_bits  = str2ushort(hdrl_data+i+14);
            auds_strf_seen = 1;
         }
         lasttag = 0;
      }
      else
      {
         i += 8;
         lasttag = 0;
      }

      i += n;
   }

   free(hdrl_data);

   if(!vids_strh_seen || !vids_strf_seen || m_avi->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS)

   m_avi->video_tag[0] = m_avi->video_strn/10 + '0';
   m_avi->video_tag[1] = m_avi->video_strn%10 + '0';
   m_avi->video_tag[2] = 'd';
   m_avi->video_tag[3] = 'b';

   /* Audio tag is set to "99wb" if no audio present */
   if(!m_avi->a_chans) m_avi->audio_strn = 99;

   m_avi->audio_tag[0] = m_avi->audio_strn/10 + '0';
   m_avi->audio_tag[1] = m_avi->audio_strn%10 + '0';
   m_avi->audio_tag[2] = 'w';
   m_avi->audio_tag[3] = 'b';

   lseek(m_avi->fdes,m_avi->movi_start,SEEK_SET);

   /* get index if wanted */

   if(!getIndex) return true;

   /* if the file has an idx1, check if this is relative
      to the start of the file or to the start of the movi list */

   idx_type = 0;

   if(m_avi->idx)
   {
      long pos;
		unsigned long len;

      /* Search the first videoframe in the idx1 and look where
         it is in the file */

      for(i=0;i<m_avi->n_idx;i++)
         if( strncasecmp((char*)(m_avi->idx[i]),m_avi->video_tag,3)==0 ) break;
      if(i>=m_avi->n_idx) ERR_EXIT(AVI_ERR_NO_VIDS)

      pos = str2ulong(m_avi->idx[i]+ 8);
      len = str2ulong(m_avi->idx[i]+12);

      lseek(m_avi->fdes,pos,SEEK_SET);
      if(read(m_avi->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ)
      if( strncasecmp(data,(char*)(m_avi->idx[i]),4)==0 && str2ulong((unsigned char*)(data+4))==len )
      {
         idx_type = 1; /* Index from start of file */
      }
      else
      {
         lseek(m_avi->fdes,pos+m_avi->movi_start-4,SEEK_SET);
         if(read(m_avi->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ)
         if( strncasecmp(data,(char*)(m_avi->idx[i]),4)==0 && str2ulong((unsigned char*)(data+4))==len )
         {
            idx_type = 2; /* Index from start of movi list */
         }
      }
      /* idx_type remains 0 if neither of the two tests above succeeds */
   }

   if(idx_type == 0)
   {
      /* we must search through the file to get the index */

      lseek(m_avi->fdes, m_avi->movi_start, SEEK_SET);

      m_avi->n_idx = 0;

      while(1)
      {
         if( read(m_avi->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char*)(data+4));

         /* The movi list may contain sub-lists, ignore them */

         if(strncasecmp(data,"LIST",4)==0)
         {
            lseek(m_avi->fdes,4,SEEK_CUR);
            continue;
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         if( ( (data[2]=='d' || data[2]=='D') &&
               (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') )
          || ( (data[2]=='w' || data[2]=='W') &&
               (data[3]=='b' || data[3]=='B') ) )
         {
#ifdef WIN32
			 lseek(m_avi->fdes,0,SEEK_CUR);
			 long pos = ftell((FILE*)(m_avi->fdes));
			 avi_add_index_entry(data,0,pos-8,n);
#else
			 avi_add_index_entry(data,0,lseek(m_avi->fdes,0,SEEK_CUR)-8,n);
#endif
         }

         lseek(m_avi->fdes,PAD_EVEN(n),SEEK_CUR);
      }
      idx_type = 1;
   }

   /* Now generate the video index and audio index arrays */

   nvi = 0;
   nai = 0;

   for(i=0;i<m_avi->n_idx;i++)
   {
      if(strncasecmp((char*)(m_avi->idx[i]),m_avi->video_tag,3) == 0) nvi++;
      if(strncasecmp((char*)(m_avi->idx[i]),m_avi->audio_tag,4) == 0) nai++;
   }

   m_avi->video_frames = nvi;
   m_avi->audio_chunks = nai;

   if(m_avi->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS)
   m_avi->video_index = (video_index_entry *) malloc(nvi*sizeof(video_index_entry));
   if(m_avi->video_index==0) ERR_EXIT(AVI_ERR_NO_MEM)
   if(m_avi->audio_chunks)
   {
      m_avi->audio_index = (audio_index_entry *) malloc(nai*sizeof(audio_index_entry));
      if(m_avi->audio_index==0) ERR_EXIT(AVI_ERR_NO_MEM)
   }

   nvi = 0;
   nai = 0;
   tot = 0;
   ioff = idx_type == 1 ? 8 : m_avi->movi_start+4;

   for(i=0;i<m_avi->n_idx;i++)
   {
      if(strncasecmp((char*)(m_avi->idx[i]),m_avi->video_tag,3) == 0)
      {
		 m_avi->video_index[nvi].flag = str2ulong(m_avi->idx[i]+4);
         m_avi->video_index[nvi].pos = str2ulong(m_avi->idx[i]+ 8)+ioff;
         m_avi->video_index[nvi].len = str2ulong(m_avi->idx[i]+12);
         nvi++;
      }
      if(strncasecmp((char*)(m_avi->idx[i]),m_avi->audio_tag,4) == 0)
      {
	     m_avi->audio_index[nai].flag = str2ulong(m_avi->idx[i]+4);
         m_avi->audio_index[nai].pos = str2ulong(m_avi->idx[i]+ 8)+ioff;
         m_avi->audio_index[nai].len = str2ulong(m_avi->idx[i]+12);
         m_avi->audio_index[nai].tot = tot;
         tot += m_avi->audio_index[nai].len;
         nai++;
      }
   }

   m_avi->audio_bytes = tot;

   /* Reposition the file */

   lseek(m_avi->fdes,m_avi->movi_start,SEEK_SET);
   m_avi->video_pos = 0;

   return true;
}

long CAviFmtInterface::AVI_video_frames()
{
   return m_avi->video_frames;
}
int  CAviFmtInterface::AVI_video_width()
{
   return m_avi->width;
}
int  CAviFmtInterface::AVI_video_height()
{
   return m_avi->height;
}
double CAviFmtInterface::AVI_video_frame_rate()
{
   return m_avi->fps;
}
char* CAviFmtInterface::AVI_video_compressor()
{
   return m_avi->compressor;
}

int CAviFmtInterface::AVI_audio_channels()
{
   return m_avi->a_chans;
}
int CAviFmtInterface::AVI_audio_bits()
{
   return m_avi->a_bits;
}
int CAviFmtInterface::AVI_audio_format()
{
   return m_avi->a_fmt;
}
long CAviFmtInterface::AVI_audio_rate()
{
   return m_avi->a_rate;
}
long CAviFmtInterface::AVI_audio_bytes()
{
   return m_avi->audio_bytes;
}


long CAviFmtInterface::AVI_frame_size(long frame)
{
   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!m_avi->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= m_avi->video_frames) return 0;
   return(m_avi->video_index[frame].len);
}

bool CAviFmtInterface::AVI_seek_start()
{
   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return false; }

   lseek(m_avi->fdes,m_avi->movi_start,SEEK_SET);
   m_avi->video_pos = 0;
   m_avi->audio_posc = 0;
   m_avi->audio_posb = 0;
   return true;
}

bool CAviFmtInterface::AVI_set_video_position(long frame, long *frame_len)
{
   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return false; }
   if(!m_avi->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return false; }

   if (frame < 0 ) frame = 0;
   m_avi->video_pos = frame;
   if (frame_len != NULL)
     *frame_len = m_avi->video_index[frame].len;
   return true;
}

long CAviFmtInterface::AVI_read_frame(char *vidbuf,unsigned long bufLength,bool &bIsKeyFrame)
{
   long n;

   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!m_avi->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(m_avi->video_pos < 0 || m_avi->video_pos >= m_avi->video_frames) return 0;
   n = m_avi->video_index[m_avi->video_pos].len;
   bIsKeyFrame = ((m_avi->video_index[m_avi->video_pos].flag&AVIIF_KEYFRAME)!=0x0)?true:false;
   if ( bufLength < n)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }

   long offset = lseek(m_avi->fdes, m_avi->video_index[m_avi->video_pos].pos, SEEK_SET);
   long readNum = read(m_avi->fdes,vidbuf,n);
   if ( readNum != n)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }

   m_avi->video_pos++;

   return n;
}

bool CAviFmtInterface::AVI_set_audio_position(long byte)
{
   long n0, n1, n;

   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return false; }
   if(!m_avi->audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return false; }

   if(byte < 0) byte = 0;

   /* Binary search in the audio chunks */

   n0 = 0;
   n1 = m_avi->audio_chunks;

   while(n0<n1-1)
   {
      n = (n0+n1)/2;
      if(m_avi->audio_index[n].tot>byte)
         n1 = n;
      else
         n0 = n;
   }

   m_avi->audio_posc = n0;
   m_avi->audio_posb = byte - m_avi->audio_index[n0].tot;

   return true;
}

bool CAviFmtInterface::AVI_set_audio_frame (long frame, long *frame_len)
{
  if (m_avi->audio_posc >= m_avi->audio_chunks - 1) { return false; }
  m_avi->audio_posc = frame;
  m_avi->audio_posb = 0;
  if (frame_len != NULL)
    *frame_len = m_avi->audio_index[frame].len;
  return true;
}

long CAviFmtInterface::AVI_read_audio(char *audbuf, long bytes)
{
   long nr, pos, left, todo;

   if(m_avi->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!m_avi->audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   nr = 0; /* total number of bytes read */

   while(bytes>0)
   {
      left = m_avi->audio_index[m_avi->audio_posc].len - m_avi->audio_posb;
      if(left==0)
      {
         if(m_avi->audio_posc>=m_avi->audio_chunks-1) return nr;
         m_avi->audio_posc++;
         m_avi->audio_posb = 0;
         continue;
      }
      if(bytes<left)
         todo = bytes;
      else
         todo = left;
      pos = m_avi->audio_index[m_avi->audio_posc].pos + m_avi->audio_posb;
      lseek(m_avi->fdes, pos, SEEK_SET);
      if (read(m_avi->fdes,audbuf+nr,todo) != todo)
      {
         AVI_errno = AVI_ERR_READ;
         return -1;
      }
      bytes -= todo;
      nr    += todo;
      m_avi->audio_posb += todo;
   }

   return nr;
}

/* AVI_read_data: Special routine for reading the next audio or video chunk
                  without having an index of the file. */

int CAviFmtInterface::AVI_read_data(char *vidbuf, long max_vidbuf,
                              char *audbuf, long max_audbuf,
                              long *len,bool &bIsKeyFrame)
{

/*
 * Return codes:
 *
 *    1 = video data read
 *    2 = audio data read
 *    0 = reached EOF
 *   -1 = video buffer too small
 *   -2 = audio buffer too small
 */

   int n;
   char data[8];
   bIsKeyFrame = false;

   if(m_avi->mode==AVI_MODE_WRITE) return 0;

   while(1)
   {
      /* Read tag and length */

      if( read(m_avi->fdes,data,8) != 8 ) return 0;

      /* if we got a list tag, ignore it */

      if(strncasecmp(data,"LIST",4) == 0)
      {
         lseek(m_avi->fdes,4,SEEK_CUR);
         continue;
      }

      n = PAD_EVEN(str2ulong((unsigned char*)(data+4)));

      if(strncasecmp(data,m_avi->video_tag,3) == 0)
      {
		 //Modify by cwl,2014.5.27
	     //改为获取真实的长度，而不是对齐后的长度
         //*len = n;
		 *len = str2ulong((unsigned char*)(data+4));
		 bIsKeyFrame = ((m_avi->video_index[m_avi->video_pos].flag&AVIIF_KEYFRAME)!=0x0)?true:false;
         m_avi->video_pos++;
         if(n>max_vidbuf)
         {
            lseek(m_avi->fdes,n,SEEK_CUR);
            return -1;
         }
         if(read(m_avi->fdes,vidbuf,n) != n ) return 0;
         return 1;
      }
      else if(strncasecmp(data,m_avi->audio_tag,4) == 0)
      {
         //Modify by cwl,2014.5.27
	     //改为获取真实的长度，而不是对齐后的长度
         //*len = n;
		 *len = str2ulong((unsigned char*)(data+4));

         if(n>max_audbuf)
         {
            lseek(m_avi->fdes,n,SEEK_CUR);
            return -2;
         }
         if(read(m_avi->fdes,audbuf,n) != n ) return 0;
         return 2;
         break;
      }
      else
         if(lseek(m_avi->fdes,n,SEEK_CUR)<0)  return 0;
   }
}


/************************************************* 
Function:    FindStartCode3  
Description: 判断是否为0x00000001,如果是返回1	  
Return: 
Others:
Author: licaibiao
Date:   2017-12-06
*************************************************/
int CAviFmtInterface::FindStartCode3 (char *Buf)  
{  
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) 
	{
		return 0;
	}
	else
	{
		return 1;  
	}   
} 

#define MAX_NALU_LEN ((1)*(512)*(1024))   /**h264 最大一帧数据长度 512K**/

/************************************************* 
Function:    GetAnnexbNALU  
Description: 获取一个NALU数据，规定输入文件的NALU头为:00 00 00 01 
Input:  u32_Num 通道号， nalu 存储nalu数据地址指针    
Return: NALU 长度 包含前缀4个字节
Others: 该函数只识别前缀为00 00 00 01 文件
Author: licaibiao
Date:   2017-12-07
*************************************************/
int CAviFmtInterface::GetAnnexbNALU (FILE *pFd, char* nalu)  
{  
	int l_s32Pos = 0;
	int l_s32Rewind = -4;
	int l_s32NaluLen = 0;
	unsigned char l_u8FindFlag = 0;
	char* pBuf = NULL;
	FILE* p_fd = NULL;
	
	p_fd = pFd;
	if((NULL == nalu)||(NULL == p_fd))
	{
		printf("【[%s:%d] input para error!  \n", __func__, __LINE__);
		return -1;
	}
	pBuf = (char*)malloc (MAX_NALU_LEN);
	if(NULL == pBuf)
	{
		printf("【[%s:%d] allocate pBuf memory error  \n", __func__, __LINE__);
		return -1;
	}
	memset(pBuf,0,MAX_NALU_LEN);
	/**从码流中读取4个字节**/
	if (4 != fread (pBuf, 1, 4, p_fd))
	{  
		free(pBuf);  
		return -1;  
	} 
	
	/**判断是否是00 00 00 01**/
	if(FindStartCode3(pBuf))
	{
		l_u8FindFlag = 1;
		l_s32Pos = 4;
	}
	else
	{
		l_u8FindFlag = 0;
		printf("【[%s:%d] start code error  \n", __func__, __LINE__);
		return -1;
	}
	
	/**查找下一个开始字符标志位**/
	l_u8FindFlag = 0;
	while(!l_u8FindFlag)
	{
		/**判断是否到了文件尾**/
		if(feof(p_fd))
		{
			/**如果读取到了结尾，重头开始读取**/
			//fseek(p_fd,0,SEEK_SET);
#if 1		
			l_s32NaluLen = l_s32Pos - 1;
			memcpy (nalu, pBuf, l_s32NaluLen);
			free(pBuf);
			pBuf = NULL;
			return l_s32NaluLen;
#endif
		}
		/**读取一个字节到pBuf 中**/
		pBuf[l_s32Pos++] = fgetc (p_fd);
		if(FindStartCode3(&pBuf[l_s32Pos-4]))
		{
			l_u8FindFlag = 1;
		}
	}
	
	/**把文件指针指向前一个NALU的末尾 **/
	if (0 != fseek (p_fd, l_s32Rewind, SEEK_CUR))
	{  
		free(pBuf);  
		printf("【[%s:%d] GetAnnexbNALU: Cannot fseek in the bit stream file  \n", __func__, __LINE__);
	} 
	
	/**NALU 长度 包含前缀4个字节**/
	l_s32NaluLen = l_s32Pos + l_s32Rewind;
	memcpy (nalu, pBuf, l_s32NaluLen);
	free(pBuf);
	pBuf = NULL;
	
	return l_s32NaluLen;
}


							  
