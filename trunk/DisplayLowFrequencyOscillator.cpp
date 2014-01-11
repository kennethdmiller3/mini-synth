/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator Display
*/
#include "StdAfx.h"

#include "DisplayLowFrequencyOscillator.h"
#include "Menu.h"
#include "Math.h"
#include "OscillatorLFO.h"

void UpdateLowFrequencyOscillatorDisplay(HANDLE hOut)
{
	// initialize buffer
	CHAR_INFO const negative = { 0, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
	CHAR_INFO const positive = { 0, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
	CHAR_INFO buf[18];
	for (int x = 0; x < 9; x++)
		buf[x] = negative;
	for (int x = 9; x < 18; ++x)
		buf[x] = positive;

	// plot low-frequency oscillator value
	CHAR const plot[2] = { 221, 222 };
	float const lfo = lfo_state.Update(lfo_config, 1.0f, 0.0f);
	int grid_x = Clamp(FloorInt(18.0f * lfo + 18.0f), 0, 35);
	buf[grid_x / 2].Char.AsciiChar = plot[grid_x & 1];

	// draw the gauge
	COORD const pos = { 0, 0 };
	COORD const size = { 18, 1 };
	SMALL_RECT region = {
		menu_pos[MENU_LFO].X, menu_pos[MENU_LFO].Y + 4,
		menu_pos[MENU_LFO].X + 19, menu_pos[MENU_LFO].Y + 4
	};
	WriteConsoleOutput(hOut, &buf[0], size, pos, &region);
}
