#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Oscillator
*/

#include "Wave.h"

// base frequency oscillator configuration
class OscillatorConfig
{
public:
	bool enable;

	Wave wavetype;
	float waveparam;
	float frequency;
	float amplitude;

	// hard sync
	bool sync_enable;
	float sync_phase;

	// derived values
	WaveEvaluate evaluate;
	size_t cycle;
	float adjust;

	OscillatorConfig(bool const enable, Wave const wavetype, float const waveparam, float const frequency, float const amplitude)
		: enable(enable)
		, wavetype(wavetype)
		, waveparam(waveparam)
		, frequency(frequency)
		, amplitude(amplitude)
		, sync_enable(false)
		, sync_phase(1.0f)
	{
		SetWaveType(wavetype);
	}

	void SetWaveType(Wave type)
	{
		wavetype = type;
		evaluate = wave_evaluate[wavetype];
		cycle = wave_loop_cycle[wavetype];
		adjust = wave_adjust_frequency[wavetype];
	}
};

// oscillator state
class OscillatorState
{
public:
	float phase;
	int index;

	OscillatorState()
	{
		Reset();
	}

	// reset the oscillator
	void Reset();

	// start the oscillator
	void Start();

	// update the oscillator by one step
	float Update(OscillatorConfig const &config, float frequency, float const step);

	// compute the oscillator value
	float Compute(OscillatorConfig const &config, float delta);

	// advance the oscillator phase
	void Advance(OscillatorConfig const &config, float delta);
};
