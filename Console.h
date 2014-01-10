#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Console Functions
*/

// formatted write console output
extern int PrintConsole(HANDLE out, COORD pos, char const *format, ...);

// formatted write console output with an attribute
extern int PrintConsoleWithAttribute(HANDLE out, COORD pos, WORD attrib, char const *format, ...);

// clear the console window
extern void Clear(HANDLE hOut);
