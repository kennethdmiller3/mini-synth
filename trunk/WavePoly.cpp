/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Poly-Noise Wave
*/
#include "StdAfx.h"

#include "Wave.h"
#include "WavePoly.h"
#include "Oscillator.h"
#include "PolyBLEP.h"
#include "Math.h"

// precomputed linear feedback shift register output
char poly4[(1 << 4) - 1];
char poly5[(1 << 5) - 1];
char period93[93];
char poly9[(1 << 9) - 1];
char poly17[(1 << 17) - 1];

// precomputed wave tables for poly5-clocked waveforms
char pulsepoly5[ARRAY_SIZE(poly5) * 2];
char poly4poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly4)];
char poly17poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly17)];

static char const * const poly_data[WAVE_COUNT] =
{
	NULL,			// WAVE_SINE,
	NULL,			// WAVE_PULSE,
	NULL,			// WAVE_SAWTOOTH,
	NULL,			// WAVE_TRIANGLE,
	NULL,			// WAVE_NOISE,
	NULL,			// WAVE_NOISE_HOLD,
	NULL,			// WAVE_NOISE_SLOPE,
	poly4,			// WAVE_POLY4,
	poly5,			// WAVE_POLY5,
	period93,		// WAVE_PERIOD93,
	poly9,			// WAVE_POLY9,
	poly17,			// WAVE_POLY17,
	pulsepoly5,		// WAVE_PULSE_POLY5,
	poly4poly5,		// WAVE_POLY4_POLY5,
	poly17poly5,	// WAVE_POLY17_POLY5,
};

// generate polynomial table
// from Atari800 pokey.c
static void InitPoly(char aOut[], int aSize, int aTap, unsigned int aSeed, char aInvert)
{
	unsigned int x = aSeed;
	unsigned int i = 0;
	do
	{
		aOut[i] = (x & 1) ^ aInvert;
		x = ((((x >> aTap) ^ x) & 1) << (aSize - 1)) | (x >> 1);
		++i;
	}
	while (x != aSeed);
}

// generate pulsepoly5 table
static void InitPulsePoly5()
{
	char output = 0;
	int index5 = 0;
	for (int i = 0; i < ARRAY_SIZE(pulsepoly5); ++i)
	{
		if (poly5[index5])
			output = !output;
		pulsepoly5[i] = output;
		if (++index5 == ARRAY_SIZE(poly5))
			index5 = 0;
	}
}

// generate poly4poly5 table
static void InitPoly4Poly5()
{
	char output = 0;
	int index5 = 0;
	int index4 = 0;
	for (int i = 0; i < ARRAY_SIZE(poly4poly5); ++i)
	{
		if (++index4 == ARRAY_SIZE(poly4))
			index4 = 0;
		if (poly5[index5])
			output = poly4[index4];
		poly4poly5[i] = output;
		if (++index5 == ARRAY_SIZE(poly5))
			index5 = 0;
	}
}

// generate poly17poly5 table
static void InitPoly17Poly5()
{
	char output = 0;
	int index5 = 0;
	int index17 = 0;
	for (int i = 0; i < ARRAY_SIZE(poly17poly5); ++i)
	{
		if (++index17 == ARRAY_SIZE(poly17))
			index17 = 0;
		if (poly5[index5])
			output = poly17[index17];
		poly17poly5[i] = output;
		if (++index5 == ARRAY_SIZE(poly5))
			index5 = 0;
	}
}

// initialize polynomial noise tables
void InitPoly()
{
	InitPoly(poly4, 4, 1, 0xF, 0);
	InitPoly(poly5, 5, 2, 0x1F, 1);
	InitPoly(period93, 15, 6, 0x7FFF, 0);
	InitPoly(poly9, 9, 4, 0x1FF, 0);
	InitPoly(poly17, 17, 5, 0x1FFFF, 0);
	InitPulsePoly5();
	InitPoly4Poly5();
	InitPoly17Poly5();
}

// shared poly oscillator
float OscillatorPoly(OscillatorConfig const &config, OscillatorState &state, float step)
{
	// poly info for the wave type
	int const cycle = wave_loop_cycle[config.wavetype];
	if (step > 0.5f * cycle)
		return 0;

	// current wavetable value
	char const * const poly = poly_data[config.wavetype];
	float value = poly[state.index];

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
			char v0 = poly[i];
			for (int c = 0; c < count; ++c)
			{
				if (++i >= cycle)
					i -= cycle;
				t -= 1.0f;
				char v1 = poly[i];
				if (v0 != v1)
					value += PolyBLEP(t, w, float(v1 - v0));
				v0 = v1;
			}
		}
	}
#endif

	return value + value - 1.0f;
}
