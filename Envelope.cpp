/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Envelope Generator
*/
#include "StdAfx.h"

#include "Envelope.h"
#include "Math.h"
#include "Voice.h"

// envelope bias values to make the exponential decay arrive in finite time
// the attack part is set up to be more linear (and shorter) than the decay/release part
static const float ENV_ATTACK_CONSTANT = 1.0f;
static const float ENV_DECAY_CONSTANT = 3.0f;
static float const ENV_ATTACK_BIAS = 1.0f / (1.0f - expf(-ENV_ATTACK_CONSTANT)) - 1.0f;
static float const ENV_DECAY_BIAS = 1.0f - 1.0f / (1.0f - expf(-ENV_DECAY_CONSTANT));

// envelope config constructor
EnvelopeConfig::EnvelopeConfig(bool const enable, float const attack_time, float const decay_time, float const sustain_level, float const release_time)
: enable(enable)
, attack_time(attack_time)
, attack_rate(1 / (attack_time + FLT_MIN))
, decay_time(decay_time)
, decay_rate(1 / (decay_time + FLT_MIN))
, sustain_level(sustain_level)
, release_time(release_time)
, release_rate(1 / (release_time + FLT_MIN))
{
}

// envelope state constructor
EnvelopeState::EnvelopeState()
: gate(false)
, state(OFF)
, amplitude(0.0f)
{
}

// gate envelope generator
void EnvelopeState::Gate(EnvelopeConfig const &config, bool on)
{
	if (gate == on)
		return;

	gate = on;
	if (gate)
	{
		if (config.enable && config.attack_time)
			state = EnvelopeState::ATTACK;
		else if (config.enable && config.decay_time)
			state = EnvelopeState::DECAY, amplitude = 1;
		else
			state = EnvelopeState::SUSTAIN, amplitude = config.sustain_level;
	}
	else
	{
		if (config.enable && config.release_time)
			state = EnvelopeState::RELEASE;
		else
			state = EnvelopeState::OFF;
	}
}

// update envelope generator
float EnvelopeState::Update(EnvelopeConfig const &config, float const step)
{
	if (!config.enable)
		return gate;
	float env_target;
	switch (state)
	{
	case ATTACK:
		env_target = 1.0f + ENV_ATTACK_BIAS;
		amplitude += (env_target - amplitude) * config.attack_rate * step;
		if (amplitude >= 1.0f)
		{
			amplitude = 1.0f;
			if (config.sustain_level < 1.0f)
				state = DECAY;
			else
				state = SUSTAIN;
		}
		break;

	case DECAY:
		env_target = config.sustain_level + (1.0f - config.sustain_level) * ENV_DECAY_BIAS;
		amplitude += (env_target - amplitude) * config.decay_rate * step;
		if (amplitude <= config.sustain_level)
		{
			amplitude = config.sustain_level;
			state = SUSTAIN;
		}
		break;

	case RELEASE:
		env_target = ENV_DECAY_BIAS;
		if (amplitude < config.sustain_level || config.decay_rate < config.release_rate)
			amplitude += (env_target - amplitude) * config.release_rate * step;
		else
			amplitude += (env_target - amplitude) * config.decay_rate * step;
		if (amplitude <= 0.0f)
		{
			amplitude = 0.0f;
			state = OFF;
		}
		break;
	}

	return amplitude;
}
