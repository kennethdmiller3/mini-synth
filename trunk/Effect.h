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

// enable/disable effect
extern void EnableEffect(int index, bool enable);
