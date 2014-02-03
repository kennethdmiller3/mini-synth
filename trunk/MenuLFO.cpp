/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator Menu
*/
#include "StdAfx.h"

#include "MenuLFO.h"
#include "Console.h"
#include "OscillatorLFO.h"

namespace Menu
{
	LFO menu_lfo({ 41, 12 }, "F3 LFO", LFO::COUNT);

	void LFO::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			lfo_config.enable = sign > 0;
			break;
		case WAVETYPE:
			lfo_config.SetWaveType(Wave((lfo_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT));
			lfo_state.Reset();
			break;
		case WAVEPARAM:
			UpdatePercentageProperty(lfo_config.waveparam, sign, modifiers, 0, 1);
			break;
		case FREQUENCY:
			UpdatePitchProperty(lfo_config.frequency_base, sign, modifiers, -8, 14);
			lfo_config.frequency = powf(2.0f, lfo_config.frequency_base);
			break;
		default:
			__assume(0);
		}
	}

	void LFO::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, lfo_config.enable, flags, " ON", "OFF");
			break;
		case WAVETYPE:
			PrintItemString(hOut, pos, flags, "%-18s", wave_name[lfo_config.wavetype]);
			break;
		case WAVEPARAM:
			PrintItemFloat(hOut, pos, flags, "Width:     % 6.1f%%", lfo_config.waveparam * 100.0f);
			break;
		case FREQUENCY:
			PrintItemFloat(hOut, pos, flags, "Freq: %10.3fHz", lfo_config.frequency);
			break;
		default:
			__assume(0);
		}
	}
}
