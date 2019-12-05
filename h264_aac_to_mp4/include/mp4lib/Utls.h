 
#ifndef __PORTALBE_H__
#define __PORTALBE_H__

#include "stdio.h"
#include <stdlib.h>
#include <string>
#include <assert.h>
#ifdef WIN32
#include <tchar.h>
#endif

typedef unsigned char byte;
typedef unsigned char BYTE;
typedef unsigned char* LPBYTE;

typedef char    int8;
typedef short   int16;
typedef int     int32;

#ifndef WIN32
typedef long long   int64;
#else
typedef long        int64;
#endif

//typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
#ifndef WIN32
typedef unsigned long long   uint64;
#else
typedef unsigned long        uint64;
#endif

//typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;

//  #ifndef WIN32
//  typedef long long       int64_t;
//  #else
//  typedef long            int64_t;
//  #endif

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned int    UINT;

// #ifndef WIN32
// typedef unsigned long long   uint64_t;
// #else
// typedef unsigned long        uint64_t;
// #endif

#ifdef __cplusplus

#else
typedef uint8 bool;
#endif 


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef true
	#define true  1
#endif 

#ifndef false
	#define false 0
#endif 



#ifndef BOOL
typedef int   BOOL;
#endif 

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif 


typedef unsigned long DWORD;
typedef unsigned short WORD;

//////////////////////////////////////////////////////////////////////////
//  #ifndef DWORD
//  #define DWORD unsigned long
//  #endif
//  
//  #ifndef WORD
//  #define WORD unsigned short
//  #endif

#ifndef ASSERT
#ifdef NDEBUG
#define ASSERT(expr)
#else
#define ASSERT(expr) \
	if (!(expr)) { \
	fflush(stdout); \
	assert((expr)); \
	}
#endif
#endif

#endif //


