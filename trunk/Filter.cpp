/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter
*/
#include "StdAfx.h"

#include "Filter.h"
#include "Envelope.h"
#include "Math.h"
#include "Voice.h"

// filter configuration
FilterConfig flt_config(false, FilterConfig::LOWPASS_4, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

// filter envelope config
EnvelopeConfig flt_env_config(false, 0.0f, 1.0f, 0.0f, 0.1f);

// filter envelope state
EnvelopeState flt_env_state[VOICES];

// filter mode names
char const * const filter_name[FilterConfig::COUNT] =
{
	"Peak",
	"Low-Pass 1",
	"Low-Pass 2",
	"Low-Pass 3",
	"Low-Pass 4",
	"High-Pass 1",
	"High-Pass 2",
	"High-Pass 3",
	"High-Pass 4",
	"Band-Pass 1",
	"Band 1 + Low 1",
	"Band 1 + Low 2",
	"Band 1 + High 1",
	"Band 1 + High 2",
	"Band-Pass 2",
	"Notch 1",
	"Notch 1 + Low 1",
	"Notch 1 + Low 2",
	"Notch 1 + High 1",
	"Notch 1 + High 2",
	"Notch 2",
	"Phase Shift 1",
	"Phase Shift 2",
	"Phase Shift 3",
	"Phase Shift 4",
};

// The Oberheim Xpander and Matrix-12 analog synthesizers use a typical four-
// stage low-pass filter but combine voltages from each stage to produce 15
// different filter modes.  The publication describing the Improved Moog Filter
// mentioned this but gave no details.

// The circuit diagram on page 4 of the Oberheim Matrix-12 Service Manual
// shows how the filter works:
// http://elektrotanya.com/oberheim_matrix-12_sm.pdf/download.html

// The first three bits of the filter mode select one of eight resistor
// networks that combine the stage voltages in various ways.  The fourth
// bit disables the first filter stage.

// The mixing values below were derived from the resistor networks in the
// circuit diagram.  The IIR digital filter has an additional stage output
// to work with and no hard restriction on the number of options so there
// are several more filter options here than on the Oberheim synthesizers.

// Also: http://www.kvraudio.com/forum/viewtopic.php?p=3821632

// Creating Filters

// Use stage output directly for a 1-pole low-pass filter:
// LP(1) = y[1]

// Subtract stage output from stage input for a 1-pole high-pass filter"
// HP(1) = y[0] - y[1]

// Add coefficient vectors (+) to apply filters in parallel:
// Filter A: a0, a1, a2, a3, a4
// Filter B: b0, b1, b2, b3, b4
// Filter A + B: a0 + b0, a1 + b2, a2 + b2, a3 + b3, a4 + b4

// Scale coefficient vector (*) to scale filter output:
// Filter A: a0, a1, a2, a3, a4
// Scale: s
// Filter A * s: a0 * s, a1 * s, a2 * s, a3 * s, a4 * s

// Convolve coefficient vectors (*) to apply filters in series:
// Filter A: a0, a1, a2, a3, a4
// Filter B: b0, b1, b2, b3, b4
// Filter A * B:
//		a0 * b0,
//		a0 * b1 + a1 * b0,
//		a0 * b2 + a1 * b1 + a2 * b0,
//		a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0,
//		a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0

// Band-pass is a low-pass and high-pass in series:
// BP(1) = HP(1) * LP(1) or LP(1) * HP(1) = y[1] - 2 y[2]

// 1-Pole Low-Pass phase shift: -atan(frequency/cutoff)
// 0 degrees at low frequencies
// -45 degrees at the cutoff frequency
// -90 degrees at high frequencies

// 1-Pole High-Pass phase shift: pi/2 - atan(frequency/cutoff)
// +90 degrees at low frequencies
// +45 degrees at the cutoff frequency
// 0 degrees at high frequencies

// Filters in series combine phase shifts
// 4P Band-Pass = 2P Low-Pass * 2P High-Pass = pi - 4 atan(frequency/cutoff))

// Filters in parallel add signals
// 2P Notch = 2P Low-Pass + 2P High-Pass
// 2P Phase-Shift = 2P Low-Pass - 2P High-Pass
// 2P Low-Pass phase shift: -2 atan(frequency/cutoff)
// 2P High-Pass phase shift: pi - 2 atan(frequency/cutoff)
// Signals are 180 degrees out of phase so they have opposite signs
// 2P Notch adds the signals so they cancel each other
// 2P Phase-Shift subtracts the signals so they reinforce each other

// filter stage coefficients for each filter mode
static float const filter_mix[FilterConfig::COUNT][5] =
{
	//y0 y1 y2 y3 y4 
	{ 1, 0, 0, 0, 0 },		// PEAK,					// input with feedback term
	{ 0, 1, 0, 0, 0 },		// LOWPASS_1,				// LP(1) = y[1]
	{ 0, 0, 1, 0, 0 },		// LOWPASS_2,				// LP(2) = LP(1) * LP(1) = y[2]
	{ 0, 0, 0, 1, 0 },		// LOWPASS_3,				// LP(3) = LP(2) * LP(1) = y[3]
	{ 0, 0, 0, 0, 1 },		// LOWPASS_4,				// LP(4) = LP(3) * LP(1) = y[4]
	{ 1, -1, 0, 0, 0 },		// HIGHPASS_1,				// HP(1) = y[0] - y[1]
	{ 1, -2, 1, 0, 0 },		// HIGHPASS_2,				// HP(2) = HP(1) * HP(1)
	{ 1, -3, 3, -1, 0 },	// HIGHPASS_3,				// HP(3) = HP(2) * HP(1)
	{ 1, -4, 6, -4, 1 },	// HIGHPASS_4,				// HP(4) = HP(3) * HP(1)
	{ 0, 2, -2, 0, 0 },		// BANDPASS_1,				// BP(1) = LP(1) * HP(1) * 2
	{ 0, 0, 2, -2, 0 },		// BANDPASS_1_LOWPASS_1,	// BP(1) * LP(1)
	{ 0, 0, 0, 2, -2 },		// BANDPASS_1_LOWPASS_2,	// BP(1) * LP(2)
	{ 0, 2, -4, 2, 0 },		// BANDPASS_1_HIGHPASS_1,	// BP(1) * HP(1)
	{ 0, 2, -6, 6, -2 },	// BANDPASS_1_HIGHPASS_2,	// BP(1) * HP(2)
	{ 0, 0, 4, -8, 4 },		// BANDPASS_2,				// BP(2) = BP(1) * BP(1)
	{ 1, -2, 2, 0, 0 },		// NOTCH_1,					// N(1) = HP(2) + LP(2)
	{ 0, 1, -2, 2, 0 },		// NOTCH_1_LOWPASS_1,		// N(1) * LP(1)
	{ 0, 0, 1, -2, 2 },		// NOTCH_1_LOWPASS_2,		// N(1) * LP(2)
	{ 1, -3, 4, -2, 0 },	// NOTCH_1_HIGHPASS_1,		// N(1) * HP(1)
	{ 1, -4, 7, -6, 2 },	// NOTCH_1_HIGHPASS_2,		// N(1) * HP(2)
	{ 1, -4, 8, -8, 4 },	// NOTCH_2,					// N(2) = N(1) * N(1)
	{ 1, -2, 0, 0, 0 },		// PHASESHIFT_1,			// PS(1) = HP(1) - LP(1)
	{ 1, -4, 4, 0, 0 },		// PHASESHIFT_2,			// PS(2) = PS(1) * PS(1)
	{ 1, -6, 12, -8, 0 },	// PHASESHIFT_3,			// PS(3) = PS(2) * PS(1)
	{ 1, -8, 24, -32, 16 },	// PHASESHIFT_4,			// PS(4) = PS(3) * PS(1)
};

// filter state
FilterState flt_state[VOICES];

// reset filter state
void FilterState::Reset()
{
	feedback = 0.0f;
#if FILTER == FILTER_NONLINEAR_MOOG
	delayed = 0.0f;
	tune = 0.0f;
	memset(z, 0, sizeof(z));
#else
	a1 = 0.0f; b0 = 0.0f; b1 = 0.0f;
#endif
	memset(y, 0, sizeof(y));
}

// compute filter values based on cutoff frequency and resonance
void FilterState::Setup(float const cutoff, float const resonance, float const step)
{
	//float const fn = FILTER_OVERSAMPLE * 0.5f * info.freq;
	//float const fc = cutoff < fn ? cutoff / fn : 1.0f;
	float const fc = cutoff * step * 2.0f / FILTER_OVERSAMPLE;

#if FILTER == FILTER_IMPROVED_MOOG

	// Based on Improved Moog Filter description
	// http://www.music.mcgill.ca/~ich/research/misc/papers/cr1071.pdf

	float const g = 1 - expf(-M_PI * fc);
	feedback = 4.0f * resonance;
	// y[n] = ((1.0 / 1.3) * x[n] + (0.3 / 1.3) * x[n-1] - y[n-1]) * g + y[n-1]
	// y[n] = (g / 1.3) * x[n] + (g * 0.3 / 1.3) * x[n-1] - (g - 1) * y[n-1]
	a1 = g - 1.0f; b0 = g * 0.769231f; b1 = b0 * 0.3f;

#elif FILTER == FILTER_NONLINEAR_MOOG

	// Based on Antti Huovilainen's non-linear digital implementation
	// http://dafx04.na.infn.it/WebProc/Proc/P_061.pdf
	// https://raw.github.com/ddiakopoulos/MoogLadders/master/Source/Huovilainen.cpp
	// 0 <= resonance <= 1

	float const fcr = ((1.8730f * fc + 0.4955f) * fc + -0.6490f) * fc + 0.9988f;
	float const acr = (-3.9364f * fc + 1.8409f) * fc + 0.9968f;
	feedback = resonance * 4.0f * acr;
	tune = (1.0f - expf(-M_PI * fc * fcr)) * 1.22070313f;

#endif
}

// update the filter
float FilterState::Update(FilterConfig const &config, float const input)
{
#if FILTER == FILTER_IMPROVED_MOOG

#if FILTER_OVERSAMPLE > 1
	for (int i = 0; i < FILTER_OVERSAMPLE; ++i)
#endif
	{
		// feedback with nonlinear saturation
		float const comp = 0.5f;
		float const in = input - feedback * (FastTanh(y[4]) - comp * input);

		// stage 1: x1[n-1] = x1[n]; x1[n] = in;    y1[n-1] = y1[n]; y1[n] = func(y1[n-1], x1[n], x1[n-1])
		// stage 2: x2[n-1] = x2[n]; x2[n] = y1[n]; y2[n-1] = y2[n]; y2[n] = func(y2[n-1], x2[n], x2[n-1])
		// stage 3: x3[n-1] = x3[n]; x3[n] = y2[n]; y3[n-1] = y3[n]; y3[n] = func(y3[n-1], x3[n], x3[n-1])
		// stage 4: x4[n-1] = x4[n]; x4[n] = y3[n]; y4[n-1] = y4[n]; y4[n] = func(y4[n-1], x4[n], x4[n-1])

		// stage 1: x1[n-1] = x1[n]; x1[n] = in;    t1 = y1[n-1]; y1[n-1] = y1[n]; y1[n] = func(y1[n-1], x1[n], x1[n-1])
		// stage 2: x2[n-1] = t1;    x2[n] = y1[n]; t2 = y2[n-1]; y2[n-1] = y2[n]; y2[n] = func(y2[n-1], x2[n], x2[n-1])
		// stage 3: x3[n-1] = t2;    x3[n] = y2[n]; t3 = y3[n-1]; y3[n-1] = y3[n]; y3[n] = func(y3[n-1], x3[n], x3[n-1])
		// stage 4: x4[n-1] = t3;    x4[n] = y3[n];               y4[n-1] = y4[n]; y4[n] = func(y4[n-1], x4[n], x4[n-1])

		// t0 = y0[n], t1 = y1[n], t2 = y2[n], t3 = y3[n]
		// stage 0: y0[n] = in
		// stage 1: y1[n] = func(y1[n], y1[n], t0)
		// stage 2: y2[n] = func(y2[n], y1[n], t1)
		// stage 3: y3[n] = func(y3[n], y2[n], t2)
		// stage 4: y4[n] = func(y4[n], y3[n], t3)

		// four-pole low-pass filter
		float const t[4] = { y[0], y[1], y[2], y[3] };
		y[0] = in;
		y[1] = b0 * y[0] + b1 * t[0] - a1 * y[1];
		y[2] = b0 * y[1] + b1 * t[1] - a1 * y[2];
		y[3] = b0 * y[2] + b1 * t[2] - a1 * y[3];
		y[4] = b0 * y[3] + b1 * t[3] - a1 * y[4];
	}

	// generate output by mixing stage values
	float const *mix = filter_mix[config.mode];
	return
		y[0] * mix[0] +
		y[1] * mix[1] +
		y[2] * mix[2] +
		y[3] * mix[3] +
		y[4] * mix[4];

#elif FILTER == FILTER_NONLINEAR_MOOG

	// modified original algorithm based on sample code here:
	// http://www.kvraudio.com/forum/viewtopic.php?p=3821632
	float output;
#if FILTER_OVERSAMPLE > 1
	for (int i = 0; i < FILTER_OVERSAMPLE; ++i)
#endif
	{
		// nonlinear feedback
		y[0] = input - feedback * delayed;
		z[0] = FastTanh(y[0] * 0.8192f);

		// nonlinear four-pole low-pass filter
		y[1] += tune * (z[0] - z[1]);
		z[1] = FastTanh(y[1] * 0.8192f);
		y[2] += tune * (z[1] - z[2]);
		z[2] = FastTanh(y[2] * 0.8192f);
		y[3] += tune * (z[2] - z[3]);
		z[3] = FastTanh(y[3] * 0.8192f);
		y[4] += tune * (z[3] - z[4]);
		z[4] = FastTanh(y[4] * 0.8192f);

		// half-sample delay for phase compensation
		delayed = 0.5f * (z[4] + z[5]);
		z[5] = z[4];
	}

	// generate output by mixing stage values
	return
		z[0] * mix[0] +
		z[1] * mix[1] +
		z[2] * mix[2] +
		z[3] * mix[3] +
		z[4] * mix[4];

#endif
}
