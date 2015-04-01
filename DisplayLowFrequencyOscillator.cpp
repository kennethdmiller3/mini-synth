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

void DisplayLowFrequencyOscillator::Update(HANDLE hOut)
{
	// initialize buffer
	CHAR_INFO *buf = static_cast<CHAR_INFO *>(_alloca(size.X * sizeof(CHAR_INFO)));
	for (int x = 0; x < size.X / 2; x++)
		buf[x] = negative;
	for (int x = size.X / 2; x < size.X; ++x)
		buf[x] = positive;

	// plot low-frequency oscillator value
	float const lfo = lfo_state.Update(lfo_config, 0.0f);
	int const grid_x = Clamp(FloorInt(size.X * lfo + size.X), 0, size.X * 2 - 1);
	buf[grid_x / 2].Char.UnicodeChar = plot[grid_x & 1];

	// draw the gauge
	SMALL_RECT region = {
		Menu::menu_lfo.rect.Left, Menu::menu_lfo.rect.Bottom,
		Menu::menu_lfo.rect.Right + 1, Menu::menu_lfo.rect.Bottom
	};
	WriteConsoleOutput(hOut, &buf[0], size, pos, &region);
}
