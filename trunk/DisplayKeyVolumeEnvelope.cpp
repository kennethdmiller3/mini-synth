/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Per-Key Volume Envelope Display
*/
#include "StdAfx.h"

#include "DisplayKeyVolumeEnvelope.h"
#include "DisplaySpectrumAnalyzer.h"
#include "Keys.h"

static COORD const key_pos = { 12, SPECTRUM_HEIGHT };

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
	for (int k = 0; k < KEYS; ++k)
	{
		char buf[] = { char(keys[k]), '\0' };
		COORD pos = { SHORT(key_pos.X + k), SHORT(key_pos.Y) };
		WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		DWORD written;
		WriteConsoleOutputAttribute(hOut, &attrib, 1, pos, &written);
		WriteConsoleOutputCharacter(hOut, buf, 1, pos, &written);
	}
}

void UpdateKeyVolumeEnvelopeDisplay(HANDLE hOut, EnvelopeState::State vol_env_display[])
{
	for (int k = 0; k < KEYS; k++)
	{
		if (vol_env_display[k] != vol_env_state[k].state)
		{
			vol_env_display[k] = vol_env_state[k].state;
			COORD pos = { SHORT(key_pos.X + k), key_pos.Y };
			DWORD written;
			WriteConsoleOutputAttribute(hOut, &env_attrib[vol_env_display[k]], 1, pos, &written);
		}
	}
}
