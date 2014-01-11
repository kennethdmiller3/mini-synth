/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III
*/
#include "StdAfx.h"

#include "Debug.h"
#include "Console.h"
#include "Math.h"
#include "Random.h"
#include "Menu.h"
#include "Keys.h"

#include "PolyBLEP.h"
#include "Oscillator.h"
#include "OscillatorLFO.h"
#include "OscillatorNote.h"
#include "SubOscillator.h"
#include "Wave.h"
#include "WavePoly.h"
#include "Envelope.h"
#include "Filter.h"

#include "DisplaySpectrumAnalyzer.h"
#include "DisplayKeyVolumeEnvelope.h"
#include "DisplayOscillatorWaveform.h"
#include "DisplayOscillatorFrequency.h"
#include "DisplayLowFrequencyOscillator.h"

BASS_INFO info;
HSTREAM stream; // the stream

// display error message, clean up, and exit
void Error(char const *text)
{
	fprintf(stderr, "Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

// window title
char const title_text[] = ">>> MINI VIRTUAL ANALOG SYNTHESIZER";

char const * const fx_name[9] =
{
	"Chorus", "Compressor", "Distortion", "Echo", "Flanger", "Gargle", "I3DL2Reverb", "ParamEQ", "Reverb"
};

// effect config
bool fx_enable;
bool fx_active[9];

// effect handles
HFX fx[9];


// output scale factor
float output_scale = 0.25f;	// 0.25f;

DWORD CALLBACK WriteStream(HSTREAM handle, short *buffer, DWORD length, void *user)
{
	// get active oscillators
	int index[KEYS];
	int active = 0;
	for (int k = 0; k < ARRAY_SIZE(keys); k++)
	{
		if (vol_env_state[k].state != EnvelopeState::OFF)
		{
			index[active++] = k;
		}
	}

	// number of samples
	size_t count = length / (2 * sizeof(short));

	if (active == 0)
	{
		// clear buffer
		memset(buffer, 0, length);

		// advance low-frequency oscillator
		float lfo = lfo_state.Update(lfo_config, 1.0f, float(count) / info.freq);

		// compute shared oscillator values
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			osc_config[o].Modulate(lfo);
		}

		return length;
	}

	// time step per output sample
	float const step = 1.0f / info.freq;

	// for each output sample...
	for (size_t c = 0; c < count; ++c)
	{
		// get low-frequency oscillator value
		float const lfo = lfo_state.Update(lfo_config, 1.0f, step);

		// compute shared oscillator values
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			osc_config[o].Modulate(lfo);
		}

		// accumulated sample value
		float sample = 0.0f;

		// for each active oscillator...
		for (int i = 0; i < active; ++i)
		{
			int k = index[i];

			// key frequency (taking octave shift into account)
			float const key_freq = keyboard_frequency[k] * keyboard_timescale;

			// update filter envelope generator
			float const flt_env_amplitude = flt_env_state[k].Update(flt_env_config, step);

			// update volume envelope generator
			float const vol_env_amplitude = vol_env_state[k].Update(vol_env_config, step);

			// if the envelope generator finished...
			if (vol_env_state[k].state == EnvelopeState::OFF)
			{
				// remove from active oscillators
				--active;
				index[i] = index[active];
				--i;
				continue;
			}

			// set up sync phases
			for (int o = 1; o < NUM_OSCILLATORS; ++o)
			{
				if (osc_config[o].sync_enable)
					osc_config[o].sync_phase = osc_config[o].frequency / osc_config[0].frequency;
			}

			// update oscillator
			// (assume key follow)
			float osc_value = 0.0f;
			for (int o = 0; o < NUM_OSCILLATORS; ++o)
			{
				if (osc_config[o].sub_osc_mode)
					osc_value += osc_config[o].sub_osc_amplitude * SubOscillator(osc_config[o], osc_state[k][o], key_freq, step);
				osc_value += osc_state[k][o].Update(osc_config[o], key_freq, step);
			}

			// update filter
			float flt_value;
			if (flt_config.enable)
			{
				// compute cutoff frequency
				// (assume key follow)
				float const cutoff = key_freq * powf(2, flt_config.cutoff_base + flt_config.cutoff_lfo * lfo + flt_config.cutoff_env * flt_env_amplitude);

				// get filtered oscillator value
				flt_value = flt_state[k].Update(flt_config, cutoff, osc_value, step);
			}
			else
			{
				// pass unfiltered value
				flt_value = osc_value;
			}

			// apply envelope to amplitude and accumulate result
			sample += flt_value * vol_env_amplitude;
		}

		// left and right channels are the same
		//short output = (short)Clamp(int(sample * output_scale * 32768), SHRT_MIN, SHRT_MAX);
		short output = (short)(FastTanh(sample * output_scale) * 32768);
		*buffer++ = output;
		*buffer++ = output;
	}

	return length;
}

