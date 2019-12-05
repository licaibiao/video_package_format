#ifndef _TYPE_H
#define _TYPE_H
#define MAKEFOURCC(ch0, ch1, ch2, ch3)   ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define H264BUFSIZE 1024*1024
#define AACBUFSIZE 1024*1024
#define IDSIZE 1024*1024*2

typedef  unsigned long DWORD;
typedef  unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned long FOURCC;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

extern long file_h264_length;//H264文件的长度
extern long file_aac_length;//AAC文件的长度

typedef struct
{
	uint8 *id;
	uint8 *id_start;
	uint8 *id_end;
	unsigned int id_shift_length;
} INDEX;

#endif