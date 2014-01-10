/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Console Functions
*/
#include "StdAfx.h"

#include "Console.h"

// formatted write console output
int PrintConsole(HANDLE out, COORD pos, char const *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char buf[256];
	vsnprintf_s(buf, sizeof(buf), format, ap);
	DWORD written;
	WriteConsoleOutputCharacter(out, buf, strlen(buf), pos, &written);
	return written;
}

// formatted write console output with an attribute
int PrintConsoleWithAttribute(HANDLE out, COORD pos, WORD attrib, char const *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char buf[256];
	vsnprintf_s(buf, sizeof(buf), format, ap);
	DWORD written;
	WriteConsoleOutputCharacter(out, buf, strlen(buf), pos, &written);
	FillConsoleOutputAttribute(out, attrib, written, pos, &written);
	return written;
}

// clear the console window
void Clear(HANDLE hOut)
{
	CONSOLE_SCREEN_BUFFER_INFO bufInfo;
	COORD osc_phase = { 0, 0 };
	DWORD written;
	DWORD size;
	GetConsoleScreenBufferInfo(hOut, &bufInfo);
	size = bufInfo.dwSize.X * bufInfo.dwSize.Y;
	FillConsoleOutputCharacter(hOut, (TCHAR)' ', size, osc_phase, &written);
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, size, osc_phase, &written);
	SetConsoleCursorPosition(hOut, osc_phase);
}
