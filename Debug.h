#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Debug/Error Functions
*/

// formatted debugger output
extern int DebugPrint(char const *format, ...);

// get the last error message
extern void GetLastErrorMessage(CHAR buf[], DWORD size);
