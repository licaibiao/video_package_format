//#include "StdAfx.h"
#include "MP4Writer.h"
#include "MP4WriteImpl.h"

CMP4Writer::CMP4Writer(void)
{
}

CMP4Writer::~CMP4Writer(void)
{
}

CMP4Writer* CMP4Writer::CreateWriter()
{
	return new CMP4WriteImpl();
}

void CMP4Writer::DeleteWriter(CMP4Writer* pWriter)
{
	if(pWriter)
		delete pWriter;
}

