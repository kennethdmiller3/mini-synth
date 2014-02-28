/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter Frequency Display
*/
#include "StdAfx.h"

#include "DisplayFilterFrequency.h"
#include "Menu.h"
#include "MenuFLT.h"
#include "Voice.h"
#include "Control.h"
#include "Console.h"
#include "OscillatorLFO.h"
#include "Filter.h"

// show filter frequency
void DisplayFilterFrequency::Update(HANDLE hOut, int const v)
{
	// get low-frequency filter value
	// (assume it is constant for the duration)
	float const lfo = lfo_state.Update(lfo_config, 0.0f);

	// get filter envelope generator amplitude
	float const flt_env_amplitude = flt_env_state[v].amplitude;

	// key velocity
	float const key_vel = voice_vel[v] / 64.0f;

	// filter key frequency (taking key follow and pitch wheel control into account)
	float const flt_key_freq = NoteFrequency(voice_note[v], flt_config.key_follow);

	// get attributes to use
	COORD const pos = { Menu::menu_flt.pos.X + 8, Menu::menu_flt.pos.Y };
	bool const selected = (Menu::active_page == Menu::PAGE_MAIN && Menu::active_menu == Menu::MAIN_FLT);
	bool const title_selected = selected && Menu::menu_flt.item == 0;
	WORD const title_attrib = Menu::title_attrib[true][selected + title_selected];
	WORD const num_attrib = (title_attrib & 0xF8) | (FOREGROUND_GREEN);
	WORD const unit_attrib = (title_attrib & 0xF8) | (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// current frequency in Hz
	float const freq = flt_key_freq * flt_config.GetCutoff(lfo, flt_env_amplitude, key_vel);

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
