/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Pulse Wave
*/
#include "StdAfx.h"

#include "Wave.h"
#include "WavePulse.h"
#include "Oscillator.h"
#include "PolyBLEP.h"
#include "Math.h"

// pulse waveform
// - param controls pulse width
// - pulse width 0.5 is square wave
// - 4/pi sum k=0..infinity sin((2*k+1)*2*pi*phase)/(2*k+1)
// - smoothed transition to reduce aliasing
static __forceinline float GetPulseValue(float const phase, float const width)
{
	return phase - FloorInt(phase) < width ? 1.0f : -1.0f;
}
float OscillatorPulse(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	if (config.waveparam <= 0.0f)
		return -1.0f;
	if (config.waveparam >= 1.0f)
		return 1.0f;
	float value = GetPulseValue(state.phase, config.waveparam);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);

		// up edges before and after the current phase
		float const up_before = float(FloorInt(state.phase));
		float const up_after = up_before + 1;

		// down edges before and after the current phase
		float const down_before = float(FloorInt(state.phase - config.waveparam)) + config.waveparam;
		float const down_after = down_before + 1;

		if (config.sync_enable)
		{
			// short-circuit case
			if (config.sync_phase < config.waveparam)
				return 1.0f;

			// last up transition before sync
			float const up_before_sync = float(CeilingInt(config.sync_phase) - 1);

			// handle discontinuity at zero phase
			if (config.sync_phase > up_before_sync + config.waveparam)
			{
				// down edge before sync wrapped before zero
				value -= PolyBLEP(state.phase - (up_before_sync + config.waveparam - config.sync_phase), w);
				// up edge at 0
				value += PolyBLEP(state.phase, w);
			}
			else
			{
				// up edge before sync wrapped before zero
				value += PolyBLEP(state.phase - (up_before_sync - config.sync_phase), w);
			}

			// handle up transitions if they're in range
			if (up_before > 0)
				value += PolyBLEP(state.phase - up_before, w);
			if (up_after <= up_before_sync)
				value += PolyBLEP(state.phase - up_after, w);

			// handle down transitions if they're in range
			if (down_before > 0)
				value -= PolyBLEP(state.phase - down_before, w);
			if (down_after < config.sync_phase)
				value -= PolyBLEP(state.phase - down_after, w);

			// handle discontinuity at sync phase
			if (config.sync_phase > up_before_sync + config.waveparam)
			{
				// up edge at config.sync_phase
				value += PolyBLEP(state.phase - config.sync_phase, w);
			}
		}
		else
		{
			value += PolyBLEP(state.phase - up_before, w);
			value += PolyBLEP(state.phase - up_after, w);
			value -= PolyBLEP(state.phase - down_before, w);
			value -= PolyBLEP(state.phase - down_after, w);
		}
	}
#endif
	return value;
}
