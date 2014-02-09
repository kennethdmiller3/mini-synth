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
#include "Voice.h"
#include "Midi.h"
#include "Control.h"

#include "PolyBLEP.h"
#include "Oscillator.h"
#include "OscillatorLFO.h"
#include "OscillatorNote.h"
#include "SubOscillator.h"
#include "Wave.h"
#include "Filter.h"
#include "Amplifier.h"
#include "Effect.h"

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

// output scale factor
float output_scale = 0.25f;	// 0.25f;

static size_t const BLOCK_UPDATE_SAMPLES = 16;

// apply low-frequency oscillator value
static void ApplyLFO(float lfo)
{
	// compute shared oscillator values
	for (int o = 0; o < NUM_OSCILLATORS; ++o)
	{
		osc_config[o].Modulate(lfo);
	}

	// set up sync phases
	for (int o = 1; o < NUM_OSCILLATORS; ++o)
	{
		if (osc_config[o].sync_enable)
			osc_config[o].sync_phase = osc_config[o].frequency / osc_config[0].frequency;
	}
}

DWORD CALLBACK WriteStream(HSTREAM handle, float *buffer, DWORD length, void *user)
{
	// get active voices
	int index[VOICES];
	int active = 0;
	for (int v = 0; v < VOICES; v++)
	{
		if (amp_env_state[v].state != EnvelopeState::OFF)
		{
			index[active++] = v;
		}
	}

	// number of samples
	size_t count = length / (2 * sizeof(buffer[0]));

	// low-frequency oscillator value
	// (updated every BLOCK_UPDATE_SAMPLES)
	float lfo = 0;

	if (active == 0)
	{
		// clear buffer
		memset(buffer, 0, length);

		// advance low-frequency oscillator
		if (lfo_config.enable)
			lfo = lfo_state.Update(lfo_config, float(count) / info.freq);

		// compute shared oscillator values
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			osc_config[o].Modulate(lfo);
		}

		return length;
	}

	// flush denormals
	unsigned int prev;
	_controlfp_s(&prev, _DN_FLUSH, _MCW_DN);

	// time step per output sample
	float const step = 1.0f / info.freq;

	// time step per output block
	float const block_step = step * BLOCK_UPDATE_SAMPLES;

	// if the low-frequency oscillator is off...
	if (!lfo_config.enable)
	{
		ApplyLFO(0);
	}

	// for each output sample...
	for (size_t c = 0; c < count; ++c)
	{
		if ((c & (BLOCK_UPDATE_SAMPLES - 1)) == 0)
		{
			// apply low-frequency oscillator
			if (lfo_config.enable)
			{
				// get low-frequency oscillator value
				lfo = lfo_state.Update(lfo_config, block_step);

				// apply low-frequency oscillator
				ApplyLFO(lfo);
			}
		}

		// accumulated sample value
		float sample = 0.0f;

		// for each active voice...
		for (int i = 0; i < active; ++i)
		{
			// get the voice index
			int const v = index[i];

			// update volume envelope generator
			float const amp_env_amplitude = amp_env_state[v].Update(amp_env_config, step);

			// if the envelope generator finished...
			if (amp_env_state[v].state == EnvelopeState::OFF)
			{
				// remove from active oscillators
				--active;
				index[i] = index[active];
				--i;
				continue;
			}

			// key frequency (taking pitch wheel control into account)
			float const key_freq = Control::pitch_scale * note_frequency[voice_note[v]];

			// key velocity
			float const key_vel = voice_vel[v] / 64.0f;

			// key frequency times step
			float const key_step = key_freq * step;

			// update oscillators
			// (assume key follow)
			float osc_value = 0.0f;
			for (int o = 0; o < NUM_OSCILLATORS; ++o)
			{
				if (!osc_config[o].enable)
					continue;
				if (osc_config[o].sub_osc_mode && osc_config[o].sub_osc_amplitude)
					osc_value += osc_config[o].sub_osc_amplitude * SubOscillator(osc_config[o], osc_state[v][o], key_step);
				osc_value += osc_state[v][o].Update(osc_config[o], key_step);
			}

			// update filter
			if (flt_config.enable)
			{
				if ((c & (BLOCK_UPDATE_SAMPLES - 1)) == 0)
				{
					// update filter envelope generator
					float const flt_env_amplitude = flt_env_state[v].Update(flt_env_config, block_step);

					// compute cutoff frequency
					// (assume key follow)
					float const cutoff = key_freq * flt_config.GetCutoff(lfo, flt_env_amplitude, key_vel);

					// set up the filter
					flt_state[v].Setup(cutoff, flt_config.resonance, step);
				}

				// get filtered oscillator value
				osc_value = flt_state[v].Update(flt_config, osc_value);
			}

			// apply amplifier level and accumulate result
			sample += osc_value * amp_config.GetLevel(amp_env_amplitude, key_vel);
		}

		// left and right channels are the same
		//short const output = short(Clamp(int(sample * output_scale * 32768), SHRT_MIN, SHRT_MAX));
		//short const output = short(FastTanh(sample * output_scale) * 32767);
		float const output = FastTanh(sample * output_scale);
		*buffer++ = output;
		*buffer++ = output;
	}

	// restore denormal
	_controlfp_s(&prev, prev, _MCW_DN);

	return length;
}

