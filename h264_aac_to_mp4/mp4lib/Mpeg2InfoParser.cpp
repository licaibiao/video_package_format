// Mpeg2InfoParser.cpp: implementation of the CMpeg2InfoParser class.
//
//////////////////////////////////////////////////////////////////////
#include "Mpeg2InfoParser.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void ReverseBits(byte & bt)
{
	byte delta[8] ;

	memset((void * )delta,'\0',8); 

	delta[0] = 0x01 & bt ;
	delta[0] = delta[0]<<7 ;
	delta[1] = 0x02 & bt ;
	delta[1] = delta[1]<<5 ;
	delta[2] = 0x04 & bt ;
	delta[2] = delta[2]<<3 ;
	delta[3] = 0x08 & bt ;
	delta[3] = delta[3]<<1 ;
	delta[4] = 0x10 & bt ;
	delta[4] = delta[4]>>1 ;
	delta[5] = 0x20 & bt ;
	delta[5] = delta[5]>>3;
	delta[6] = 0x40 & bt ;
	delta[6] = delta[6]>>5;
	delta[7] = 0x80 & bt ;
	delta[7] = delta[7]>>7;

	bt = 0x00;
	for(int i = 0;i<8 ; i++)
	{
		bt += delta[i]; 
	}

}


void ReverseBytes(byte * pData, int len , bool bReverseBit = false)
{
	ASSERT(len>0 && pData) ;

	byte tmp;
	int i ;
	for(i = 0 ; i< len / 2; i++)
	{
		tmp = *(pData + (len - 1 - i) );
		*(pData + (len - 1 - i)) = *(pData + i);
		*(pData + i) = tmp;
	}
	if(bReverseBit)
	{
		for(i=0;i<len;i++)
		{
			ReverseBits(*(pData + i));
		}
	}
}


//@DESC: IF == 1, true
bool getBit(const byte & bt , byte  pos)
{
	ASSERT(pos>=0 && pos<=7 );

	byte oneFlag = 0x01; 
	
	oneFlag = oneFlag << pos ;
	return (bt & oneFlag ) != 0x00; 
}


//@desc: set bit value  in a byte
void setBit(byte & bt, byte pos, bool bZero)
{
	ASSERT(pos>=0 && pos<=7 );
	int i = 0;

	byte flag = 0x01; 
	if(bZero)
	{
		flag =  flag<< pos ;
		flag = ~flag ;
		bt = bt & flag ;
	}
	else
	{
		flag = flag << pos ;
		bt = bt | flag ;
	}
}

void ShiftInOneByte(byte & bt, int shiftLen ,bool bLeft, bool bZeroFill)
{
	if(shiftLen <= 0 || shiftLen >=8)
		return;

	int i = 0;

	if(bLeft)
		bt = bt<<shiftLen;
	else
		bt = bt>>shiftLen;

	if(bLeft)
	{
		for(i=0;i<shiftLen;i++)
		{
			setBit(bt,i,bZeroFill);
		}
	}
	else
	{
		for(i=7;i>7-shiftLen;i--)
		{
			setBit(bt,i,bZeroFill);
		}
	}
}



//@param : bLeft for shift from left to right
//@note : pos 0 is the most left element (len is in bytes)
void ShiftBits(byte * pData,int len ,int shiftLen, bool bLeft,byte btFill,LPShift_Delta_Bits pShiftDeltaBits)
{
	int i;
	bool bStoreShiftBits = pShiftDeltaBits!=NULL;
	byte deltaBits;

	//shift length ought to be small than 8 bits (one byte)
	ASSERT(shiftLen<8);

	if(bStoreShiftBits)
	{
		pShiftDeltaBits->len = shiftLen ;
		pShiftDeltaBits->bLeftAligned = bLeft;
	}
	
	if(bLeft)
	{
		for(i=0;i<len;i++)
		{
			if(i==len - 1)
			{//add fill bits,the left bits in btFill
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
				ShiftInOneByte(btFill,8-shiftLen,!bLeft,true);
				*(pData + i) += btFill;
			}else if(i==0)
			{//save left bits in pShiftDeltaBits
				pShiftDeltaBits->data = *(pData + i);
				ShiftInOneByte(pShiftDeltaBits->data, 8 - shiftLen ,!bLeft,true);
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
			}else
			{
				deltaBits = 0x00 ;
				deltaBits = *(pData + i);
				ShiftInOneByte(deltaBits,8-shiftLen,!bLeft,true);
				*(pData + i - 1) += deltaBits;
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
			}
		}
	}else
	{
		for(i=len-1;i>=0;i--)
		{
			if(i==0)
			{//add fill bits,the left bits in btFill
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
				ShiftInOneByte(btFill,8-shiftLen,!bLeft,true);
				*(pData + i) += btFill;
			}else if(i==len-1)
			{//save left bits in pShiftDeltaBits
				pShiftDeltaBits->data = *(pData + i);
				ShiftInOneByte(pShiftDeltaBits->data, 8 - shiftLen ,!bLeft,true);
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
			}else
			{
				deltaBits = 0x00 ;
				deltaBits = *(pData + i);
				ShiftInOneByte(deltaBits,8-shiftLen,!bLeft,true);
				*(pData + i + 1) += deltaBits;
				ShiftInOneByte(*(pData + i),shiftLen,bLeft,true);
			}
		}
	}
}


