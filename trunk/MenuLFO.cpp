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
	namespace LFO
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			switch (index)
			{
			case LFO_TITLE:
				lfo_config.enable = sign > 0;
				break;
			case LFO_WAVETYPE:
				lfo_config.SetWaveType(Wave((lfo_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT));
				lfo_state.Reset();
				break;
			case LFO_WAVEPARAM:
				UpdatePercentageProperty(lfo_config.waveparam, sign, modifiers, 0, 1);
				break;
			case LFO_FREQUENCY:
				UpdateFrequencyProperty(lfo_config.frequency_base, sign, modifiers, -8, 14);
				lfo_config.frequency = powf(2.0f, lfo_config.frequency_base);
				break;
			default:
				__assume(0);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			switch (index)
			{
			case LFO_TITLE:
				PrintMenuTitle(menu, hOut, pos, lfo_config.enable, flags, " ON", "OFF");
				break;
			case LFO_WAVETYPE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "%-18s", wave_name[lfo_config.wavetype]);
				break;
			case LFO_WAVEPARAM:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Width:     % 6.1f%%", lfo_config.waveparam * 100.0f);
				break;
			case LFO_FREQUENCY:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Freq: %10.3fHz", lfo_config.frequency);
				break;
			default:
				__assume(0);
			}
		}
	}
}
