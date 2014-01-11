/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Oscillator Frequency Display
*/
#include "StdAfx.h"

#include "DisplayOscillatorFrequency.h"
#include "Menu.h"
#include "Keys.h"
#include "Console.h"
#include "OscillatorNote.h"

// show oscillator frequency
void UpdateOscillatorFrequencyDisplay(HANDLE hOut, int const k, int const o)
{
	// key frequency (taking octave shift into account)
	float const key_freq = keyboard_frequency[k] * keyboard_timescale;

	// get attributes to use
	MenuMode menu = MenuMode(MENU_OSC1 + o);
	COORD const pos = { menu_pos[menu].X + 8, menu_pos[menu].Y };
	bool const menu_selected = (menu_active == menu);
	bool const title_selected = menu_selected && menu_item[menu] == 0;
	WORD const title_attrib = menu_title_attrib[true][menu_selected + title_selected];
	WORD const num_attrib = (title_attrib & 0xF8) | (FOREGROUND_GREEN);
	WORD const unit_attrib = (title_attrib & 0xF8) | (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// current frequency in Hz
	float const freq = key_freq * osc_config[o].frequency;

	if (freq >= 20000.0f)
	{
		// print in kHz
		PrintConsoleWithAttribute(hOut, pos, num_attrib, "%7.2f", freq / 1000.0f);
		COORD const pos2 = { pos.X + 7, pos.Y };
		PrintConsoleWithAttribute(hOut, pos2, unit_attrib, "kHz");
	}
	else
	{
		// print in Hz
		PrintConsoleWithAttribute(hOut, pos, num_attrib, "%8.2f", freq);
		COORD const pos2 = { pos.X + 8, pos.Y };
		PrintConsoleWithAttribute(hOut, pos2, unit_attrib, "Hz");
	}
}