//##ModelId=3C919E700355
typedef struct _Mpeg2_packet_start_code_prefix 
{	//can be read derectly from file data : 3 bytes
	BYTE btPrefix[3] ;
}stMpeg2_packet_start_code_prefix,*LPMpeg2_packet_start_code_prefix;

//can be read derectly from file data
//##ModelId=3C919E7003AF
typedef struct _Mepg2_Start_Code
{	//can be read derectly from file data : 4 bytes
	stMpeg2_packet_start_code_prefix prefix;
	BYTE btStartCodeID ; 

}stMepg2_Start_Code, * LPMepg2_Start_Code;

#ifndef WIN32
#define __int64 long long
#endif

//##ModelId=3C919E71032E
typedef struct _ES_Sequence_Header
{
	stMepg2_Start_Code sequence_header_code	; 
	struct _Stuff
	{//i'll read 64 bits ,will include one bit of follow !!
		union
		{
			struct
			{
				__int64	tempbit						:	

1;//store for  follow bit

				__int64	load_intra_quantiser_matrix	:	1;
				__int64	constrained_parameters_flag	:	1;
				__int64	vbv_buffer_size_value		:	10;
				__int64	marker_bit					:	1;
				__int64 bit_rate_value				:	18;
				__int64	frame_rate_code				:	4;
				__int64	aspect_ratio_information	:	4;
				__int64	vertical_size_value			:	12;
				__int64	horizontal_size_value		:	12;
			};
			BYTE data[8] ;
		};
	}stuff;

	//if ( load_intra_quantiser_matrix ) 	
	BYTE	intra_quantiser_matrix[64]     	;//8*64
	BYTE	load_non_intra_quantiser_matrix	:	1;
	BYTE	:	0;
	//if ( load_non_intra_quantiser_matrix )	
	BYTE	non_intra_quantiser_matrix[64]	;//8*64
	//next_start_code()	
}stES_Sequence_Header, * LPES_Sequence_Header	;



