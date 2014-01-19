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

// sample-and-hold noise
extern float OscillatorNoiseHold(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorHold(config, state, noise, ARRAY_SIZE(noise), step);
}
