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
bool fx_active[9];

// effect handles
HFX fx[9];

// channel to apply effects
DWORD fx_channel;

// effect parameters
BASS_DX8_CHORUS fx_chorus = { 50, 10, 25, 1, 1, 16, 3 };	// 1.1f
BASS_DX8_COMPRESSOR fx_compressor = { 0, 10, 200, -20, 3, 4 };
BASS_DX8_DISTORTION fx_distortion = { -18, 15, 2400, 2400, 8000 };
BASS_DX8_ECHO fx_echo = { 50, 50, 500, 500, 0 };
BASS_DX8_FLANGER fx_flanger = { 50, 100, -50, 0.25f, 1, 2, 2 };
BASS_DX8_GARGLE fx_gargle = { 20, 0 };
BASS_DX8_I3DL2REVERB fx_reverb3d = { -1000, -100, 0, 1.49f, 0.83f, -2602, 0.007f, 200, 0.011f, 100, 100, 5000 };
BASS_DX8_PARAMEQ fx_parameq = { 8000, 12, 0 };	// should be an array of these
BASS_DX8_REVERB fx_reverb = { 0, 0, 1000, 0.001f };

// map index to parameters
void *fx_params[] =
{
	&fx_chorus,
	&fx_compressor,
	&fx_distortion,
	&fx_echo,
	&fx_flanger,
	&fx_gargle,
	&fx_reverb3d,
	&fx_parameq,
	&fx_reverb
};

// enable/disable effect
void EnableEffect(int index, bool enable)
{
	if (enable)
	{
		if (!fx[index])
		{
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

// update effect
void UpdateEffect(int index)
{
	if (fx[index])
	{
		BASS_FXSetParameters(fx[index], fx_params[index]);
	}
}