int Xh_GetVEsSeqHeaderInfo(BYTE* pBufferIn,int nBufferLen,
						   stES_Sequence_Header&  SeqHeader,double& dFrameRate)
{
	int nRet = 1;
	if (pBufferIn == NULL || nBufferLen - 4 < sizeof(stES_Sequence_Header)) 
	{
		return FALSE;
	}
	
	try
	{
		int nBufOfst = 0;
		BYTE* pBuf = pBufferIn;
	//	DWORD length = 0 ;
		BYTE* ptr ;
		int  size = 0;
		
		//read prefix
		// need to test if reach the end of file, same is to below
		size = sizeof(stMepg2_Start_Code) ;
		memcpy((void *)& SeqHeader.sequence_header_code,pBuf + nBufOfst, size);
		nBufOfst += size ;
	//	length += size ;
		
		//read stuff
		size = 8;
		memcpy((void *)SeqHeader.stuff.data  , pBuf + nBufOfst ,size);
		nBufOfst += size ;
	//	length += size ;
		ptr = SeqHeader.stuff.data;
		ReverseBytes(ptr, 8);
		
		if(SeqHeader.stuff.load_intra_quantiser_matrix)
		{//need to load intra_quantiser_matrix add aanother bit for 
			size = 64 ;
			memcpy((void *) SeqHeader.intra_quantiser_matrix  , pBuf + nBufOfst,size);
			nBufOfst += size ;
		//	length += size ;
			
			//shift toward right one bit
			stShift_Delta_Bits bits;
			stShift_Delta_Bits * pShiftDeltaBits = &bits; //new stShift_Delta_Bits ;
			ShiftBits(SeqHeader.intra_quantiser_matrix,64 ,1,false,SeqHeader.stuff.tempbit, pShiftDeltaBits);
			//test bit shifted out
			SeqHeader.stuff.tempbit = (pShiftDeltaBits->data & 0x80) ? 0x01 : 0x00;
			SeqHeader.load_non_intra_quantiser_matrix = SeqHeader.stuff.tempbit;
			if(SeqHeader.load_non_intra_quantiser_matrix)
			{//need to load non_intra_quantiser_matrix
				size = 64 ;
				memcpy((void *) SeqHeader.non_intra_quantiser_matrix  ,pBuf + nBufOfst,  size);
				nBufOfst += size ;
			//	length += size ;
			}
			//delete pShiftDeltaBits ;
		}
		
		DWORD dwFrameRate[2];
		switch(SeqHeader.stuff.frame_rate_code)
		{
		case 1:
			dwFrameRate[0] = 24000;
			dwFrameRate[1] = 1001;
			//		nAverageTimePerFrame = 1001 * 10000 / 24;
			break;
		case 2:
			dwFrameRate[0] = 24;
			dwFrameRate[1] = 1;
			//		nAverageTimePerFrame = 1000 * 10000 / 24;
			break;
		case 3:
			dwFrameRate[0] = 25;
			dwFrameRate[1] = 1;
			//		nAverageTimePerFrame = 1000 * 10000 / 25;
			break;
		case 4:
			dwFrameRate[0] = 30000;
			dwFrameRate[1] = 1001;
			//		nAverageTimePerFrame = 1001 * 10000 / 30;
			break;
		case 5:
			dwFrameRate[0] = 30;
			dwFrameRate[1] = 1;
			//		nAverageTimePerFrame = 1000 * 10000 / 30;
			break;
		case 6:
			dwFrameRate[0] = 50;
			dwFrameRate[1] = 1;
			//		nAverageTimePerFrame = 1000 * 10000 / 50;
			break;
		case 7:
			dwFrameRate[0] = 60000;
			dwFrameRate[1] = 1001;
			//		nAverageTimePerFrame = 1001 * 10000 / 60;
			break;
		case 8	:
			dwFrameRate[0] = 60;
			dwFrameRate[1] = 1;
			//		nAverageTimePerFrame = 1000 * 10000 / 60;
			break;
		};
		
		if (dwFrameRate[1] != 0)
		{
			dFrameRate = 1.0 * dwFrameRate[0] / dwFrameRate[1];
		}

		nRet = 1;
	}
	catch(...)
	{
		nRet = 0;
	}

	return nRet;

}


//##ModelId=3C919E7103E3
typedef struct _ES_Sequence_Extension
{	
	stMepg2_Start_Code	extension_start_code ; 

	//6 bytes , can be read directly
	
	struct _Stuff1
	{//4 bytes
		DWORD	marker_bit						:	1;
		DWORD	bit_rate_extension				:	12;
		DWORD	vertical_size_extension			:	2;
		DWORD	horizontal_size_extension		:	2;
		DWORD	chroma_format					:	2;
		DWORD	progressive_sequence			:	1;
		DWORD	profile_and_level_indication	:	8;
		DWORD	extension_start_code_identifier	:	4;
	}stuff1;
	struct _Stuff2
	{//2 bytes
		WORD	frame_rate_extension_d		:	5;
		WORD	frame_rate_extension_n		:	2;
		WORD	low_delay 					:	1;
		WORD	vbv_buffer_size_extension	:	8;
	}stuff2;

//	next_start_code()	
}stES_Sequence_Extension, * LPES_Sequence_Extension;


int Xh_GetSeqExt(BYTE* pBuffer,int nBufferLen,stES_Sequence_Extension& seqExt)
{
	if (pBuffer == NULL || nBufferLen - 4 < sizeof(_ES_Sequence_Extension)) 
	{
		return 0;
	}
	
	try
	{
		byte * ptr ;
		int size = sizeof(_ES_Sequence_Extension);
		BYTE* pExtPtr = pBuffer;

		memset((void *)&seqExt,'\0',size);
		memcpy((void *) &seqExt, pExtPtr  ,size);
		ptr = (byte *)& seqExt.stuff1 ;
		ReverseBytes(ptr, 4);
		ptr = (byte *)& seqExt.stuff2 ;
		ReverseBytes(ptr, 2);
	}
	catch (...)
	{
		return 0;
	}


	return 1;
}




CMpeg2InfoParser::CMpeg2InfoParser()
{

}

CMpeg2InfoParser::~CMpeg2InfoParser()
{

}


