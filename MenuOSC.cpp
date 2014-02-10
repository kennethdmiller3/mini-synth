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
	OSC menu_osc[NUM_OSCILLATORS] =
	{
		OSC(0, { 1, 12 }, "F1 OSC1", OSC::COUNT - 1),
		OSC(1, { 21, 12 }, "F2 OSC2", OSC::COUNT),
	};

	// oscillator menus
	void OSC::Update(int index, int sign, DWORD modifiers)
	{
		NoteOscillatorConfig &config = osc_config[osc];
		switch (index)
		{
		case TITLE:
			config.enable = sign > 0;
			break;
		case WAVETYPE:
			config.SetWaveType(Wave((config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT));
			for (int v = 0; v < VOICES; ++v)
				osc_state[v][osc].Reset();
			break;
		case WAVEPARAM_BASE:
			UpdatePercentageProperty(config.waveparam_base, sign, modifiers, 0, 1);
			break;
		case FREQUENCY_BASE:
			UpdatePitchProperty(config.frequency_base, sign, modifiers, -5, 5);
			break;
		case AMPLITUDE_BASE:
			UpdatePercentageProperty(config.amplitude_base, sign, modifiers, -10, 10);
			break;
		case WAVEPARAM_LFO:
			UpdatePercentageProperty(config.waveparam_lfo, sign, modifiers, -10, 10);
			break;
		case FREQUENCY_LFO:
			UpdatePitchProperty(config.frequency_lfo, sign, modifiers, -5, 5);
			break;
		case AMPLITUDE_LFO:
			UpdatePercentageProperty(config.amplitude_lfo, sign, modifiers, -10, 10);
			break;
		case KEY_FOLLOW:
			UpdatePercentageProperty(config.key_follow, sign, modifiers, -2, 2);
			break;
		case SUB_OSC_MODE:
			config.sub_osc_mode = SubOscillatorMode((config.sub_osc_mode + sign + SUBOSC_COUNT) % SUBOSC_COUNT);
			break;
		case SUB_OSC_AMPLITUDE:
			UpdatePercentageProperty(config.sub_osc_amplitude, sign, modifiers, -10, 10);
			break;
		case HARD_SYNC:
			config.sync_enable = sign > 0;
			config.sync_phase = 1.0f;
			break;
		default:
			__assume(0);
		}
	}

	void OSC::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		NoteOscillatorConfig &config = osc_config[osc];
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, config.enable, flags, NULL, "OFF");
			break;
		case WAVETYPE:
			PrintItemString(hOut, pos, flags, "%-18s", wave_name[config.wavetype]);
			break;
		case WAVEPARAM_BASE:
			PrintItemFloat(hOut, pos, flags, "Width:     % 6.1f%%", config.waveparam_base * 100.0f);
			break;
		case FREQUENCY_BASE:
			PrintItemFloat(hOut, pos, flags, "Frequency: %+7.2f", config.frequency_base * 12.0f);
			break;
		case AMPLITUDE_BASE:
			PrintItemFloat(hOut, pos, flags, "Amplitude:% 7.1f%%", config.amplitude_base * 100.0f);
			break;
		case WAVEPARAM_LFO:
			PrintItemFloat(hOut, pos, flags, "Width LFO: %+6.1f%%", config.waveparam_lfo * 100.0f);
			break;
		case FREQUENCY_LFO:
			PrintItemFloat(hOut, pos, flags, "Freq LFO:  %+7.2f", config.frequency_lfo * 12.0f);
			break;
		case AMPLITUDE_LFO:
			PrintItemFloat(hOut, pos, flags, "Ampl LFO: %+7.1f%%", config.amplitude_lfo * 100.0f);
			break;
		case KEY_FOLLOW:
			PrintItemFloat(hOut, pos, flags, "Key Follow:% 6.1f%%", config.key_follow * 100.0f);
			break;
		case SUB_OSC_MODE:
			PrintItemString(hOut, pos, flags, "Sub Osc:%10s", sub_osc_name[config.sub_osc_mode]);
			break;
		case SUB_OSC_AMPLITUDE:
			PrintItemFloat(hOut, pos, flags, "Sub Ampl: % 7.1f%%", config.sub_osc_amplitude * 100.0f);
			break;
		case HARD_SYNC:
			PrintItemBool(hOut, pos, flags, "Hard Sync:     ", config.sync_enable);
			break;
		default:
			__assume(0);
		}
	}
}