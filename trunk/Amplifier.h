#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Amplifier
*/

#include "Envelope.h"

class AmplifierConfig
{
public:
	float level_env;
	float level_env_vel;

	AmplifierConfig(float const level_env, float const level_env_vel)
		: level_env(level_env)
		, level_env_vel(level_env_vel)
	{
	}

	// get the modulated level value
	float GetLevel(float const env, float const vel)
	{
		return env * (level_env + vel * level_env_vel);
	}
};

// amplifier configuration
extern AmplifierConfig amp_config;

// amplifier envelope configuration
extern EnvelopeConfig amp_env_config;

// amplifier envelope state
extern EnvelopeState amp_env_state[];
