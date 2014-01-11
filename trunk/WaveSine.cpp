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

// sine wave
static __forceinline float GetSineValue(float const phase)
{
	return sinf(M_PI * 2.0f * phase);
}
static __forceinline float GetSineSlope(float const phase)
{
	return cosf(M_PI * 2.0f * phase);
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
			value -= (GetSineValue(config.sync_phase)) * 0.5f * PolyBLEP(state.phase, w);
			value -= (GetSineSlope(config.sync_phase) - 1.0f) * 0.5f * IntegratedPolyBLEP(state.phase, w);
		}

		// handle discontinuity at sync phase
		if (state.phase - config.sync_phase > -w)
		{
			value -= (GetSineValue(config.sync_phase)) * 0.5f * PolyBLEP(state.phase - config.sync_phase, w);
			value -= (GetSineSlope(config.sync_phase) - 1.0f) * 0.5f * IntegratedPolyBLEP(state.phase - config.sync_phase, w);
		}
	}
#endif
	return value;
}

