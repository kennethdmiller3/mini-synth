/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Triangle Wave
*/
#include "StdAfx.h"

#include "Wave.h"
#include "Oscillator.h"
#include "PolyBLEP.h"
#include "Math.h"

// triangle oscillator
// - 8/pi**2 sum k=0..infinity (-1)**k sin((2*k+1)*2*pi*phase)/(2*k+1)**2
static __forceinline float GetTriangleValue(float const phase)
{
	return fabsf(4 * (phase - FloorInt(phase - 0.25f)) - 3) - 1;
}
float OscillatorTriangle(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = GetTriangleValue(state.phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * INTEGRATED_POLYBLEP_WIDTH, 0.5f);

		// nearest /\ slope transtiion
		float const down_nearest = float(FloorInt(state.phase + 0.25f)) + 0.25f;

		// nearest \/ slope transition
		float const up_nearest = float(FloorInt(state.phase - 0.25f)) + 0.75f;

		if (config.sync_enable)
		{
			// handle discontinuity at 0
			if (state.phase < w)
			{
				// sync during downward slope creates a \/ transition
				float const sync_fraction = config.sync_phase - float(FloorInt(config.sync_phase));
				if (sync_fraction > 0.25f && sync_fraction < 0.75)
					value += IntegratedPolyBLEP(state.phase, w);

				// wave value discontinuity creates a | transition 
				float const sync_value = GetTriangleValue(config.sync_phase);
				value -= PolyBLEP(state.phase, w, sync_value);
			}

			// handle /\ and \/ slope discontinuities
			if (down_nearest > 0 && down_nearest < config.sync_phase)
				value -= IntegratedPolyBLEP(state.phase - down_nearest, w);
			if (up_nearest > 0 && up_nearest < config.sync_phase)
				value += IntegratedPolyBLEP(state.phase - up_nearest, w);

			// handle discontinuity at sync phase
			if (state.phase - config.sync_phase < w)
			{
				// sync during downward slope creates a \/ transition
				float const sync_fraction = config.sync_phase - float(FloorInt(config.sync_phase));
				if (sync_fraction > 0.25f && sync_fraction < 0.75)
					value += IntegratedPolyBLEP(state.phase - config.sync_phase, w);

				// wave value discontinuity creates a | transition 
				float const sync_value = GetTriangleValue(config.sync_phase);
				value += PolyBLEP(state.phase - config.sync_phase, w, sync_value);
			}
		}
		else
		{
			// handle /\ and \/ slope discontinuities
			value -= IntegratedPolyBLEP(state.phase - down_nearest, w);
			value += IntegratedPolyBLEP(state.phase - up_nearest, w);
		}
	}
#endif
	return value;
}
