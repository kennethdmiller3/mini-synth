/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

White Noise
*/
#include "StdAfx.h"

#include "Random.h"
#include "WaveNoise.h"

// white noise wave
float OscillatorNoise(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return Random::Float() * 2.0f - 1.0f;
}