void PrintOutputScale(HANDLE hOut)
{
	COORD pos = { 1, 10 };
	PrintConsole(hOut, pos, "- + Output: %5.1f%%", output_scale * 100.0f);

	DWORD written;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED, 1, pos, &written);
	pos.X += 2;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, 1, pos, &written);
}

void PrintKeyOctave(HANDLE hOut)
{
	COORD pos = { 21, 10 };
	PrintConsole(hOut, pos, "[ ] Key Octave: %d", keyboard_octave);

	DWORD written;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED, 1, pos, &written);
	pos.X += 2;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, 1, pos, &written);
}

void PrintAntialias(HANDLE hOut)
{
	COORD pos = { 41, 10 };
	PrintConsole(hOut, pos, "F12 Antialias: %3s", use_antialias ? "ON" : "OFF");

	DWORD written;
	pos.X += 15;
	FillConsoleOutputAttribute(hOut, use_antialias ? FOREGROUND_GREEN : FOREGROUND_RED, 3, pos, &written);
}

void UpdateOscillatorFrequencyDisplay(HANDLE hOut, int o);

void MenuOSC(HANDLE hOut, WORD key, DWORD modifiers, MenuMode menu)
{
	int const o = menu - MENU_OSC1;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 9 + (o > 0);
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 9 + (o > 0))
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			osc_config[o].enable = (key == VK_RIGHT);
			break;
		case 1:
			osc_config[o].wavetype = Wave((osc_config[o].wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			for (int k = 0; k < KEYS; ++k)
				osc_state[k][o].Reset();
			break;
		case 2:
			UpdatePercentageProperty(osc_config[o].waveparam_base, sign, modifiers, 0, 1);
			break;
		case 3:
			UpdateFrequencyProperty(osc_config[o].frequency_base, sign, modifiers, -5, 5);
			break;
		case 4:
			UpdatePercentageProperty(osc_config[o].amplitude_base, sign, modifiers, -10, 10);
			break;
		case 5:
			UpdatePercentageProperty(osc_config[o].waveparam_lfo, sign, modifiers, -10, 10);
			break;
		case 6:
			UpdateFrequencyProperty(osc_config[o].frequency_lfo, sign, modifiers, -5, 5);
			break;
		case 7:
			UpdatePercentageProperty(osc_config[o].amplitude_lfo, sign, modifiers, -10, 10);
			break;
		case 8:
			osc_config[o].sub_osc_mode = SubOscillatorMode((osc_config[o].sub_osc_mode + sign + SUBOSC_COUNT) % SUBOSC_COUNT);
			break;
		case 9:
			UpdatePercentageProperty(osc_config[o].sub_osc_amplitude, sign, modifiers, -10, 10);
			break;
		case 10:
			if (key == VK_RIGHT)
			{
				osc_config[o].sync_enable = true;
			}
			else
			{
				osc_config[o].sync_enable = false;
				osc_config[o].sync_phase = 1.0f;
			}
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = osc_config[o].enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, NULL, "OFF");

	for (int i = 1; i <= 9 + (o > 0); ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", wave_name[osc_config[o].wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", osc_config[o].waveparam_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Frequency: %+7.2f", osc_config[o].frequency_base * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Amplitude:% 7.1f%%", osc_config[o].amplitude_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Width LFO: %+6.1f%%", osc_config[o].waveparam_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq LFO:  %+7.2f", osc_config[o].frequency_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Ampl LFO: %+7.1f%%", osc_config[o].amplitude_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Sub Osc:%10s", sub_osc_name[osc_config[o].sub_osc_mode]);

	++pos.Y;
	PrintConsole(hOut, pos, "Sub Ampl: % 7.1f%%", osc_config[o].sub_osc_amplitude * 100.0f);

	if (o > 0)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "Hard Sync:     %3s", osc_config[o].sync_enable ? "ON" : "OFF");
		WORD const attrib = menu_item_attrib[item_selected == 10];
		COORD const pos2 = { pos.X + 15, pos.Y };
		FillConsoleOutputAttribute(hOut, (attrib & 0xF8) | (osc_config[o].sync_enable ? FOREGROUND_GREEN : FOREGROUND_RED), 3, pos2, &written);
	}
}

