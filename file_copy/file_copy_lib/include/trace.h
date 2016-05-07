#pragma once
// TRACE macro for win32
#include <crtdbg.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _DEBUG
#ifndef TRACE
#define TRACEMAXSTRING	1024

inline void TRACE(const char* format, ...)
{
	char szBuffer[TRACEMAXSTRING];
	va_list args;
	va_start(args, format);
	int nBuf;
	nBuf = _vsnprintf_s(szBuffer,
		TRACEMAXSTRING,
		TRACEMAXSTRING,
		format,
		args);
	va_end(args);

	_RPT0(_CRT_WARN, szBuffer);
}

inline void TRACE(const wchar_t* format, ...)
{
	wchar_t szBuffer[TRACEMAXSTRING];
	va_list args;
	va_start(args, format);
	int nBuf;
	nBuf = _vsnwprintf_s(szBuffer,
		TRACEMAXSTRING,
		TRACEMAXSTRING,
		format,
		args);
	va_end(args);

	_RPTW0(_CRT_WARN, szBuffer);
}
#define TRACEF _snprintf(szBuffer,TRACEMAXSTRING,"%s(%d): ", \
				&strrchr(__FILE__,'\\')[1],__LINE__); \
				_RPT0(_CRT_WARN,szBuffer); \
				TRACE
#endif // #ifndef TRACE
#else
// Remove for release mode
#ifndef TRACE
#define TRACE 
#define TRACEF
#endif
#endif // #ifdef _DEBUG