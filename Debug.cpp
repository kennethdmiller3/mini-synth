/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Debug Functions
*/
#include "StdAfx.h"

#include "Debug.h"

// formatted debugger output
int DebugPrint(char const *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf_s(buf, sizeof(buf), format, ap);
	OutputDebugStringA(buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

// get the last error message
void GetLastErrorMessage(CHAR buf[], DWORD size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, size, NULL);
}
