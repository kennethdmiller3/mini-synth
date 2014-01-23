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
static float OscillatorPoly(OscillatorConfig const &config, OscillatorState &state, char poly[], int cycle, float step)
{
	if (step > 0.5f * cycle)
		return 0;

	// current wavetable value
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

// poly4 oscillator
// 4-bit linear feedback shift register noise
float OscillatorPoly4(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly4, ARRAY_SIZE(poly4), step);
}

// poly5 oscillator
// 5-bit linear feedback shift register noise
float OscillatorPoly5(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly5, ARRAY_SIZE(poly5), step);
}

// period93 oscillator
// 15-bit linear feedback shift register noise with variant tap position
float OscillatorPeriod93(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, period93, ARRAY_SIZE(period93), step);
}

// poly9 oscillator
// 9-bit linear feedback shift register noise
float OscillatorPoly9(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly9, ARRAY_SIZE(poly9), step);
}

// poly17 oscillator
// 17-bit linear feedback shift register noise
float OscillatorPoly17(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly17, ARRAY_SIZE(poly17), step);
}

// pulse wave clocked by poly5
// (what the Atari POKEY actually does with poly5)
float OscillatorPulsePoly5(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, pulsepoly5, ARRAY_SIZE(pulsepoly5), step);
}

// poly4 clocked by poly5
float OscillatorPoly4Poly5(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly4poly5, ARRAY_SIZE(poly4poly5), step);
}

// poly17 clocked by poly5
float OscillatorPoly17Poly5(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly17poly5, ARRAY_SIZE(poly17poly5), step);
}
