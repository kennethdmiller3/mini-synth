/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sine Wave
*/
#include "StdAfx.h"

#include "Wave.h"
#include "Oscillator.h"
#include "PolyBLEP.h"
#include "Math.h"

static float const SINE_VALUE_AT_0 = 0.0f;
static float const SINE_SLOPE_AT_0 = M_PI * 2;

// sine wave
static __forceinline float GetSineValue(float const phase)
{
	return sinf(M_PI * 2 * phase);
}
static __forceinline float GetSineSlope(float const phase)
{
	return 2 * M_PI * cosf(M_PI * 2 * phase);
}
float OscillatorSine(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float phase = state.phase;
	float value = GetSineValue(phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias && config.sync_enable)
	{
		int const index = state.index;
		phase += index;

		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);

		// handle discontinuity at zero phase
		if (phase < w)
		{
			value -= PolyBLEP(phase, w, GetSineValue(config.sync_phase) - SINE_VALUE_AT_0);
			value -= IntegratedPolyBLEP(phase, w, GetSineSlope(config.sync_phase) - SINE_SLOPE_AT_0);
		}

		// handle discontinuity at sync phase
		if (phase - config.sync_phase > -w)
		{
			value -= PolyBLEP(phase - config.sync_phase, w, GetSineValue(config.sync_phase) - SINE_VALUE_AT_0);
			value -= IntegratedPolyBLEP(phase - config.sync_phase, w, GetSineSlope(config.sync_phase) - SINE_SLOPE_AT_0);
		}
	}
#endif
	return value;
}