void PrintOutputScale(HANDLE hOut)
{
	COORD const pos = { 1, 10 };
	PrintConsoleWithAttribute(hOut, { pos.X,     pos.Y }, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED,  "-");
	PrintConsoleWithAttribute(hOut, { pos.X + 2, pos.Y }, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, "+");
	PrintConsole(hOut, { pos.X + 4, pos.Y }, "Output: %5.1f%%", output_scale * 100.0f);
}

void PrintKeyOctave(HANDLE hOut)
{
	COORD const pos = { 21, 10 };
	PrintConsoleWithAttribute(hOut, { pos.X,     pos.Y }, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED,  "[");
	PrintConsoleWithAttribute(hOut, { pos.X + 2, pos.Y }, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, "]");
	PrintConsole(hOut, { pos.X + 4, pos.Y }, "Key Octave: %d", keyboard_octave);
}

void PrintAntialias(HANDLE hOut)
{
	COORD const pos = { 41, 10 };
	PrintConsole(hOut, pos, "F12 Antialias:");
	if (use_antialias)
		PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, FOREGROUND_GREEN, " ON");
	else
		PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, FOREGROUND_RED,   "OFF");
}


void __cdecl main(int argc, char **argv)
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
	stream = BASS_StreamCreate(info.freq, 2, BASS_SAMPLE_FLOAT, (STREAMPROC*)WriteStream, 0);

	// set channel to apply effects
	fx_channel = stream;

#ifdef BANDLIMITED_SAWTOOTH
	// initialize bandlimited sawtooth tables
	InitSawtooth();
