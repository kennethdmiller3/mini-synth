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
	float value = GetSineValue(state.phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias && config.sync_enable)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);

		// handle discontinuity at zero phase
		if (state.phase < w)
		{
			value -= PolyBLEP(state.phase, w, GetSineValue(config.sync_phase) - SINE_VALUE_AT_0);
			value -= IntegratedPolyBLEP(state.phase, w, GetSineSlope(config.sync_phase) - SINE_SLOPE_AT_0);
		}

		// handle discontinuity at sync phase
		if (state.phase - config.sync_phase > -w)
		{
			value -= PolyBLEP(state.phase - config.sync_phase, w, GetSineValue(config.sync_phase) - SINE_VALUE_AT_0);
			value -= IntegratedPolyBLEP(state.phase - config.sync_phase, w, GetSineSlope(config.sync_phase) - SINE_SLOPE_AT_0);
		}
	}
#endif
	return value;
}