void MenuOSC1(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuOSC(hOut, key, modifiers, MENU_OSC1);
}

void MenuOSC2(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuOSC(hOut, key, modifiers, MENU_OSC2);
}

void MenuLFO(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_LFO;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 3;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 3)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			lfo_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			lfo_config.wavetype = Wave((lfo_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			lfo_state.Reset();
			break;
		case 2:
			UpdatePercentageProperty(lfo_config.waveparam, sign, modifiers, 0, 1);
			break;
		case 3:
			UpdateFrequencyProperty(lfo_config.frequency_base, sign, modifiers, -8, 14);
			lfo_config.frequency = powf(2.0f, lfo_config.frequency_base);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = lfo_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 3; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", wave_name[lfo_config.wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", lfo_config.waveparam * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq: %10.3fHz", lfo_config.frequency);
}

void MenuFLT(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_FLT;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 9;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 9)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			flt_config.enable = (key == VK_RIGHT);
			flt_env_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			flt_config.mode = FilterConfig::Mode((flt_config.mode + FilterConfig::COUNT + sign) % FilterConfig::COUNT);
			break;
		case 2:
			UpdatePercentageProperty(flt_config.resonance, sign, modifiers, 0, 4);
			break;
		case 3:
			UpdateFrequencyProperty(flt_config.cutoff_base, sign, modifiers, -10, 10);
			break;
		case 4:
			UpdateFrequencyProperty(flt_config.cutoff_lfo, sign, modifiers, -10, 10);
			break;
		case 5:
			UpdateFrequencyProperty(flt_config.cutoff_env, sign, modifiers, -10, 10);
			break;
		case 6:
			UpdateTimeProperty(flt_env_config.attack_time, sign, modifiers, 0, 10);
			flt_env_config.attack_rate = 1.0f / Max(flt_env_config.attack_time, 0.0001f);
			break;
		case 7:
			UpdateTimeProperty(flt_env_config.decay_time, sign, modifiers, 0, 10);
			flt_env_config.decay_rate = 1.0f / Max(flt_env_config.decay_time, 0.0001f);
			break;
		case 8:
			UpdatePercentageProperty(flt_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 9:
			UpdateTimeProperty(flt_env_config.release_time, sign, modifiers, 0, 10);
			flt_env_config.release_rate = 1.0f / Max(flt_env_config.release_time, 0.0001f);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = flt_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 9; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d FLT         %3s", menu + 1, flt_config.enable ? "ON" : "OFF");

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", filter_name[flt_config.mode]);

	++pos.Y;
	PrintConsole(hOut, pos, "Resonance: % 7.3f", flt_config.resonance);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff:    % 7.2f", flt_config.cutoff_base * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff LFO:% 7.2f", flt_config.cutoff_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff ENV:% 7.2f", flt_config.cutoff_env * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %7.3fs", flt_env_config.attack_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %7.3fs", flt_env_config.decay_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", flt_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %7.3fs", flt_env_config.release_time);
}

void MenuVOL(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_VOL;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 4;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 4)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			vol_env_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			UpdateTimeProperty(vol_env_config.attack_time, sign, modifiers, 0, 10);
			vol_env_config.attack_rate = 1.0f / Max(vol_env_config.attack_time, 0.0001f);
			break;
		case 2:
			UpdateTimeProperty(vol_env_config.decay_time, sign, modifiers, 0, 10);
			vol_env_config.decay_rate = 1.0f / Max(vol_env_config.decay_time, 0.0001f);
			break;
		case 3:
			UpdatePercentageProperty(vol_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 4:
			UpdateTimeProperty(vol_env_config.release_time, sign, modifiers, 0, 10);
			vol_env_config.release_rate = 1.0f / Max(vol_env_config.release_time, 0.0001f);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = vol_env_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 4; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d VOL", menu + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %7.3fs", vol_env_config.attack_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %7.3fs", vol_env_config.decay_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", vol_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %7.3fs", vol_env_config.release_time);
}

static void EnableEffect(int index, bool enable)
{
	if (enable)
	{
		if (!fx[index])
		{
			// set the effect, not bothering with parameters (use defaults)
			fx[index] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + index, 0);
		}
	}
	else
	{
		if (fx[index])
		{
			BASS_ChannelRemoveFX(stream, fx[index]);
			fx[index] = 0;
		}
	}
}

