#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Spectrum Analyzer Display
*/

#define SPECTRUM_WIDTH 80
#define SPECTRUM_HEIGHT 8

void UpdateSpectrumAnalyzer(HANDLE hOut, DWORD stream, BASS_INFO const &info, float const freq_min);
