#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter
*/

// filter type
#define FILTER_IMPROVED_MOOG 0
#define FILTER_NONLINEAR_MOOG 1
#define FILTER 0

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
		NOTCH,
		NOTCH_LOWPASS_1,
		NOTCH_LOWPASS_2,
		PHASESHIFT,
		PHASESHIFT_LOWPASS_1,

		COUNT
	};
	Mode mode;

	// cutoff frequency (logarithmic)
	float cutoff_base;
	float cutoff_lfo;
	float cutoff_env;

	// resonance parameter
	float resonance;

	FilterConfig(bool const enable, Mode const mode, float const cutoff_base, float const cutoff_lfo, float const cutoff_env, float const resonance)
		: enable(enable)
		, mode(mode)
		, cutoff_base(cutoff_base)
		, cutoff_lfo(cutoff_lfo)
		, cutoff_env(cutoff_env)
		, resonance(resonance)
	{
	}
};

// filter mode names
extern const char const * const filter_name[FilterConfig::COUNT];

// filter configuration
extern FilterConfig flt_config;

// filter state
class FilterState
{
public:
	// feedback coefficient
	float feedback;

#if FILTER == FILTER_NONLINEAR_MOOG

#define FILTER_OVERSAMPLE 2

	// output delayed by half a sample for phase compensation
	float delayed;

	// tuning coefficient
	float tune;

	// nonlinear output values from each stage
	float z[6];

#else

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
	float Apply(float const input, float const mix[]);
	float Update(FilterConfig const &config, float const cutoff, float const input, float const step);
};

// filter state
extern FilterState flt_state[KEYS];