void MenuFX(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_FX;
	COORD pos = menu_pos[menu];
	DWORD written;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = ARRAY_SIZE(fx);
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > ARRAY_SIZE(fx))
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		if (menu_item[menu] == 0)
		{
			// set master switch
			fx_enable = (key == VK_RIGHT);
			for (int i = 0; i < ARRAY_SIZE(fx); ++i)
				EnableEffect(i, fx_enable && fx_active[i]);
		}
		else
		{
			// set item
			fx_active[menu_item[menu] - 1] = (key == VK_RIGHT);
			EnableEffect(menu_item[menu] - 1, fx_enable && fx_active[menu_item[menu] - 1]);
		}
		break;
	}

	// menu title
	bool const component_enabled = fx_enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 0; i < ARRAY_SIZE(fx); ++i)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "%-11s    %3s", fx_name[i], fx_active[i] ? "ON" : "OFF");
		WORD const attrib = menu_item_attrib[item_selected == i + 1];
		FillConsoleOutputAttribute(hOut, attrib, 15, pos, &written);
		COORD const pos2 = { pos.X + 15, pos.Y };
		FillConsoleOutputAttribute(hOut, (attrib & 0xF8) | (fx_active[i] ? FOREGROUND_GREEN : FOREGROUND_RED), 3, pos2, &written);
	}
}

typedef void(*MenuFunc)(HANDLE hOut, WORD key, DWORD modifiers);
MenuFunc menufunc[MENU_COUNT] =
{
	MenuOSC1, MenuOSC2, MenuLFO, MenuFLT, MenuVOL, MenuFX
};

void main(int argc, char **argv)
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	int running = 1;

#ifdef WIN32
	if (IsDebuggerPresent())
	{
		// turn on floating-point exceptions
		unsigned int prev;
		_clearfp();
		_controlfp_s(&prev, 0, _EM_ZERODIVIDE|_EM_INVALID);
	}
#endif

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
	{
		fprintf(stderr, "An incorrect version of BASS.DLL was loaded");
		return;
	}

	// set the window title
	SetConsoleTitle(TEXT(title_text));

	// set the console buffer size
	static const COORD bufferSize = { 80, 50 };
	SetConsoleScreenBufferSize(hOut, bufferSize);

	// set the console window size
	static const SMALL_RECT windowSize = { 0, 0, 79, 49 };
	SetConsoleWindowInfo(hOut, TRUE, &windowSize);

	// clear the window
	Clear(hOut);

	// hide the cursor
	static const CONSOLE_CURSOR_INFO cursorInfo = { 100, FALSE };
	SetConsoleCursorInfo(hOut, &cursorInfo);

	// set input mode
	SetConsoleMode(hIn, 0);

	// 10ms update period
	const int STREAM_UPDATE_PERIOD = 10;
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, STREAM_UPDATE_PERIOD);

	// initialize BASS sound library
	if (!BASS_Init(-1, 48000, BASS_DEVICE_LATENCY, 0, NULL))
		Error("Can't initialize device");

	// get device info
	BASS_GetInfo(&info);
	DebugPrint("device latency: %dms\n", info.latency);
	DebugPrint("device minbuf: %dms\n", info.minbuf);
	DebugPrint("ds version: %d (effects %s)\n", info.dsver, info.dsver < 8 ? "disabled" : "enabled");

	// default buffer size = update period + 'minbuf' + 1ms extra margin
	BASS_SetConfig(BASS_CONFIG_BUFFER, STREAM_UPDATE_PERIOD + info.minbuf + 1);
	DebugPrint("using a %dms buffer\r", BASS_GetConfig(BASS_CONFIG_BUFFER));

	// if the device's output rate is unknown default to 44100 Hz
	if (!info.freq) info.freq = 44100;

	// create a stream, stereo so that effects sound nice
	stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);

#ifdef BANDLIMITED_SAWTOOTH
	// initialize bandlimited sawtooth tables
	InitSawtooth();
