/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Amplifier
*/
#include "StdAfx.h"

#include "Amplifier.h"
#include "Voice.h"

// amplifier configuration
AmplifierConfig amp_config(0.0f, 1.0f);

// amplifier envelope configuration
EnvelopeConfig amp_env_config(false, 0.0f, 1.0f, 1.0f, 0.1f);

// amplifier envelope state
EnvelopeState amp_env_state[VOICES];
