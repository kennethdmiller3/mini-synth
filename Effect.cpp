/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

DirectSound Effects
*/
#include "StdAfx.h"

#include "Effect.h"

char const * const fx_name[9] =
{
	"Chorus", "Compressor", "Distortion", "Echo", "Flanger", "Gargle", "I3DL2Reverb", "ParamEQ", "Reverb"
};

// effect config
bool fx_enable;
bool fx_active[9];

// effect handles
HFX fx[9];

// channel to apply effects
DWORD fx_channel;

// enable/disable effect
void EnableEffect(int index, bool enable)
{
	if (enable)
	{
		if (!fx[index])
		{
			// set the effect, not bothering with parameters (use defaults)
			fx[index] = BASS_ChannelSetFX(fx_channel, BASS_FX_DX8_CHORUS + index, 0);
		}
	}
	else
	{
		if (fx[index])
		{
			BASS_ChannelRemoveFX(fx_channel, fx[index]);
			fx[index] = 0;
		}
	}
}
