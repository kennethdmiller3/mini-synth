/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Oscillator
*/
#include "StdAfx.h"

#include "Oscillator.h"
#include "Wave.h"
#include "Math.h"

// reset oscillator
void OscillatorState::Reset()
{
	phase = 0.0f;
	index = 0;
}

// start oscillator
void OscillatorState::Start()
{
//	Reset();
}

// update oscillator
float OscillatorState::Update(OscillatorConfig const &config, float const frequency, float const step)
{
	if (!config.enable)
		return 0.0f;

	float const delta = config.frequency * frequency * step;

	// compute oscillator value
	float const value = Compute(config, delta);

	// advance oscillator phase
	Advance(config, delta);

	return value;
}

// compute the oscillator value
float OscillatorState::Compute(OscillatorConfig const &config, float delta)
{
	return config.amplitude * oscillator[config.wavetype](config, *this, delta);
}

// advance the oscillator phase
void OscillatorState::Advance(OscillatorConfig const &config, float delta)
{
	phase += delta;
	if (phase >= config.sync_phase)
	{
		// wrap phase around
		int const advance = FloorInt(phase / config.sync_phase);
		phase -= advance * config.sync_phase;

		// advance the wavetable index
		int const cycle = wave_loop_cycle[config.wavetype];
		index += advance;
		if (index >= cycle)
			index -= cycle;
	}
	else if (phase < 0.0f)
	{
		// wrap phase around
		int const advance = FloorInt(phase / config.sync_phase);
		phase -= advance * config.sync_phase;

		// rewind the wavetable index
		int const cycle = wave_loop_cycle[config.wavetype];
		index += advance;
		if (index < 0)
			index += cycle;
	}
}
