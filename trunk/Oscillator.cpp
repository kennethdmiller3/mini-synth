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
	memset(f, 0, sizeof(f));
}

// start oscillator
void OscillatorState::Start()
{
//	Reset();
	phase = 0.0f;
}

// update oscillator
float OscillatorState::Update(OscillatorConfig const &config, float const step)
{
	float const delta = config.frequency * config.adjust * step;

	// compute oscillator value
	float const value = Compute(config, delta);

	// advance oscillator phase
	Advance(config, delta);

	return value;
}

// compute the oscillator value
float OscillatorState::Compute(OscillatorConfig const &config, float delta)
{
	return config.amplitude * config.evaluate(config, *this, delta);
}

// advance the oscillator phase
void OscillatorState::Advance(OscillatorConfig const &config, float delta)
{
	phase += delta;
#if 1
	int const advance = FloorInt(phase);
	if (advance)
	{
		// wrap phase around
		phase -= advance;

		// advance the wavetable index
		index += advance;
		if (index >= int(config.cycle))
			index -= int(config.cycle);
		else if (index < 0)
			index += int(config.cycle);
	}
	if (config.sync_enable)
	{
		if (phase >= config.sync_phase - index)
		{
			phase -= config.sync_phase - index;
			index = 0;
		}
	}
#else
	if (phase >= config.sync_phase)
	{
		// wrap phase around
		int const advance = FloorInt(phase / config.sync_phase);
		phase -= advance * config.sync_phase;

		// advance the wavetable index
		int const cycle = config.cycle;
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
		int const cycle = config.cycle;
		index += advance;
		if (index < 0)
			index += cycle;
	}
#endif
}
