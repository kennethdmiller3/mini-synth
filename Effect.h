#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

DirectSound Effects
*/

extern char const * const fx_name[9];

// effect config
extern bool fx_enable;
extern bool fx_active[9];

// effect handles
extern HFX fx[9];

// channel to apply effects
extern DWORD fx_channel;

// effect parameters
extern BASS_DX8_CHORUS fx_chorus;
extern BASS_DX8_COMPRESSOR fx_compressor;
extern BASS_DX8_DISTORTION fx_distortion;
extern BASS_DX8_ECHO fx_echo;
extern BASS_DX8_FLANGER fx_flanger;
extern BASS_DX8_GARGLE fx_gargle;
extern BASS_DX8_I3DL2REVERB fx_reverb3d;
extern BASS_DX8_PARAMEQ fx_parameq;
extern BASS_DX8_REVERB fx_reverb;

// enable/disable effect
extern void EnableEffect(int index, bool enable);

// update effect (after changing parameters)
extern void UpdateEffect(int index);