#endif

	// initialize polynomial noise tables
	InitPoly();

	// compute frequency for each note key
	for (int k = 0; k < KEYS; ++k)
	{
		keyboard_frequency[k] = powf(2.0F, (k + 3) / 12.0f) * 220.0f;	// middle C = 261.626Hz; A above middle C = 440Hz
		//keyboard_frequency[k] = powF(2.0F, k / 12.0f) * 256.0f;		// middle C = 256Hz
	}

	// initialize key display
	InitKeyVolumeEnvelopeDisplay(hOut);

	// show output scale and key octave
	PrintOutputScale(hOut);
	PrintKeyOctave(hOut);
	PrintAntialias(hOut);

	// enable the first oscillator
	osc_config[0].enable = true;

	// show all menus
	for (int i = 0; i < MENU_COUNT - (info.dsver < 8); ++i)
		menufunc[i](hOut, 0, 0);

	// start playing the audio stream
	BASS_ChannelPlay(stream, FALSE);

	// previous volume envelope state for each key
	EnvelopeState::State vol_env_display[KEYS] = { EnvelopeState::OFF };

	while (running)
	{
		// if there are any pending input events...
		DWORD numEvents = 0;
		while (GetNumberOfConsoleInputEvents(hIn, &numEvents) && numEvents > 0)
		{
			// get the next input event
			INPUT_RECORD keyin;
			ReadConsoleInput(hIn, &keyin, 1, &numEvents);
			if (keyin.EventType == KEY_EVENT)
			{
				// handle interface keys
				if (keyin.Event.KeyEvent.bKeyDown)
				{
					WORD code = keyin.Event.KeyEvent.wVirtualKeyCode;
					DWORD modifiers = keyin.Event.KeyEvent.dwControlKeyState;
					if (code == VK_ESCAPE)
					{
						running = 0;
						break;
					}
					else if (code == VK_OEM_MINUS || code == VK_SUBTRACT)
					{
						output_scale -= 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_PLUS || code == VK_ADD)
					{
						output_scale += 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_4)	// '['
					{
						if (keyboard_octave > 0)
						{
							--keyboard_octave;
							keyboard_timescale *= 0.5f;
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_OEM_6)	// ']'
					{
						if (keyboard_octave < 8)
						{
							++keyboard_octave;
							keyboard_timescale *= 2.0f;
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_F12)
					{
						use_antialias = !use_antialias;
						PrintAntialias(hOut);
					}
					else if (code >= VK_F1 && code < VK_F1 + MENU_COUNT - (info.dsver < 8))
					{
						int prev_active = menu_active;
						menu_active = MENU_COUNT;
						menufunc[prev_active](hOut, 0, 0);
						menu_active = MenuMode(code - VK_F1);
						menufunc[menu_active](hOut, 0, 0);
					}
					else if (code == VK_TAB)
					{
						int prev_active = menu_active;
						menu_active = MENU_COUNT;
						menufunc[prev_active](hOut, 0, 0);
						if (modifiers & SHIFT_PRESSED)
							menu_active = MenuMode(prev_active == 0 ? MENU_COUNT - 1 : prev_active - 1);
						else
							menu_active = MenuMode(prev_active == MENU_COUNT - 1 ? 0 : prev_active + 1);
						menufunc[menu_active](hOut, 0, 0);
					}
					else if (code == VK_UP || code == VK_DOWN || code == VK_RIGHT || code == VK_LEFT)
					{
						menufunc[menu_active](hOut, code, modifiers);
					}
				}

				// handle note keys
				for (int k = 0; k < KEYS; k++)
				{
					if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[k])
					{
						// gate filters based on key down
						bool gate = (keyin.Event.KeyEvent.bKeyDown != 0);

						// gate the filter envelope
						flt_env_state[k].Gate(flt_env_config, gate);

						// if pressing the key
						if (gate)
						{
							// save the note key
							keyboard_most_recent = k;

							// if the volume envelope is currently off...
							if (vol_env_state[k].state == EnvelopeState::OFF)
							{
								// start the oscillator
								// (assume restart on key)
								for (int o = 0; o < NUM_OSCILLATORS; ++o)
									osc_state[k][o].Start();

								// start the filter
								flt_state[k].Reset();
							}
						}

						// gate the volume envelope
						vol_env_state[k].Gate(vol_env_config, gate);
						break;
					}
				}
			}
		}

		// center frequency of the zeroth semitone band
		// (one octave down from the lowest key)
		float const freq_min = keyboard_frequency[0] * keyboard_timescale * 0.5f;

		// update the spectrum analyzer display
		UpdateSpectrumAnalyzer(hOut, stream, info, freq_min);

		// update note key volume envelope display
		UpdateKeyVolumeEnvelopeDisplay(hOut, vol_env_display);

		// update the oscillator waveform display
		UpdateOscillatorWaveformDisplay(hOut, info, keyboard_most_recent);

		// update the oscillator frequency displays
		for (int o = 0; o < 2; ++o)
		{
			if (osc_config[o].enable)
				UpdateOscillatorFrequencyDisplay(hOut, keyboard_most_recent, o);
		}

		// update the low-frequency oscillator dispaly
		UpdateLowFrequencyOscillatorDisplay(hOut);

		// sleep for 1/60th of second
		Sleep(16);
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
