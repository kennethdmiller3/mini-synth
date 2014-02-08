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
	return 1 - phase - phase;
}
float OscillatorSawtooth(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float phase = state.phase;
	float value = GetSawtoothValue(phase);

#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 0.5f);

		// up edge nearest the current phase
		float up_nearest = float(phase >= 0.5f);

		if (config.sync_enable)
		{
			int const index = state.index;
			phase += index;
			up_nearest += index;
			float const sync_fraction = config.sync_phase - float(FloorInt(config.sync_phase));

			// handle last integer phase before sync from the previous wave cycle
			float const up_before_zero = -sync_fraction;
			value += PolyBLEP(phase - up_before_zero, w);

			// handle discontinuity at integer phases
			if (up_nearest > 0 && up_nearest <= config.sync_phase)
				value += PolyBLEP(phase - up_nearest, w);

			// handle discontituity at sync phase
			float const sync_value = GetSawtoothValue(sync_fraction);
			if (sync_value < 0.999969482421875f)
			{
				value += PolyBLEP(phase, w, 1 - sync_value);
				value += PolyBLEP(phase - config.sync_phase, w, 1 - sync_value);
			}
		}
		else
		{
			value += PolyBLEP(phase - up_nearest, w);
		}
	}
#endif
	return value;
}
