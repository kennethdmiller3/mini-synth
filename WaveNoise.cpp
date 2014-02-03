/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

White Noise
*/
#include "StdAfx.h"

#include "Random.h"
#include "WaveNoise.h"
#include "Oscillator.h"
#include "Math.h"

// http://www.firstpr.com.au/dsp/pink-noise/
//b0 = 0.99765f * b0 + white * 0.0990460f;	// x42.147234f
//b1 = 0.96300f * b1 + white * 0.2965164f;	// x8.013957f
//b2 = 0.57000f * b2 + white * 1.0526913f;	// x2.448119f
//float const pink = 0.25f * (b0 + b1 + b2 + white * 0.1848f);

#define FILTERS 3

// low-pass filter bank for colored noise
#if FILTERS == 3
static const float a[3] =
{
	1 - 0.997907817f,	// 1 - expf(-2 * M_PI * 16 / 48000);
	1 - 0.967044930f,	// 1 - expf(-2 * M_PI * 256 / 48000);
	1 - 0.584987297f,	// 1 - expf(-2 * M_PI * 4096 / 48000);
};
static const float falloff = 0.25;	// sqrtf(1.f / 16.0f)
static float const w0 = 1 - falloff;
static float const w1 = w0 * falloff;
static float const w2 = w1 * falloff;
static const float w3 = falloff - w1 - w2;
#elif FILTERS == 4
static const float a[4] =
{
	1 - 0.997907817f,	// 1 - expf(-2 * M_PI * 16 / 48000);
	1 - 0.983384430f,	// 1 - expf(-2 * M_PI * 128 / 48000);
	1 - 0.874553978f,	// 1 - expf(-2 * M_PI * 1024 / 48000);
	1 - 0.342210114f,	// 1 - expf(-2 * M_PI * 8192 / 48000);
};
static const float falloff = 0.353553385f;	// sqrtf(1.f / 8.f);
static float const w0 = 1 - falloff;
static float const w1 = w0 * falloff;
static float const w2 = w1 * falloff;
static float const w3 = w2 * falloff;
static float const w4 = falloff - w1 - w2 - w3;
#elif FILTERS == 5
static const float a[5] =
{
	1 - 0.997907817f,	// 1 - expf(-2 * M_PI * 16 / 48000);
	1 - 0.991657414f,	// 1 - expf(-2 * M_PI * 64 / 48000);
	1 - 0.967044930f,	// 1 - expf(-2 * M_PI * 256 / 48000);
	1 - 0.874553978f,	// 1 - expf(-2 * M_PI * 1024 / 48000);
	1 - 0.584987297f,	// 1 - expf(-2 * M_PI * 4096 / 48000);
};
static const float falloff = 0.25f;	//sqrtf(1.f / 4.f);
static float const w0 = 1 - falloff;
static float const w1 = w0 * falloff;
static float const w2 = w1 * falloff;
static float const w3 = w2 * falloff;
static float const w4 = w3 * falloff;
static float const w5 = falloff - w1 - w2 - w3 - w4;
#endif

// noise wave
float OscillatorNoise(OscillatorConfig const &config, OscillatorState &state, float step)
{
#if 1
	// whte noise
	float white = Random::Float() * 2.0f - 1.0f;

	// if generating pure white noise, return that
	if (config.waveparam == 0.5f)
		return white;

	float param = config.waveparam;

	// low-pass filter bank
	for (int i = 0; i < FILTERS; ++i)
		state.f[i] += (white - state.f[i]) * a[i];


	// desired spectral tilt:
	// param=0.00 red -6dB/oct
	// param=0.25 pink -3dB/oct, 
	// param=0.50 white 0dB/oct
	// param=0.75 blue +3dB/oct
	// param=1.00 violet +6dB/oct

	// if generating red/pink...
	if (config.waveparam < 0.5f)
	{
		// -3dB/oct pink noise
#if FILTERS == 3
		float const pink = 8 * (w0 * state.f[0] + w1 * state.f[1] + w2 * state.f[2] + w3 * white);
#elif FILTERS == 4
		float const pink = 8 * (w0 * state.f[0] + w1 * state.f[1] + w2 * state.f[2] + w3 * state.f[3] + w4 * white);
#elif FILTERS == 5
		float const pink = 8 * (w0 * state.f[0] + w1 * state.f[1] + w2 * state.f[2] + w3 * state.f[3] + w2 * state.f[4] + w5 * white);
#endif

		// if generating pure pink noise, return that
		if (param == 0.25f)
			return pink;

		if (param < 0.25)
		{
			// -6dB/oct red noise
			float const red = 16 * state.f[0];

			// blend between red and pink
			float const s = param * 4;
			return red + (pink - red) * s;
		}
		else
		{
			// bend between white and pink
			float const s = param * 4 - 1;
			return pink + (white - pink) * s;
		}
	}
	else
	{
		// +3dB/oct blue noise
#if FILTERS == 3
		float const blue = 2 * (white - w2 * state.f[0] - w1 * state.f[1] - w0 * state.f[2]);
#elif FILTERS == 4
		float const blue = 2 * (white - w3 * state.f[0] - w2 * state.f[1] - w1 * state.f[2] - w0 * state.f[3]);
#elif FILTERS == 5
		float const blue = 2 * (white - w4 * state.f[0] - w3 * state.f[1] - w2 * state.f[2] - w1 * state.f[3] - w0 * state.f[4]);
#endif

		if (param < 0.75f)
		{
			// blend between white and blue
			float const s = param * 4 - 2;
			return white + (blue - white) * s;
		}
		else
		{
			// +6dB/oct violet noise
			float const violet = 1.41421356f *(white - state.f[FILTERS]);
			state.f[FILTERS] = white;

			// blend between blue and violet
			float const s = param * 4 - 3;
			return blue + (violet - blue) * s;
		}
	}
#else
	return Random::Float() * 2.0f - 1.0f;
#endif
}
