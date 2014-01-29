#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter
*/

#include "Envelope.h"

// filter type
#define FILTER_IMPROVED_MOOG 0
#define FILTER_LINEAR_MOOG 1
#define FILTER_NONLINEAR_MOOG 2
#define FILTER FILTER_LINEAR_MOOG

// resonant lowpass filter
class FilterConfig
{
public:
	bool enable;
	enum Mode
	{
		PEAK,
		LOWPASS_1,
		LOWPASS_2,
		LOWPASS_3,
		LOWPASS_4,
		HIGHPASS_1,
		HIGHPASS_2,
		HIGHPASS_3,
		HIGHPASS_4,
		BANDPASS_1,
		BANDPASS_1_LOWPASS_1,
		BANDPASS_1_LOWPASS_2,
		BANDPASS_1_HIGHPASS_1,
		BANDPASS_1_HIGHPASS_2,
		BANDPASS_2,
		NOTCH_1,
		NOTCH_1_LOWPASS_1,
		NOTCH_1_LOWPASS_2,
		NOTCH_1_HIGHPASS_1,
		NOTCH_1_HIGHPASS_2,
		NOTCH_2,
		PHASESHIFT_1,
		PHASESHIFT_2,
		PHASESHIFT_3,
		PHASESHIFT_4,

		COUNT
	};
	Mode mode;
	float mix[5];

	// resonance parameter
	float resonance;

	// cutoff frequency (logarithmic)
	float cutoff_base;
	float cutoff_lfo;
	float cutoff_env;
	float cutoff_env_vel;

	FilterConfig(bool const enable, Mode const mode, float const resonance, float const cutoff_base, float const cutoff_lfo, float const cutoff_env, float const cutoff_env_vel)
		: enable(enable)
		, cutoff_base(cutoff_base)
		, cutoff_lfo(cutoff_lfo)
		, cutoff_env(cutoff_env)
		, cutoff_env_vel(cutoff_env_vel)
		, resonance(resonance)
	{
		SetMode(mode);
	}

	// set filter mode
	void SetMode(Mode newmode);

	// get the modulated cutoff value
	float GetCutoff(float const lfo, float const env, float const vel)
	{
		return powf(2, cutoff_base + lfo * cutoff_lfo + env * (cutoff_env + vel * cutoff_env_vel));
	}
};

// filter state
class FilterState
{
public:
	// feedback coefficient
	float feedback;

#if FILTER == FILTER_NONLINEAR_MOOG

#define FILTER_OVERSAMPLE 2

	// output delayed by half a sample for phase compensation
	float previous;
	float delayed;

	// tuning coefficient
	float tune;

	// nonlinear output values from each stage
	float z[5];

#elif FILTER == FILTER_LINEAR_MOOG

#define FILTER_OVERSAMPLE 2

	// output delayed by half a sample for phase compensation
	float previous;
	float delayed;

	// tuning coefficient
	float tune;

#elif FILTER == FILTER_IMPROVED_MOOG

#define FILTER_OVERSAMPLE 2

	// filter stage IIR coefficients
	// H(z) = (b0 * z + b1) / (z + a1)
	// H(z) = (b0 + b1 * z^-1) / (1 + a1 * z-1)
	// H(z) = Y(z) / X(z)
	// Y(z) = b0 + b1 * z^-1
	// X(z) = 1 + a1 * z^-1
	// (1 + a1 * z^-1) * Y(z) = (b0 + b1 * z^-1) * X(z)
	// y[n] + a1 * y[n - 1] = b0 * x[n] + b1 * x[n - 1]
	// y[n] = b0 * x[n] + b1 * x[n-1] - a1 * y[n-1]
	float b0, b1, a1;

#endif

	// linear output values from each stage
	// (y[0] is input to the first stage)
	float y[5];

	FilterState()
	{
		Reset();
	}
	void Reset(void);
	void Setup(float const cutoff, float const resonance, float const step);
	float Update(FilterConfig const &config, float const input);
};

// filter mode names
extern char const * const filter_name[FilterConfig::COUNT];

// filter configuration
extern FilterConfig flt_config;

// filter envelope configuration
extern EnvelopeConfig flt_env_config;

// filter state
extern FilterState flt_state[];

// filter envelope state
extern EnvelopeState flt_env_state[];