#endif

	// initialize waves
	InitWave();

	// compute frequency for each note
	for (int n = 0; n < NOTES; ++n)
	{
		note_frequency[n] = powf(2.0F, (n - 69) / 12.0f) * 440.0f;		// middle C = 261.626Hz; A above middle C = 440Hz
		//note_frequency[n] = powF(2.0F, (n - 60) / 12.0f) * 256.0f;	// middle C = 256Hz
	}

	// enable the first oscillator
	osc_config[0].enable = true;

	// reset all controllers
	Control::ResetAll();

	// start playing the audio stream
	BASS_ChannelPlay(stream, FALSE);

	// get the number of midi devices
	UINT midiInDevs = Midi::Input::GetNumDevices();
	DebugPrint("MIDI input devices: %d\n", midiInDevs);

	// print device names
	for (UINT i = 0; i < midiInDevs; ++i)
	{
		MIDIINCAPS midiInCaps;
		if (Midi::Input::GetDeviceCaps(i, midiInCaps) == 0)
		{
			DebugPrint("%d: %s\n", i, midiInCaps.szPname);
		}
	}

	// if there are any devices available...
	if (midiInDevs > 0)
	{
		// open and start midi input
		// TO DO: select device number via a configuration setting
		Midi::Input::Open(0);
		Midi::Input::Start();
	}

	// initialize to middle c
	note_most_recent = 60;
	voice_note[voice_most_recent] = note_most_recent;

	// initialize key display
	InitKeyVolumeEnvelopeDisplay(hOut);

	// show output scale and key octave
	PrintOutputScale(hOut);
	PrintKeyOctave(hOut);
	PrintAntialias(hOut);

	// show main page
	Menu::SetActivePage(hOut, Menu::PAGE_MAIN);

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
						Menu::UpdatePercentageProperty(output_scale, -1, modifiers, 0, 4);
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_PLUS || code == VK_ADD)
					{
						Menu::UpdatePercentageProperty(output_scale, +1, modifiers, 0, 4);
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_4)	// '['
					{
						if (keyboard_octave > 1)
						{
							for (int k = 0; k < KEYS; ++k)
							{
								if (key_down[k])
									NoteOff(k + keyboard_octave * 12);
							}
							--keyboard_octave;
							for (int k = 0; k < KEYS; ++k)
							{
								if (key_down[k])
									NoteOn(k + keyboard_octave * 12);
							}
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_OEM_6)	// ']'
					{
						if (keyboard_octave < 9)
						{
							for (int k = 0; k < KEYS; ++k)
							{
								if (key_down[k])
									NoteOff(k + keyboard_octave * 12);
							}
							++keyboard_octave;
							for (int k = 0; k < KEYS; ++k)
							{
								if (key_down[k])
									NoteOn(k + keyboard_octave * 12);
							}
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_F12)
					{
						use_antialias = !use_antialias;
						PrintAntialias(hOut);
					}
					else if (code >= VK_F1 && code < VK_F10)
					{
						Menu::SetActiveMenu(hOut, code - VK_F1);
					}
					else if (code == VK_F10)
					{
						Menu::SetActivePage(hOut, Menu::PAGE_MAIN);
					}
					else if (code == VK_F11)
					{
						Menu::SetActivePage(hOut, Menu::PAGE_FX);
					}
					else if (code == VK_TAB)
					{
						if (modifiers & SHIFT_PRESSED)
							Menu::PrevMenu(hOut);
						else
							Menu::NextMenu(hOut);
					}
					else if (code == VK_UP || code == VK_DOWN || code == VK_RIGHT || code == VK_LEFT)
					{
						Menu::Handler(hOut, code, modifiers);
					}
				}

				// handle note keys
				for (int k = 0; k < KEYS; k++)
				{
					if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[k])
					{
						// key down
						bool down = (keyin.Event.KeyEvent.bKeyDown != 0);

						// if key down state changed...
						if (key_down[k] != down)
						{
							// update state
							key_down[k] = down;

							// if pressing the key
							if (down)
							{
								// note on
								NoteOn(k + keyboard_octave * 12);
							}
							else
							{
								// note off
								NoteOff(k + keyboard_octave * 12);
							}
						}
						break;
					}
				}
			}
		}

		// center frequency of the zeroth semitone band
		// (one octave down from the lowest key)
		float const freq_min = 0.5f * note_frequency[keyboard_octave * 12];

		// update the spectrum analyzer display
		UpdateSpectrumAnalyzer(hOut, stream, info, freq_min);

		// update note key volume envelope display
		UpdateKeyVolumeEnvelopeDisplay(hOut);

		if (Menu::active_page == Menu::PAGE_MAIN)
		{
			// update the oscillator waveform display
			UpdateOscillatorWaveformDisplay(hOut, info, voice_most_recent);

			// update the oscillator frequency displays
			for (int o = 0; o < NUM_OSCILLATORS; ++o)
			{
				if (osc_config[o].enable)
					UpdateOscillatorFrequencyDisplay(hOut, voice_most_recent, o);
			}

			// update the low-frequency oscillator dispaly
			UpdateLowFrequencyOscillatorDisplay(hOut);
		}

		// show CPU usage
		PrintConsole(hOut, { 73, 49 }, "%6.2f%%", BASS_GetCPU());

		// sleep for 1/60th of second
		Sleep(16);
	}

	if (midiInDevs)
	{
		// stop and close midi input
		Midi::Input::Stop();
		Midi::Input::Close();
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
