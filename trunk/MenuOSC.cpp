/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Note Oscillator Menu
*/
#include "StdAfx.h"

#include "MenuOsc.h"
#include "Console.h"
#include "OscillatorNote.h"
#include "Voice.h"

namespace Menu
{
	// oscillator menus
	namespace OSC
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			NoteOscillatorConfig &config = osc_config[menu - MENU_OSC1];
			switch (index)
			{
			case OSC_TITLE:
				config.enable = sign > 0;
				break;
			case OSC_WAVETYPE:
				config.wavetype = Wave((config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
				for (int k = 0; k < VOICES; ++k)
					osc_state[k][menu - MENU_OSC1].Reset();
				break;
			case OSC_WAVEPARAM_BASE:
				UpdatePercentageProperty(config.waveparam_base, sign, modifiers, 0, 1);
				break;
			case OSC_FREQUENCY_BASE:
				UpdateFrequencyProperty(config.frequency_base, sign, modifiers, -5, 5);
				break;
			case OSC_AMPLITUDE_BASE:
				UpdatePercentageProperty(config.amplitude_base, sign, modifiers, -10, 10);
				break;
			case OSC_WAVEPARAM_LFO:
				UpdatePercentageProperty(config.waveparam_lfo, sign, modifiers, -10, 10);
				break;
			case OSC_FREQUENCY_LFO:
				UpdateFrequencyProperty(config.frequency_lfo, sign, modifiers, -5, 5);
				break;
			case OSC_AMPLITUDE_LFO:
				UpdatePercentageProperty(config.amplitude_lfo, sign, modifiers, -10, 10);
				break;
			case OSC_SUB_OSC_MODE:
				config.sub_osc_mode = SubOscillatorMode((config.sub_osc_mode + sign + SUBOSC_COUNT) % SUBOSC_COUNT);
				break;
			case OSC_SUB_OSC_AMPLITUDE:
				UpdatePercentageProperty(config.sub_osc_amplitude, sign, modifiers, -10, 10);
				break;
			case OSC_HARD_SYNC:
				config.sync_enable = sign > 0;
				config.sync_phase = 1.0f;
				break;
			default:
				__assume(0);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			NoteOscillatorConfig &config = osc_config[menu - MENU_OSC1];
			switch (index)
			{
			case OSC_TITLE:
				PrintMenuTitle(menu, hOut, pos, config.enable, flags, NULL, "OFF");
				break;
			case OSC_WAVETYPE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "%-18s", wave_name[config.wavetype]);
				break;
			case OSC_WAVEPARAM_BASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Width:     % 6.1f%%", config.waveparam_base * 100.0f);
				break;
			case OSC_FREQUENCY_BASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Frequency: %+7.2f", config.frequency_base * 12.0f);
				break;
			case OSC_AMPLITUDE_BASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Amplitude:% 7.1f%%", config.amplitude_base * 100.0f);
				break;
			case OSC_WAVEPARAM_LFO:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Width LFO: %+6.1f%%", config.waveparam_lfo * 100.0f);
				break;
			case OSC_FREQUENCY_LFO:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Freq LFO:  %+7.2f", config.frequency_lfo * 12.0f);
				break;
			case OSC_AMPLITUDE_LFO:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Ampl LFO: %+7.1f%%", config.amplitude_lfo * 100.0f);
				break;
			case OSC_SUB_OSC_MODE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Sub Osc:%10s", sub_osc_name[config.sub_osc_mode]);
				break;
			case OSC_SUB_OSC_AMPLITUDE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Sub Ampl: % 7.1f%%", config.sub_osc_amplitude * 100.0f);
				break;
			case OSC_HARD_SYNC:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Hard Sync:     ");
				if (config.sync_enable)
					PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, (item_attrib[flags] & 0xF8) | FOREGROUND_GREEN, " ON");
				else
					PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, (item_attrib[flags] & 0xF8) | FOREGROUND_RED, "OFF");
				break;
			default:
				__assume(0);
			}
		}
	}
}