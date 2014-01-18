/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Per-Key Volume Envelope Display
*/
#include "StdAfx.h"

#include "DisplayKeyVolumeEnvelope.h"
#include "DisplaySpectrumAnalyzer.h"
#include "Math.h"
#include "Console.h"
#include "Keys.h"
#include "Voice.h"
#include "Amplifier.h"

static COORD const key_pos = { 12, SPECTRUM_HEIGHT };
static COORD const voice_pos = { 73 - VOICES, 49 };

// attribute associated with each envelope state
static WORD const env_attrib[EnvelopeState::COUNT] =
{
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,	// EnvelopeState::OFF
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,				// EnvelopeState::ATTACK,
	BACKGROUND_RED | BACKGROUND_INTENSITY,					// EnvelopeState::DECAY,
	BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,	// EnvelopeState::SUSTAIN,
	BACKGROUND_INTENSITY,									// EnvelopeState::RELEASE,
};

void InitKeyVolumeEnvelopeDisplay(HANDLE hOut)
{
	// show the note keys
	DWORD written;
	WriteConsoleOutputCharacterW(hOut, LPCWSTR(keys), KEYS, key_pos, &written);
	FillConsoleOutputAttribute(hOut, env_attrib[EnvelopeState::OFF], KEYS, key_pos, &written);

	// voice indicators
	CHAR voice[VOICES];
	memset(voice, 7, VOICES);
	WriteConsoleOutputCharacter(hOut, voice, VOICES, voice_pos, &written);
}


void UpdateKeyVolumeEnvelopeDisplay(HANDLE hOut)
{
	WORD note_env_attrib[NOTES];
	memset(note_env_attrib, env_attrib[0], sizeof(note_env_attrib));
	WORD voice_env_attrib[VOICES];
	memset(voice_env_attrib, env_attrib[0], sizeof(voice_env_attrib));
	for (int v = 0; v < VOICES; ++v)
	{
		EnvelopeState::State state = amp_env_state[v].state;
		if (state != EnvelopeState::OFF)
			note_env_attrib[voice_note[v]] = voice_env_attrib[v] = env_attrib[state];
	}

	DWORD written;
	int x = key_pos.X - keyboard_octave * 12 - 12;
	int x0 = Max(x, 0);
	int x1 = Min(x + NOTES, 80);
	WriteConsoleOutputAttribute(hOut, note_env_attrib - x, x1 - x0, { 0, key_pos.Y }, &written);
	WriteConsoleOutputAttribute(hOut, voice_env_attrib, VOICES, voice_pos, &written);
}
