#include "StdAfx.h"

#include "Wave.h"
#include "WaveHold.h"
#include "PolyBLEP.h"
#include "Oscillator.h"
#include "Math.h"
#include "Random.h"

// precomputed noise wavetable
float noise[65536];

// initialize noise wavetable
void InitNoise()
{
	for (int i = 0; i < ARRAY_SIZE(noise); ++i)
	{
		noise[i] = Random::Float() * 2.0f - 1.0f;
	}
}

// shared data oscillator
static float OscillatorHold(OscillatorConfig const &config, OscillatorState &state, float data[], int cycle, float step)
{
	if (step > 0.5f * cycle)
		return 0;

	// current wavetable value
	float value = data[state.index];
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float w = Min(step * POLYBLEP_WIDTH, 8.0f);

		int const back = FloorInt(state.phase - w);
		int const ahead = FloorInt(state.phase + w);
		int const count = ahead - back;
		if (count > 0)
		{
			int i = state.index + back + cycle;
			if (i >= cycle)
				i -= cycle;
			float t = state.phase - back;
			float v0 = data[i];
			for (int c = 0; c < count; ++c)
			{
				if (++i >= cycle)
					i -= cycle;
				t -= 1.0f;
				float v1 = data[i];
				if (v0 != v1)
					value += PolyBLEP(t, w, v1 - v0);
				v0 = v1;
			}
		}
	}
#endif
	return value;
}

// shared data oscillator
static float OscillatorLerp(OscillatorConfig const &config, OscillatorState &state, float data[], int cycle, float step)
{
	if (step > 0.5f * cycle)
		return 0;

	// current and next wavetable value
	int const index0 = state.index;
	float const value0 = data[index0];
	int const index1 = state.index < cycle ? state.index + 1 : 0;
	float const value1 = data[index1];
	float value = value0 + (value1 - value0) * state.phase;

#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float w = Min(step * INTEGRATED_POLYBLEP_WIDTH, 8.0f);

		int const back = FloorInt(state.phase - w);
		int const ahead = FloorInt(state.phase + w);
		int const count = ahead - back;
		if (count > 0)
		{
			int i = index0 + back + cycle;
			if (i >= cycle)
				i -= cycle;
			float const vn1 = data[i];
			if (++i >= cycle)
				i -= cycle;
			float t = state.phase - back;
			float v0 = data[i];
			float s0 = v0 - vn1;
			for (int c = 0; c < count; ++c)
			{
				if (++i >= cycle)
					i -= cycle;
				t -= 1.0f;
				float const v1 = data[i];
				float const s1 = v1 - v0;
				if (s0 != s1)
					value += IntegratedPolyBLEP(t, w, s1 - s0);
				v0 = v1;
				s0 = s1;
			}
		}
	}
#endif
	return value;
}

// shared data oscillator
static float OscillatorCubic(OscillatorConfig const &config, OscillatorState &state, float data[], int cycle, float step)
{
	if (step > 0.5f * cycle)
		return 0;

	// four consecutive wavetable values
	register float const value0 = data[state.index];
	register float const value1 = data[(state.index + 1) & (cycle - 1)];
	register float const value2 = data[(state.index + 2) & (cycle - 1)];
	register float const value3 = data[(state.index + 3) & (cycle - 1)];

	// cubic interpolation
	// (Catmull-Rom spline)
	register float value = 
		value1 + 
		state.phase * 0.5 * (value2 - value0 +
		state.phase * (2.0 * value0 - 5.0 * value1 + 4.0 * value2 - value3 + 
		state.phase * (3.0 * (value1 - value2) + value3 - value0)));

	return value;
}

// sample-and-hold noise
float OscillatorNoiseHold(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorHold(config, state, noise, ARRAY_SIZE(noise), step);
}

// linear interpolated noise
float OscillatorNoiseLinear(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorLerp(config, state, noise, ARRAY_SIZE(noise), step);
}

// cubic interpolated noise
float OscillatorNoiseCubic(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorCubic(config, state, noise, ARRAY_SIZE(noise), step);
}
