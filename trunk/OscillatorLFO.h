#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator
*/

#include "Oscillator.h"
#include "Wave.h"

// low-frequency oscillator configuration
class LFOOscillatorConfig : public OscillatorConfig
{
public:
	float frequency_base;	// logarithmic offset from 1 Hz

	explicit LFOOscillatorConfig(bool const enable = false, Wave const wavetype = WAVE_SINE, float const waveparam = 0.5f, float const frequency = 1.0f, float const amplitude = 1.0f)
		: OscillatorConfig(enable, wavetype, waveparam, frequency, amplitude)
		, frequency_base(0.0f)
	{
	}
};
extern LFOOscillatorConfig lfo_config;
// TO DO: multiple LFOs?
// TO DO: LFO key sync
// TO DO: LFO temp sync?
// TO DO: LFO keyboard follow dial?

// low-frequency oscillator state
extern OscillatorState lfo_state;
