/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator Display
*/
#include "StdAfx.h"

#include "DisplayLowFrequencyOscillator.h"
#include "Menu.h"
#include "MenuLFO.h"
#include "Math.h"
#include "OscillatorLFO.h"

// local position
static COORD const pos = { 0, 0 };
static COORD const size = { 18, 1 };

// plotting characters
static CHAR_INFO const negative = { 0, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static CHAR_INFO const positive = { 0, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static WORD const plot[2] = { 221, 222 };

void UpdateLowFrequencyOscillatorDisplay(HANDLE hOut)
{
	// initialize buffer
	CHAR_INFO buf[18];
	for (int x = 0; x < 9; x++)
		buf[x] = negative;
	for (int x = 9; x < 18; ++x)
		buf[x] = positive;

	// plot low-frequency oscillator value
	float const lfo = lfo_state.Update(lfo_config, 0.0f);
	int const grid_x = Clamp(FloorInt(18.0f * lfo + 18.0f), 0, 35);
	buf[grid_x / 2].Char.UnicodeChar = plot[grid_x & 1];

	// draw the gauge
	SMALL_RECT region = {
		Menu::menu_lfo.pos.X, Menu::menu_lfo.pos.Y + 4,
		Menu::menu_lfo.pos.X + 19, Menu::menu_lfo.pos.Y + 4
	};
	WriteConsoleOutput(hOut, &buf[0], size, pos, &region);
}
