/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sawtooth Wave
*/
#include "StdAfx.h"

#include "Wave.h"
#include "Oscillator.h"
#include "PolyBLEP.h"
#include "Math.h"

// sawtooth wave
// - 2/pi sum k=1..infinity sin(k*2*pi*phase)/n
// - smoothed transition to reduce aliasing
static __forceinline float GetSawtoothValue(float const phase)
{
	return 1.0f - 2.0f * (phase - FloorInt(phase));
}
float OscillatorSawtooth(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = GetSawtoothValue(state.phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);

		// up edges before and after the current phase
		float const up_before = float(FloorInt(state.phase));
		float const up_after = up_before + 1;

		if (config.sync_enable)
		{
			// handle last integer phase before sync from the previous wave cycle
			float up_before_zero = float(FloorInt(config.sync_phase)) - config.sync_phase;
			value += PolyBLEP(state.phase - up_before_zero, w);

			// handle dscontinuity at integer phases
			if (up_before > 0)
				value += PolyBLEP(state.phase - up_before, w);
			if (up_after <= config.sync_phase)
				value += PolyBLEP(state.phase - up_after, w);

			// handle discontituity at sync phase
			float const sync_value = GetSawtoothValue(config.sync_phase);
			if (sync_value < 0.999969482421875f)
			{
				value += PolyBLEP(state.phase, w, 1 - sync_value);
				value += PolyBLEP(state.phase - config.sync_phase, w, 1 - sync_value);
			}
		}
		else
		{
			value += PolyBLEP(state.phase - up_before, w);
			value += PolyBLEP(state.phase - up_after, w);
		}
	}
#endif
	return value;
}
