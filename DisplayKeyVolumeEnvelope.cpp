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
	WORD note_env_attrib[SPECTRUM_WIDTH];
	WORD voice_env_attrib[VOICES];

	memset(note_env_attrib, env_attrib[EnvelopeState::OFF], sizeof(note_env_attrib));
	for (int v = 0; v < VOICES; ++v)
	{
		EnvelopeState::State const state = amp_env_state[v].state;
		WORD const attrib = env_attrib[state];
		int const x = key_pos.X - keyboard_octave * 12 + voice_note[v];
		if (x >= 0 && x < SPECTRUM_WIDTH)
			note_env_attrib[x] = attrib;
		voice_env_attrib[v] = attrib;
	}

	DWORD written;
	WriteConsoleOutputAttribute(hOut, note_env_attrib, SPECTRUM_WIDTH, { 0, key_pos.Y }, &written);
	WriteConsoleOutputAttribute(hOut, voice_env_attrib, VOICES, voice_pos, &written);
}