int CMpeg2InfoParser::GetMpeg2Info(BYTE* pBuffer, int nBufSize, FRAME_Info& stFrameInfo)
{
	if (pBuffer == NULL || nBufSize < 4)
	{
		return 0;
	}

	int nRet;

	stES_Sequence_Header stSeqHeader;
	stES_Sequence_Extension stEsSequenceEX;
	
	BYTE *pBuf = pBuffer;
	int  nBufLen = nBufSize;
	int  nOff = 0;
	BOOL bFindType = FALSE;
	while (TRUE)
	{
		if (!FindStartCode(pBuf,nBufLen,nOff))
		{
			break;
		}

		pBuf += nOff;
		nBufLen -= nOff;

		if (pBuf[3] == 0xB3)
		{
			nRet = Xh_GetVEsSeqHeaderInfo(pBuf, nBufLen,stSeqHeader,stFrameInfo.dFramerate);
			if(nRet > 0)
			{
				stFrameInfo.nHeight = stSeqHeader.stuff.vertical_size_value;
				stFrameInfo.nWidth = stSeqHeader.stuff.horizontal_size_value;
				stFrameInfo.nBitRate = stSeqHeader.stuff.bit_rate_value;
				stFrameInfo.bHaveSeqHeader = TRUE;
			}
			else
			{
				stFrameInfo.bHaveSeqHeader = FALSE;
			}
			
		}
		else if(pBuf[3] == 0xB5 && (pBuf[4] & 0xF0) == 0x10)
		{
			
			nRet = Xh_GetSeqExt(pBuf,nBufLen,stEsSequenceEX);
			if (nRet > 0)
			{
				stFrameInfo.bHaveSeqExt = TRUE;
				int  nHighBit = 0;
				nHighBit = stEsSequenceEX.stuff1.bit_rate_extension;
				stFrameInfo.nBitRate = (nHighBit << 12) + stFrameInfo.nBitRate;
			}
			else
				stFrameInfo.bHaveSeqExt = FALSE;
		}
		else if (pBuf[3] == 0) 
		{
			stFrameInfo.nFrameType = (pBuf[5] >> 3) & 0x7;
			bFindType = TRUE;
		}

		if (stFrameInfo.bHaveSeqHeader && stFrameInfo.bHaveSeqExt && bFindType) 
			break;
				
		pBuf += 4;
		nBufLen -= 4;
	}

	if (stFrameInfo.bHaveSeqHeader && stFrameInfo.bHaveSeqExt) 
	{
		stFrameInfo.nBitRate *= 400;
	}
	
	return nRet;
}

BOOL CMpeg2InfoParser::FindStartCode(BYTE* pBuffer, int nBufSize ,int& nOff)
{
	if (pBuffer == NULL || nBufSize < 5) 
	{
		return FALSE;
	}

	BYTE* pBuf =  pBuffer;
	DWORD addCount = 0;
	nOff = 0;
	for(int i = 0; i < nBufSize - 5; i ++)
	{
		if (pBuf[2] != 0x01)
		{
			if (pBuf[2] != 0)
			{
				addCount = 3;
			}
			else
			{
				if (pBuf[1] != 0)
				{
					addCount = 2;
				}
				else
				{
					if (pBuf[3] > 0x01)
						addCount = 4;
					else
						addCount = 1;
				}
			}
		}
		else
		{
			if (pBuf[0] == 0 && pBuf[1] == 0) 
			{
				return TRUE;
			}
			addCount = 3;
		}

		pBuf += addCount;
		nOff += addCount;
	}

	return FALSE;
}

int CMpeg2InfoParser::GetFrameType(BYTE* pBuffer, int nBufSize,int& nFrameType)
{
	if (pBuffer == NULL || nBufSize < 4)
	{
		return 0;
	}

	BYTE *pBuf = pBuffer;
	nFrameType = 0;

	//½âÎöÖ¡ÀàÐÍ
	BYTE addCount = 0;
	for(int i = 0; i < nBufSize - 5; i ++)
	{
		if (pBuf[2] != 0x01)
		{
			if (pBuf[2] != 0)
			{
				addCount = 3;
			}
			else
			{
				if (pBuf[1] != 0)
				{
					addCount = 2;
				}
				else
				{
					if (pBuf[3] > 0x01)
						addCount = 4;
					else
						addCount = 1;
				}
			}
		}
		else
		{
			if (pBuf[3] != 0) 
			{
				addCount = 4;
			}
			else
			{
				if (pBuf[0] == 0 && pBuf[1] == 0) 
				{
					nFrameType = (pBuf[5] >> 3) & 0x7;
					return 1;
				}
				addCount = 3;
			}
		}

		pBuf += addCount;
	}


	return 0;
}