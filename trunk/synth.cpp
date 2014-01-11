/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III
*/
#include "StdAfx.h"

#include "Debug.h"
#include "Console.h"
#include "Math.h"
#include "Random.h"
#include "Menu.h"

BASS_INFO info;
HSTREAM stream; // the stream

#define FILTER_IMPROVED_MOOG 0
#define FILTER_NONLINEAR_MOOG 1
#define FILTER 0

#define ANTIALIAS_NONE 0
#define ANTIALIAS_POLYBLEP 1
#define ANTIALIAS 1

bool use_antialias = true;

#define SPECTRUM_WIDTH 80
#define SPECTRUM_HEIGHT 8
CHAR_INFO const bar_full = { 0, BACKGROUND_GREEN };
CHAR_INFO const bar_top = { 223, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
CHAR_INFO const bar_bottom = { 220, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
CHAR_INFO const bar_empty = { 0, BACKGROUND_BLUE };
CHAR_INFO const bar_nyquist = { 0, BACKGROUND_RED };

#define NUM_OSCILLATORS 2

// display error message, clean up, and exit
void Error(char const *text)
{
	fprintf(stderr, "Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

#if ANTIALIAS == ANTIALIAS_POLYBLEP
// width of bandlimited step relative to the sample period
static const float POLYBLEP_WIDTH = 1.5f;

// Valimaki/Huovilainen PolyBLEP
static inline float PolyBLEP(float t, float w)
{
	if (t >= w)
		return 0.0f;
	if (t <= -w)
		return 0.0f;
	t /= w;
	if (t > 0.0f)
		return t + t - t * t - 1;
	else
		return t * t + t + t + 1;
}

// width of integrated bandlimited step relative to the sample period
static const float INTEGRATED_POLYBLEP_WIDTH = 1.5f;

// symbolically-integrated PolyBLEP
static inline float IntegratedPolyBLEP(float t, float w)
{
	if (t >= w)
		return 0.0f;
	if (t <= -w)
		return 0.0f;
	t /= w;
	float const t2 = t * t;
	float const t3 = t2 * t;
	if (t > 0.0f)
		return (0.33333333f - t + t2 - 0.33333333f * t3) * 4 * w;
	else
		return (0.33333333f + t + t2 + 0.33333333f * t3) * 4 * w;
}
#endif

// window title
char const title_text[] = ">>> MINI VIRTUAL ANALOG SYNTHESIZER";

// note key display
WORD const keys[] = {
	'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
};
COORD const key_pos = { 12, SPECTRUM_HEIGHT };
static size_t const KEYS = ARRAY_SIZE(keys);

char const * const fx_name[9] =
{
	"Chorus", "Compressor", "Distortion", "Echo", "Flanger", "Gargle", "I3DL2Reverb", "ParamEQ", "Reverb"
};

// effect config
bool fx_enable;
bool fx_active[9];

// effect handles
HFX fx[9];

// keyboard base frequencies
float keyboard_frequency[KEYS];

// keyboard octave
int keyboard_octave = 4;
float keyboard_timescale = 1.0f;

// most recent note key pressed
int keyboard_most_recent = 0;

// oscillator wave types
enum Wave
{
	WAVE_SINE,
	WAVE_PULSE,
	WAVE_SAWTOOTH,
	WAVE_TRIANGLE,
	WAVE_NOISE,

	WAVE_POLY4,			// POKEY AUDC 12
	WAVE_POLY5,
	WAVE_POLY17,		// POKEY AUDC 8
	WAVE_PULSE_POLY5,	// POKEY AUDC 2, 6
	WAVE_POLY4_POLY5,	// POKEY AUDC 4
	WAVE_POLY17_POLY5,	// POKEY AUDC 0

	WAVE_COUNT
};

// sub-oscillator modes
enum SubOscillatorMode
{
	SUBOSC_NONE,
	SUBOSC_SQUARE_1OCT,
	SUBOSC_SQUARE_2OCT,
	SUBOSC_PULSE_2OCT,
	SUBOSC_COUNT
};

const char * const sub_osc_name[SUBOSC_COUNT] =
{
	"None",
	"\xDF\xDC",
	"\xDF\xDF\xDC\xDC",
	"\xDF\xDC\xDC\xDC",
};

// precomputed linear feedback shift register output
static char poly4[(1 << 4) - 1];
static char poly5[(1 << 5) - 1];
static char poly17[(1 << 17) - 1];

// precomputed wave tables for poly5-clocked waveforms
static char pulsepoly5[ARRAY_SIZE(poly5) * 2];
static char poly4poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly4)];
static char poly17poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly17)];

// oscillator update functions
class OscillatorConfig;
class OscillatorState;
typedef float(*OscillatorFunc)(OscillatorConfig const &config, OscillatorState &state, float step);


// base frequency oscillator configuration
class OscillatorConfig
{
public:
	bool enable;

	Wave wavetype;
	float waveparam;
	float frequency;
	float amplitude;

	// hard sync
	bool sync_enable;
	float sync_phase;

	OscillatorConfig(bool const enable, Wave const wavetype, float const waveparam, float const frequency, float const amplitude)
		: enable(enable)
		, wavetype(wavetype)
		, waveparam(waveparam)
		, frequency(frequency)
		, amplitude(amplitude)
		, sync_enable(false)
		, sync_phase(1.0f)
	{
	}
};

// oscillator state
class OscillatorState
{
public:
	float phase;
	int index;

	OscillatorState()
	{
		Reset();
	}

	// reset the oscillator
	void Reset();

	// start the oscillator
	void Start();

	// update the oscillator by one step
	float Update(OscillatorConfig const &config, float frequency, float const step);

	// compute the oscillator value
	float Compute(OscillatorConfig const &config, float delta);

	// advance the oscillator phase
	void Advance(OscillatorConfig const &config, float delta);
};

// low-frequency oscillator configuration
class LFOOscillatorConfig : public OscillatorConfig
{
public:
	float frequency_base;	// logarithmic offset from 1 Hz

	explicit LFOOscillatorConfig(bool const enable = false, Wave const wavetype = WAVE_SINE, float const waveparam = 0.5f, float const frequency = 1.0f, float const amplitude = 1.0f)
		: OscillatorConfig(enable, wavetype, waveparam, frequency, amplitude)
		, frequency_base(0.0f)
	{
	}
};
LFOOscillatorConfig lfo_config;
// TO DO: multiple LFOs?
// TO DO: LFO key sync
// TO DO: LFO temp sync?
// TO DO: LFO keyboard follow dial?

// low-frequency oscillator state
OscillatorState lfo_state;

// note oscillator configuration
class NoteOscillatorConfig : public OscillatorConfig
{
public:
	// base parameters
	float waveparam_base;
	float frequency_base;	// logarithmic offset
	float amplitude_base;

	// LFO modulation parameters
	float waveparam_lfo;
	float frequency_lfo;
	float amplitude_lfo;

	// sub oscillator
	SubOscillatorMode sub_osc_mode;
	float sub_osc_amplitude;

	explicit NoteOscillatorConfig(bool const enable = false, Wave const wavetype = WAVE_SAWTOOTH, float const waveparam = 0.5f, float const frequency = 1.0f, float const amplitude = 1.0f)
		: OscillatorConfig(enable, wavetype, waveparam, frequency, amplitude)
		, waveparam_base(waveparam)
		, frequency_base(0.0f)
		, amplitude_base(amplitude)
		, waveparam_lfo(0.0f)
		, frequency_lfo(0.0f)
		, amplitude_lfo(0.0f)
		, sub_osc_mode(SUBOSC_NONE)
		, sub_osc_amplitude(0.0f)
	{
	}

	void Modulate(float lfo);
};

NoteOscillatorConfig osc_config[NUM_OSCILLATORS];
// TO DO: keyboard follow dial?
// TO DO: oscillator mixer

// note oscillator state
OscillatorState osc_state[KEYS][NUM_OSCILLATORS];

// envelope bias values to make the exponential decay arrive in finite time
// the attack part is set up to be more linear (and shorter) than the decay/release part
static const float ENV_ATTACK_CONSTANT = 1.0f;
static const float ENV_DECAY_CONSTANT = 3.0f;
static float const ENV_ATTACK_BIAS = 1.0f / (1.0f - expf(-ENV_ATTACK_CONSTANT)) - 1.0f;
static float const ENV_DECAY_BIAS = 1.0f - 1.0f / (1.0f - expf(-ENV_DECAY_CONSTANT));

// envelope generator configuration
class EnvelopeConfig
{
public:
	bool enable;
	float attack_time;
	float attack_rate;
	float decay_time;
	float decay_rate;
	float sustain_level;
	float release_time;
	float release_rate;

	EnvelopeConfig(bool const enable, float const attack_time, float const decay_time, float const sustain_level, float const release_time)
		: enable(enable)
		, attack_time(attack_time)
		, attack_rate(1 / Max(attack_time, 0.0001f))
		, decay_time(decay_time)
		, decay_rate(1 / Max(decay_time, 0.0001f))
		, sustain_level(sustain_level)
		, release_time(release_time)
		, release_rate(1 / Max(release_time, 0.0001f))
	{
	}
};
EnvelopeConfig flt_env_config(false, 0.0f, 1.0f, 0.0f, 0.1f);
EnvelopeConfig vol_env_config(false, 0.0f, 1.0f, 1.0f, 0.1f);

// envelope generator state
class EnvelopeState
{
public:
	bool gate;
	enum State
	{
		OFF,

		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE,

		COUNT
	};
	State state;
	float amplitude;

	EnvelopeState()
		: gate(false)
		, state(OFF)
		, amplitude(0.0f)
	{
	}

	void Gate(EnvelopeConfig const &config, bool on);

	float Update(EnvelopeConfig const &config, float const step);
};

EnvelopeState flt_env_state[KEYS];
EnvelopeState vol_env_state[KEYS];
// TO DO: multiple envelopes?  filter, amplifier, global

WORD const env_attrib[EnvelopeState::COUNT] =
{
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,	// EnvelopeState::OFF
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,				// EnvelopeState::ATTACK,
	BACKGROUND_RED | BACKGROUND_INTENSITY,					// EnvelopeState::DECAY,
	BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,	// EnvelopeState::SUSTAIN,
	BACKGROUND_INTENSITY,									// EnvelopeState::RELEASE,
};


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
FilterConfig flt_config(false, FilterConfig::LOWPASS_4, 0.0f, 0.0f, 0.0f, 0.0f);

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
	"Notch",
	"Notch + Low 1",
	"Notch + Low 2",
	"Phase Shift",
	"Phase + Low 1",
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

// LP(n) = y[n]
// LP(n), HP(1) = y[n + 1] - y[n]
// LP(n), HP(2) = -y[n + 2] + 2 * y[n + 1] - y[n]
// LP(n), HP(3) = y[n + 3] + 3 * y[n + 2] + 3 * y[n + 1] - y[n]
// LP(n), HP(4) = -y[n + 4] + 4 * y[n + 3] - 6 * y[n + 2] + 4 * y[n + 1] - y[n]
// BP(1) = LP(1), HP(1)
// BP(2) = LP(2), HP(2)
// Notch = HP(2) - LP(2)
// AP = 4 * y[3] - 6 * y[2] + 3*y[1] - y[0] = HP(4) + LP(4) - LP(2) ?

// filter stage coefficients for each filter mode
float const filter_mix[FilterConfig::COUNT][5] =
{
	//y0  y1  y2  y3  y4 
	{  1,  0,  0,  0,  0 },	// PEAK,					// input with feedback term
	{  0,  1,  0,  0,  0 }, // LOWPASS_1,
	{  0,  0,  1,  0,  0 }, // LOWPASS_2,
	{  0,  0,  0,  1,  0 }, // LOWPASS_3,
	{  0,  0,  0,  0,  1 }, // LOWPASS_4,
	{  1, -1,  0,  0,  0 }, // HIGHPASS_1,
	{  1, -2,  1,  0,  0 }, // HIGHPASS_2,
	{  1, -3,  3, -1,  0 }, // HIGHPASS_3,
	{  1, -4,  6, -4,  1 }, // HIGHPASS_4,
	{  0,  1, -1,  0,  0 }, // BANDPASS_1,				// LP(1) then HP(1)
	{  0,  0,  1, -1,  0 }, // BANDPASS_1_LOWPASS_1,	// LP(2) then HP(1)
	{  0,  0,  0,  1, -1 }, // BANDPASS_1_LOWPASS_2,	// LP(3) then HP(1)
	{  0,  1, -2,  1,  0 }, // BANDPASS_1_HIGHPASS_1,	// LP(1) then HP(2)
	{  0,  1, -3,  3, -1 }, // BANDPASS_1_HIGHPASS_2,	// LP(1) then HP(3)
	{  0,  0,  1, -2,  1 }, // BANDPASS_2,				// LP(2) then HP(2)
	{  1, -2,  2,  0,  0 }, // NOTCH,					// HP(2) * LP(2)
	{  0,  1, -2,  2,  0 }, // NOTCH_LOWPASS_1,			// LP(1) then HP(2) * LP(2)
	{  0,  0,  1, -2,  2 }, // NOTCH_LOWPASS_2,			// LP(2) then HP(2) * LP(2)
	{  1, -3,  6, -4,  0 }, // PHASESHIFT,
	{  0,  1, -3,  6, -4 }, // PHASESHIFT_LOWPASS_1,
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
FilterState flt_state[KEYS];

// output scale factor
float output_scale = 0.25f;	// 0.25f;

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

// names for wave types
char const * const wave_name[WAVE_COUNT] =
{
	"Sine",			// WAVE_SINE,
	"Pulse",		// WAVE_PULSE,
	"Sawtooth",		// WAVE_SAWTOOTH,
	"Triangle",		// WAVE_TRIANGLE,
	"Noise",		// WAVE_NOISE,
	"Poly4",		// WAVE_POLY4,
	"Poly5",		// WAVE_POLY5,
	"Poly17",		// WAVE_POLY17,
	"Pulse/Poly5",	// WAVE_PULSE_POLY5,
	"Poly4/Poly5",	// WAVE_POLY4_POLY5,
	"Poly17/Poly5"	// WAVE_POLY17_POLY5,
};

// multiply oscillator time scale based on wave type
// - tune the pitch of short-period poly oscillators
// - raise the pitch of poly oscillators by a factor of two
// - Atari POKEY pitch 255 corresponds to key 9 (N) in octave 2
float const wave_adjust_frequency[WAVE_COUNT] =
{
	1.0f,					// WAVE_SINE,
	1.0f,					// WAVE_PULSE,
	1.0f,					// WAVE_SAWTOOTH,
	1.0f,					// WAVE_TRIANGLE,
	1.0f, 					// WAVE_NOISE,
	2.0f * 15.0f / 16.0f,	// WAVE_POLY4,
	2.0f * 31.0f / 32.0f,	// WAVE_POLY5,
	2.0f,					// WAVE_POLY17,
	2.0f * 31.0f / 32.0f,	// WAVE_PULSE_POLY5,
	2.0f * 465.0f / 512.0f,	// WAVE_POLY4_POLY5,
	2.0f,					// WAVE_POLY17_POLY5,
};

// restart the oscillator loop index after this many phase cycles
// - poly oscillators using the loop index to look up precomputed values
int const wave_loop_cycle[WAVE_COUNT] =
{
	INT_MAX,					// WAVE_SINE,
	INT_MAX,					// WAVE_PULSE,
	INT_MAX,					// WAVE_SAWTOOTH,
	INT_MAX,					// WAVE_TRIANGLE,
	INT_MAX,					// WAVE_NOISE,
	ARRAY_SIZE(poly4),			// WAVE_POLY4,
	ARRAY_SIZE(poly5),			// WAVE_POLY5,
	ARRAY_SIZE(poly17),			// WAVE_POLY17,
	ARRAY_SIZE(pulsepoly5),		// WAVE_PULSE_POLY5,
	ARRAY_SIZE(poly4poly5),		// WAVE_POLY4_POLY5,
	ARRAY_SIZE(poly17poly5),	// WAVE_POLY17_POLY5,
};

// sine oscillator
static __forceinline float GetSineValue(float const phase)
{
	return sinf(M_PI * 2.0f * phase);
}
static __forceinline float GetSineSlope(float const phase)
{
	return cosf(M_PI * 2.0f * phase);
}
float OscillatorSine(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = GetSineValue(state.phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias && config.sync_enable)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);

		// handle discontinuity at zero phase
		if (state.phase < w)
		{
			value -= (GetSineValue(config.sync_phase)) * 0.5f * PolyBLEP(state.phase, w);
			value -= (GetSineSlope(config.sync_phase) - 1.0f) * 0.5f * IntegratedPolyBLEP(state.phase, w);
		}

		// handle discontinuity at sync phase
		if (state.phase - config.sync_phase > -w)
		{
			value -= (GetSineValue(config.sync_phase)) * 0.5f * PolyBLEP(state.phase - config.sync_phase, w);
			value -= (GetSineSlope(config.sync_phase) - 1.0f) * 0.5f * IntegratedPolyBLEP(state.phase - config.sync_phase, w);
		}
	}
#endif
	return value;
}

// sawtooth oscillator
// - 2/pi sum k=1..infinity sin(k*2*pi*phase)/n
// - smoothed transition to reduce aliasing
static __forceinline float GetSawtoothValue(float const phase)
{
	return 1.0f - 2.0f * (phase - FloorInt(phase));
}
float OscillatorSawtooth(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = GetSawtoothValue(state.phase);
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);
		if (config.sync_enable)
		{
			// handle last integer phase before sync from the previous wave cycle
			float up_before_zero = float(FloorInt(config.sync_phase)) - config.sync_phase;
			value += PolyBLEP(state.phase - up_before_zero, w);

			// handle dscontinuity at integer phases
			float const up_nearest = float(RoundInt(state.phase));
			if (up_nearest > 0 && up_nearest <= config.sync_phase)
				value += PolyBLEP(state.phase - up_nearest, w);

			// handle discontituity at sync phase
			float const sync_value = GetSawtoothValue(config.sync_phase);
			if (sync_value < 0.999969482421875f)
			{
				float const delta_value = 0.5f * (1.0f - sync_value);
				value += delta_value * PolyBLEP(state.phase, w);
				value += delta_value * PolyBLEP(state.phase - config.sync_phase, w);
			}
		}
		else
		{
			float const up_nearest = float(RoundInt(state.phase));
			value += PolyBLEP(state.phase - up_nearest, w);
		}
	}
#endif
	return value;
}

// pulse oscillator
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

		// nearest up edge
		float const up_nearest = float(RoundInt(state.phase));

		// nearest down edge
		float const down_nearest = float(RoundInt(state.phase - config.waveparam) + 1) - config.waveparam;

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

			// handle nearest up transition if it's in range
			if (up_nearest > 0 && up_nearest <= up_before_sync)
				value += PolyBLEP(state.phase - up_nearest, w);

			// handle nearest down transition if it's in range
			if (down_nearest > 0 && down_nearest < config.sync_phase)
				value -= PolyBLEP(state.phase - down_nearest, w);

			// handle discontinuity at sync phase
			if (config.sync_phase > up_before_sync + config.waveparam)
			{
				// up edge at config.sync_phase
				value += PolyBLEP(state.phase - config.sync_phase, w);
			}
		}
		else
		{
			value += PolyBLEP(state.phase - up_nearest, w);
			value -= PolyBLEP(state.phase - down_nearest, w);
		}
	}
#endif
	return value;
}

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
				float sync_value = GetTriangleValue(config.sync_phase);
				float const delta_value = -0.5f * sync_value;
				value += delta_value * PolyBLEP(state.phase, w);
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
				float sync_value = GetTriangleValue(config.sync_phase);
				float const delta_value = -0.5f * sync_value;
				value += delta_value * PolyBLEP(state.phase - config.sync_phase, w);
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

// shared poly oscillator
float OscillatorPoly(OscillatorConfig const &config, OscillatorState &state, char poly[], int cycle, float step)
{
	if (step > 0.5f * cycle)
		return 0;

	// current wavetable value
	float value = poly[state.index] ? 1.0f : -1.0f;
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
					value += (v1 - v0) * PolyBLEP(t, w);
				v0 = v1;
			}
		}
	}
#endif
	return value;
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

// white noise oscillator
float OscillatorNoise(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return Random::Float() * 2.0f - 1.0f;
}

// map wave type enumeration to oscillator function
OscillatorFunc const oscillator[WAVE_COUNT] =
{
	OscillatorSine,			// WAVE_SINE,
	OscillatorPulse,		// WAVE_PULSE,
	OscillatorSawtooth,		// WAVE_SAWTOOTH,
	OscillatorTriangle,		// WAVE_TRIANGLE,
	OscillatorNoise,		// WAVE_NOISE,
	OscillatorPoly4,		// WAVE_POLY4,
	OscillatorPoly5,		// WAVE_POLY5,
	OscillatorPoly17,		// WAVE_POLY17,
	OscillatorPulsePoly5,	// WAVE_PULSE_POLY5,
	OscillatorPoly4Poly5,	// WAVE_POLY4_POLY5,
	OscillatorPoly17Poly5	// WAVE_POLY17_POLY5,
};
// sub-oscillator
float SubOscillator(NoteOscillatorConfig const &config, OscillatorState const &state, float frequency, float step)
{
	float sub_value = (state.index & config.sub_osc_mode) ? -1.0f : 1.0f;
	float const w = frequency * step * POLYBLEP_WIDTH;
	if (state.phase < 0.5f)
	{
		float sub_prev = ((state.index - 1) & config.sub_osc_mode) ? -1.0f : 1.0f;
		if (sub_value != sub_prev)
			sub_value += 0.5f * (sub_value - sub_prev) * PolyBLEP(state.phase, w);
	}
	else
	{
		float sub_next = ((state.index + 1) & config.sub_osc_mode) ? -1.0f : 1.0f;
		if (sub_value != sub_next)
			sub_value += 0.5f * (sub_next - sub_value) * PolyBLEP(state.phase - 1, w);
	}
	return sub_value;
}

// reset oscillator
void OscillatorState::Reset()
{
	phase = 0.0f;
	index = 0;
}

// start oscillator
void OscillatorState::Start()
{
	Reset();
}

// update oscillator
float OscillatorState::Update(OscillatorConfig const &config, float const frequency, float const step)
{
	if (!config.enable)
		return 0.0f;

	float const delta = config.frequency * frequency * step;

	// compute oscillator value
	float const value = Compute(config, delta);

	// advance oscillator phase
	Advance(config, delta);

	return value;
}

// compute the oscillator value
float OscillatorState::Compute(OscillatorConfig const &config, float delta)
{
	return config.amplitude * oscillator[config.wavetype](config, *this, delta);
}

// advance the oscillator phase
void OscillatorState::Advance(OscillatorConfig const &config, float delta)
{
	phase += delta;
	if (phase >= config.sync_phase)
	{
		// wrap phase around
		int const advance = FloorInt(phase / config.sync_phase);
		phase -= advance * config.sync_phase;

		// advance the wavetable index
		int const cycle = wave_loop_cycle[config.wavetype];
		index += advance;
		if (index >= cycle)
			index -= cycle;
	}
	else if (phase < 0.0f)
	{
		// wrap phase around
		int const advance = FloorInt(phase / config.sync_phase);
		phase -= advance * config.sync_phase;

		// rewind the wavetable index
		int const cycle = wave_loop_cycle[config.wavetype];
		index += advance;
		if (index < 0)
			index += cycle;
	}
}

// modulate note oscillator
void NoteOscillatorConfig::Modulate(float lfo)
{
	// LFO wave wave parameter modulation
	waveparam = waveparam_base + waveparam_lfo * lfo;

	// LFO frequency modulation
	frequency = powf(2.0f, frequency_base + frequency_lfo * lfo) * wave_adjust_frequency[wavetype];

	// LFO amplitude modulation
	amplitude = amplitude_base + amplitude_lfo * lfo;
}

// gate envelope generator
void EnvelopeState::Gate(EnvelopeConfig const &config, bool on)
{
	if (gate == on)
		return;

	gate = on;
	if (gate)
		state = config.enable ? EnvelopeState::ATTACK : EnvelopeState::SUSTAIN;
	else
		state = config.enable ? EnvelopeState::RELEASE : EnvelopeState::OFF;
}

// update envelope generator
float EnvelopeState::Update(EnvelopeConfig const &config, float const step)
{
	if (!config.enable)
		return gate;
	float env_target;
	switch (state)
	{
	case ATTACK:
		env_target = 1.0f + ENV_ATTACK_BIAS;
		amplitude += (env_target - amplitude) * config.attack_rate * step;
		if (amplitude >= 1.0f)
		{
			amplitude = 1.0f;
			if (config.sustain_level < 1.0f)
				state = DECAY;
			else
				state = SUSTAIN;
		}
		break;

	case DECAY:
		env_target = config.sustain_level + (1.0f - config.sustain_level) * ENV_DECAY_BIAS;
		amplitude += (env_target - amplitude) * config.decay_rate * step;
		if (amplitude <= config.sustain_level)
		{
			amplitude = config.sustain_level;
			state = SUSTAIN;
		}
		break;

	case RELEASE:
		env_target = ENV_DECAY_BIAS;
		if (amplitude < config.sustain_level || config.decay_rate < config.release_rate)
			amplitude += (env_target - amplitude) * config.release_rate * step;
		else
			amplitude += (env_target - amplitude) * config.decay_rate * step;
		if (amplitude <= 0.0f)
		{
			amplitude = 0.0f;
			state = OFF;
		}
		break;
	}

	return amplitude;
}

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

float FilterState::Apply(float const input, float const mix[])
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

float FilterState::Update(FilterConfig const &config, float const cutoff, float const input, float const step)
{
	// compute filter values
	Setup(cutoff, flt_config.resonance, step);

	// apply the filter
	return Apply(input, filter_mix[flt_config.mode]);
}

DWORD CALLBACK WriteStream(HSTREAM handle, short *buffer, DWORD length, void *user)
{
	// get active oscillators
	int index[KEYS];
	int active = 0;
	for (int k = 0; k < ARRAY_SIZE(keys); k++)
	{
		if (vol_env_state[k].state != EnvelopeState::OFF)
		{
			index[active++] = k;
		}
	}

	// number of samples
	size_t count = length / (2 * sizeof(short));

	if (active == 0)
	{
		// clear buffer
		memset(buffer, 0, length);

		// advance low-frequency oscillator
		float lfo = lfo_state.Update(lfo_config, 1.0f, float(count) / info.freq);

		// compute shared oscillator values
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			osc_config[o].Modulate(lfo);
		}

		return length;
	}

	// time step per output sample
	float const step = 1.0f / info.freq;

	// for each output sample...
	for (size_t c = 0; c < count; ++c)
	{
		// get low-frequency oscillator value
		float const lfo = lfo_state.Update(lfo_config, 1.0f, step);

		// compute shared oscillator values
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			osc_config[o].Modulate(lfo);
		}

		// accumulated sample value
		float sample = 0.0f;

		// for each active oscillator...
		for (int i = 0; i < active; ++i)
		{
			int k = index[i];

			// key frequency (taking octave shift into account)
			float const key_freq = keyboard_frequency[k] * keyboard_timescale;

			// update filter envelope generator
			float const flt_env_amplitude = flt_env_state[k].Update(flt_env_config, step);

			// update volume envelope generator
			float const vol_env_amplitude = vol_env_state[k].Update(vol_env_config, step);

			// if the envelope generator finished...
			if (vol_env_state[k].state == EnvelopeState::OFF)
			{
				// remove from active oscillators
				--active;
				index[i] = index[active];
				--i;
				continue;
			}

			// set up sync phases
			for (int o = 1; o < NUM_OSCILLATORS; ++o)
			{
				if (osc_config[o].sync_enable)
					osc_config[o].sync_phase = osc_config[o].frequency / osc_config[0].frequency;
			}

			// update oscillator
			// (assume key follow)
			float osc_value = 0.0f;
			for (int o = 0; o < NUM_OSCILLATORS; ++o)
			{
				if (osc_config[o].sub_osc_mode)
					osc_value += osc_config[o].sub_osc_amplitude * SubOscillator(osc_config[o], osc_state[k][o], key_freq, step);
				osc_value += osc_state[k][o].Update(osc_config[o], key_freq, step);
			}

			// update filter
			float flt_value;
			if (flt_config.enable)
			{
				// compute cutoff frequency
				// (assume key follow)
				float const cutoff = key_freq * powf(2, flt_config.cutoff_base + flt_config.cutoff_lfo * lfo + flt_config.cutoff_env * flt_env_amplitude);

				// get filtered oscillator value
				flt_value = flt_state[k].Update(flt_config, cutoff, osc_value, step);
			}
			else
			{
				// pass unfiltered value
				flt_value = osc_value;
			}

			// apply envelope to amplitude and accumulate result
			sample += flt_value * vol_env_amplitude;
		}

		// left and right channels are the same
		//short output = (short)Clamp(int(sample * output_scale * 32768), SHRT_MIN, SHRT_MAX);
		short output = (short)(FastTanh(sample * output_scale) * 32768);
		*buffer++ = output;
		*buffer++ = output;
	}

	return length;
}

void PrintOutputScale(HANDLE hOut)
{
	COORD pos = { 1, 10 };
	PrintConsole(hOut, pos, "- + Output: %5.1f%%", output_scale * 100.0f);

	DWORD written;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED, 1, pos, &written);
	pos.X += 2;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, 1, pos, &written);
}

void PrintKeyOctave(HANDLE hOut)
{
	COORD pos = { 21, 10 };
	PrintConsole(hOut, pos, "[ ] Key Octave: %d", keyboard_octave);

	DWORD written;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED, 1, pos, &written);
	pos.X += 2;
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE, 1, pos, &written);
}

void PrintAntialias(HANDLE hOut)
{
	COORD pos = { 41, 10 };
	PrintConsole(hOut, pos, "F12 Antialias: %3s", use_antialias ? "ON" : "OFF");

	DWORD written;
	pos.X += 15;
	FillConsoleOutputAttribute(hOut, use_antialias ? FOREGROUND_GREEN : FOREGROUND_RED, 3, pos, &written);
}

void UpdateOscillatorFrequencyDisplay(HANDLE hOut, int o);

void MenuOSC(HANDLE hOut, WORD key, DWORD modifiers, MenuMode menu)
{
	int const o = menu - MENU_OSC1;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 9 + (o > 0);
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 9 + (o > 0))
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			osc_config[o].enable = (key == VK_RIGHT);
			break;
		case 1:
			osc_config[o].wavetype = Wave((osc_config[o].wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			for (int k = 0; k < KEYS; ++k)
				osc_state[k][o].Reset();
			break;
		case 2:
			UpdatePercentageProperty(osc_config[o].waveparam_base, sign, modifiers, 0, 1);
			break;
		case 3:
			UpdateFrequencyProperty(osc_config[o].frequency_base, sign, modifiers, -5, 5);
			break;
		case 4:
			UpdatePercentageProperty(osc_config[o].amplitude_base, sign, modifiers, -10, 10);
			break;
		case 5:
			UpdatePercentageProperty(osc_config[o].waveparam_lfo, sign, modifiers, -10, 10);
			break;
		case 6:
			UpdateFrequencyProperty(osc_config[o].frequency_lfo, sign, modifiers, -5, 5);
			break;
		case 7:
			UpdatePercentageProperty(osc_config[o].amplitude_lfo, sign, modifiers, -10, 10);
			break;
		case 8:
			osc_config[o].sub_osc_mode = SubOscillatorMode((osc_config[o].sub_osc_mode + sign + SUBOSC_COUNT) % SUBOSC_COUNT);
			break;
		case 9:
			UpdatePercentageProperty(osc_config[o].sub_osc_amplitude, sign, modifiers, -10, 10);
			break;
		case 10:
			if (key == VK_RIGHT)
			{
				osc_config[o].sync_enable = true;
			}
			else
			{
				osc_config[o].sync_enable = false;
				osc_config[o].sync_phase = 1.0f;
			}
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = osc_config[o].enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, NULL, "OFF");

	for (int i = 1; i <= 9 + (o > 0); ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", wave_name[osc_config[o].wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", osc_config[o].waveparam_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Frequency: %+7.2f", osc_config[o].frequency_base * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Amplitude:% 7.1f%%", osc_config[o].amplitude_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Width LFO: %+6.1f%%", osc_config[o].waveparam_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq LFO:  %+7.2f", osc_config[o].frequency_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Ampl LFO: %+7.1f%%", osc_config[o].amplitude_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Sub Osc:%10s", sub_osc_name[osc_config[o].sub_osc_mode]);

	++pos.Y;
	PrintConsole(hOut, pos, "Sub Ampl: % 7.1f%%", osc_config[o].sub_osc_amplitude * 100.0f);

	if (o > 0)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "Hard Sync:     %3s", osc_config[o].sync_enable ? "ON" : "OFF");
		WORD const attrib = menu_item_attrib[item_selected == 10];
		COORD const pos2 = { pos.X + 15, pos.Y };
		FillConsoleOutputAttribute(hOut, (attrib & 0xF8) | (osc_config[o].sync_enable ? FOREGROUND_GREEN : FOREGROUND_RED), 3, pos2, &written);
	}
}

void MenuOSC1(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuOSC(hOut, key, modifiers, MENU_OSC1);
}

void MenuOSC2(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuOSC(hOut, key, modifiers, MENU_OSC2);
}

void MenuLFO(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_LFO;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 3;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 3)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			lfo_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			lfo_config.wavetype = Wave((lfo_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			lfo_state.Reset();
			break;
		case 2:
			UpdatePercentageProperty(lfo_config.waveparam, sign, modifiers, 0, 1);
			break;
		case 3:
			UpdateFrequencyProperty(lfo_config.frequency_base, sign, modifiers, -8, 14);
			lfo_config.frequency = powf(2.0f, lfo_config.frequency_base);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = lfo_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 3; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", wave_name[lfo_config.wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", lfo_config.waveparam * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq: %10.3fHz", lfo_config.frequency);
}

void MenuFLT(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_FLT;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 9;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 9)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			flt_config.enable = (key == VK_RIGHT);
			flt_env_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			flt_config.mode = FilterConfig::Mode((flt_config.mode + FilterConfig::COUNT + sign) % FilterConfig::COUNT);
			break;
		case 2:
			UpdatePercentageProperty(flt_config.resonance, sign, modifiers, 0, 4);
			break;
		case 3:
			UpdateFrequencyProperty(flt_config.cutoff_base, sign, modifiers, -10, 10);
			break;
		case 4:
			UpdateFrequencyProperty(flt_config.cutoff_lfo, sign, modifiers, -10, 10);
			break;
		case 5:
			UpdateFrequencyProperty(flt_config.cutoff_env, sign, modifiers, -10, 10);
			break;
		case 6:
			UpdateTimeProperty(flt_env_config.attack_time, sign, modifiers, 0, 10);
			flt_env_config.attack_rate = 1.0f / Max(flt_env_config.attack_time, 0.0001f);
			break;
		case 7:
			UpdateTimeProperty(flt_env_config.decay_time, sign, modifiers, 0, 10);
			flt_env_config.decay_rate = 1.0f / Max(flt_env_config.decay_time, 0.0001f);
			break;
		case 8:
			UpdatePercentageProperty(flt_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 9:
			UpdateTimeProperty(flt_env_config.release_time, sign, modifiers, 0, 10);
			flt_env_config.release_rate = 1.0f / Max(flt_env_config.release_time, 0.0001f);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = flt_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 9; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d FLT         %3s", menu + 1, flt_config.enable ? "ON" : "OFF");

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", filter_name[flt_config.mode]);

	++pos.Y;
	PrintConsole(hOut, pos, "Resonance: % 7.3f", flt_config.resonance);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff:    % 7.2f", flt_config.cutoff_base * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff LFO:% 7.2f", flt_config.cutoff_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff ENV:% 7.2f", flt_config.cutoff_env * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %7.3fs", flt_env_config.attack_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %7.3fs", flt_env_config.decay_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", flt_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %7.3fs", flt_env_config.release_time);
}

void MenuVOL(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_VOL;
	COORD pos = menu_pos[menu];
	DWORD written;
	int sign;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = 4;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > 4)
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[menu])
		{
		case 0:
			vol_env_config.enable = (key == VK_RIGHT);
			break;
		case 1:
			UpdateTimeProperty(vol_env_config.attack_time, sign, modifiers, 0, 10);
			vol_env_config.attack_rate = 1.0f / Max(vol_env_config.attack_time, 0.0001f);
			break;
		case 2:
			UpdateTimeProperty(vol_env_config.decay_time, sign, modifiers, 0, 10);
			vol_env_config.decay_rate = 1.0f / Max(vol_env_config.decay_time, 0.0001f);
			break;
		case 3:
			UpdatePercentageProperty(vol_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 4:
			UpdateTimeProperty(vol_env_config.release_time, sign, modifiers, 0, 10);
			vol_env_config.release_rate = 1.0f / Max(vol_env_config.release_time, 0.0001f);
			break;
		}
		break;
	}

	// menu title
	bool const component_enabled = vol_env_config.enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 1; i <= 4; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[item_selected == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d VOL", menu + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %7.3fs", vol_env_config.attack_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %7.3fs", vol_env_config.decay_time);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", vol_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %7.3fs", vol_env_config.release_time);
}

static void EnableEffect(int index, bool enable)
{
	if (enable)
	{
		if (!fx[index])
		{
			// set the effect, not bothering with parameters (use defaults)
			fx[index] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + index, 0);
		}
	}
	else
	{
		if (fx[index])
		{
			BASS_ChannelRemoveFX(stream, fx[index]);
			fx[index] = 0;
		}
	}
}

void MenuFX(HANDLE hOut, WORD key, DWORD modifiers)
{
	MenuMode const menu = MENU_FX;
	COORD pos = menu_pos[menu];
	DWORD written;

	switch (key)
	{
	case 0:
		if (menu_active == menu)
			ShowMarker(hOut, menu, menu_item[menu]);
		else
			HideMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_UP:
		HideMarker(hOut, menu, menu_item[menu]);
		if (--menu_item[menu] < 0)
			menu_item[menu] = ARRAY_SIZE(fx);
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_DOWN:
		HideMarker(hOut, menu, menu_item[menu]);
		if (++menu_item[menu] > ARRAY_SIZE(fx))
			menu_item[menu] = 0;
		ShowMarker(hOut, menu, menu_item[menu]);
		break;

	case VK_LEFT:
	case VK_RIGHT:
		if (menu_item[menu] == 0)
		{
			// set master switch
			fx_enable = (key == VK_RIGHT);
			for (int i = 0; i < ARRAY_SIZE(fx); ++i)
				EnableEffect(i, fx_enable && fx_active[i]);
		}
		else
		{
			// set item
			fx_active[menu_item[menu] - 1] = (key == VK_RIGHT);
			EnableEffect(menu_item[menu] - 1, fx_enable && fx_active[menu_item[menu] - 1]);
		}
		break;
	}

	// menu title
	bool const component_enabled = fx_enable;
	bool const menu_selected = menu_active == menu;
	int const item_selected = menu_selected ? menu_item[menu] : -1;
	PrintMenuTitle(hOut, menu, component_enabled, "ON", "OFF");

	for (int i = 0; i < ARRAY_SIZE(fx); ++i)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "%-11s    %3s", fx_name[i], fx_active[i] ? "ON" : "OFF");
		WORD const attrib = menu_item_attrib[item_selected == i + 1];
		FillConsoleOutputAttribute(hOut, attrib, 15, pos, &written);
		COORD const pos2 = { pos.X + 15, pos.Y };
		FillConsoleOutputAttribute(hOut, (attrib & 0xF8) | (fx_active[i] ? FOREGROUND_GREEN : FOREGROUND_RED), 3, pos2, &written);
	}
}

typedef void(*MenuFunc)(HANDLE hOut, WORD key, DWORD modifiers);
MenuFunc menufunc[MENU_COUNT] =
{
	MenuOSC1, MenuOSC2, MenuLFO, MenuFLT, MenuVOL, MenuFX
};

// SPECTRUM ANALYZER
// horizontal axis shows semitone frequency bands
// vertical axis shows logarithmic power
void UpdateSpectrumAnalyzer(HANDLE hOut)
{
	// get complex FFT data
#define FREQUENCY_BINS 4096
	float fft[FREQUENCY_BINS * 2][2];
	BASS_ChannelGetData(stream, &fft[0][0], BASS_DATA_FFT8192 | BASS_DATA_FFT_COMPLEX);

	// center frequency of the zeroth semitone band
	// (one octave down from the lowest key)
	float const freq_min = keyboard_frequency[0] * keyboard_timescale * 0.5f;

	// get the lower frequency bin for the zeroth semitone band
	// (half a semitone down from the center frequency)
	float const semitone = powf(2.0f, 1.0f / 12.0f);
	float freq = freq_min * FREQUENCY_BINS * 2.0f / info.freq / sqrtf(semitone);
	int b0 = Max(RoundInt(freq), 0);

	// get power in each semitone band
	float spectrum[SPECTRUM_WIDTH] = { 0 };
	int xlimit = SPECTRUM_WIDTH;
	for (int x = 0; x < SPECTRUM_WIDTH; ++x)
	{
		// get upper frequency bin for the current semitone
		freq *= semitone;
		int const b1 = Min(RoundInt(freq), FREQUENCY_BINS);

		// ensure there's at least one bin
		// (or quit upon reaching the last bin)
		if (b0 == b1)
		{
			if (b1 == FREQUENCY_BINS)
			{
				xlimit = x;
				break;
			}
			--b0;
		}

		// sum power across the semitone band
		float scale = float(FREQUENCY_BINS) / float(b1 - b0);
		float value = 0.0f;
		for (; b0 < b1; ++b0)
			value += fft[b0][0] * fft[b0][0] + fft[b0][1] * fft[b0][1];
		spectrum[x] = scale * value;
	}

	// inaudible band
	int xinaudible = RoundInt(logf(20000 / freq_min) * 12 / logf(2));

	// plot log-log spectrum
	// each grid cell is one semitone wide and 6 dB high
	CHAR_INFO buf[SPECTRUM_HEIGHT][SPECTRUM_WIDTH];
	COORD const pos = { 0, 0 };
	COORD const size = { SPECTRUM_WIDTH, SPECTRUM_HEIGHT };
	SMALL_RECT region = { 0, 0, 79, 49 };
	float threshold = 1.0f;
	for (int y = 0; y < SPECTRUM_HEIGHT; ++y)
	{
		int x;
		for (x = 0; x < xlimit; ++x)
		{
			if (spectrum[x] < threshold)
				buf[y][x] = bar_empty;
			else if (spectrum[x] < threshold * 2.0f)
				buf[y][x] = bar_bottom;
			else if (spectrum[x] < threshold * 4.0f)
				buf[y][x] = bar_top;
			else
				buf[y][x] = bar_full;
			if (x >= xinaudible)
				buf[y][x].Attributes |= BACKGROUND_RED;
		}
		for (; x < SPECTRUM_WIDTH; ++x)
		{
			buf[y][x] = bar_nyquist;
		}
		threshold *= 0.25f;
	}
	WriteConsoleOutput(hOut, &buf[0][0], size, pos, &region);
}

void UpdateKeyVolumeEnvelopeDisplay(HANDLE hOut, EnvelopeState::State vol_env_display[])
{
	for (int k = 0; k < KEYS; k++)
	{
		if (vol_env_display[k] != vol_env_state[k].state)
		{
			vol_env_display[k] = vol_env_state[k].state;
			COORD pos = { SHORT(key_pos.X + k), key_pos.Y };
			DWORD written;
			WriteConsoleOutputAttribute(hOut, &env_attrib[vol_env_display[k]], 1, pos, &written);
		}
	}
}

// waveform display settings
void UpdateOscillatorWaveformDisplay(HANDLE hOut)
{
#define WAVEFORM_WIDTH 80
#define WAVEFORM_HEIGHT 20
#define WAVEFORM_MIDLINE 10

	// show the oscillator wave shape
	// (using the lowest key frequency as reference)
	WORD const positive = BACKGROUND_BLUE, negative = BACKGROUND_RED;
	CHAR_INFO const plot[2] = {
		{ 223, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE },
		{ 220, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE }
	};
	CHAR_INFO buf[WAVEFORM_HEIGHT][WAVEFORM_WIDTH] = { 0 };
	COORD const pos = { 0, 0 };
	COORD const size = { WAVEFORM_WIDTH, WAVEFORM_HEIGHT };
	SMALL_RECT region = { 0, 50 - WAVEFORM_HEIGHT, WAVEFORM_WIDTH - 1, 49 };

	// get low-frequency oscillator value
	// (assume it is constant for the duration)
	float const lfo = lfo_state.Update(lfo_config, 1.0f, 0.0f);

	// how many cycles to plot?
	int cycle = wave_loop_cycle[osc_config[0].wavetype];
	if (cycle == INT_MAX)
	{
		if (osc_config[0].sub_osc_mode == SUBOSC_NONE)
			cycle = 1;
		else if (osc_config[0].sub_osc_mode < SUBOSC_SQUARE_2OCT)
			cycle = 2;
		else
			cycle = 4;
	}
	else if (cycle > WAVEFORM_WIDTH)
	{
		cycle = WAVEFORM_WIDTH;
	}

	// base phase step for plot
	float const step_base = cycle / float(WAVEFORM_WIDTH);

	// reference key
	int const k = keyboard_most_recent;

	// key frequency
	float const key_freq = keyboard_frequency[k] * keyboard_timescale;

	// update filter envelope generator
	float const flt_env_amplitude = flt_env_state[k].amplitude;

	// update volume envelope generator
	float const vol_env_amplitude = vol_env_config.enable ? vol_env_state[k].amplitude : 1;

	// base phase delta
	float const delta_base = key_freq / info.freq;

	// local oscillators for plot
	NoteOscillatorConfig config[NUM_OSCILLATORS];
	OscillatorState state[NUM_OSCILLATORS];
	float step[NUM_OSCILLATORS];
	float delta[NUM_OSCILLATORS];

	for (int o = 0; o < NUM_OSCILLATORS; ++o)
	{
		// copy current oscillator config
		config[o] = osc_config[o];

		// compute shared oscillator values
		// (assume that LFO doesn't change over the plotted period)
		config[o].Modulate(lfo);

		// step and delta phase
		float const relative = config[o].frequency / config[0].frequency;
		step[o] = step_base * relative;
		delta[o] = delta_base * config[0].frequency * relative;

		// half-step initial phase
		state[o].phase = 0.5f * step[o];
	}

	// local filter for plot
	static FilterState filter;

	// detect when to reset the filter
	static bool prev_active;
	static int prev_k;

	// compute cutoff frequency
	// (assume key follow)
	float const cutoff = powf(2, flt_config.cutoff_base + flt_config.cutoff_lfo * lfo + flt_config.cutoff_env * flt_env_amplitude);

	// elapsed time in milliseconds since the previous frame
	static DWORD prevTime = timeGetTime();
	DWORD deltaTime = timeGetTime() - prevTime;
	prevTime += deltaTime;

	// reset filter state on playing a note
	if (prev_k != k)
	{
		filter.Reset();
		prev_k = k;
	}
	if (prev_active != (vol_env_state[k].state != EnvelopeState::OFF))
	{
		filter.Reset();
		prev_active = (vol_env_state[k].state != EnvelopeState::OFF);
	}

	// if the filter is enabled...
	if (flt_config.enable)
	{
		// get steps needed to advance OSC1 by the time step
		// (subtracting the steps that the plot itself will take)
		int steps = FloorInt(key_freq * osc_config[0].frequency * deltaTime / 1000 - 1) * WAVEFORM_WIDTH;
		if (steps > 0)
		{
			// "rewind" oscillators so they'll end at zero phase
			for (int o = 0; o < NUM_OSCILLATORS; ++o)
			{
				if (!config[o].enable)
					continue;
				state[o].Advance(config[o], step[config[o].sync_enable ? 0 : o] * steps);
			}

			// run the filter ahead for next time
			for (int x = 0; x < steps; ++x)
			{
				// sum the oscillator outputs
				float value = 0.0f;
				for (int o = 0; o < NUM_OSCILLATORS; ++o)
				{
					if (!config[o].enable)
						continue;

					if (config[o].sync_enable)
						config[o].sync_phase = config[o].frequency / config[0].frequency;
					if (config[o].sub_osc_mode)
						value += config[o].sub_osc_amplitude * SubOscillator(config[o], state[o], 1, delta[o]);
					value += state[o].Compute(config[o], delta[o]);
					state[o].Advance(config[o], step[o]);
				}

				// apply filter
				value = filter.Update(flt_config, cutoff, value, step_base);

				// apply volume envelope
				value *= vol_env_amplitude;
			}
		}
	}

#ifdef SCROLL_WAVEFORM_DISPLAY
	// scroll through the waveform
	static float phase_offset = 0.0f;
	static float index_offset = 0;
	phase_offset += 0.3f * step;
	if (phase_offset >= 1.0f)
	{
		phase_offset -= 1.0f;
		if (++index_offset >= wave_loop_cycle[config.wavetype])
			index_offset = 0;
	}
	state.phase += phase_offset;
	state.index += index_offset;
#endif

	for (int x = 0; x < WAVEFORM_WIDTH; ++x)
	{
		// sum the oscillator outputs
		float value = 0.0f;
		for (int o = 0; o < NUM_OSCILLATORS; ++o)
		{
			if (!config[o].enable)
				continue;
			if (config[o].sync_enable)
				config[o].sync_phase = config[o].frequency / config[0].frequency;
			if (config[o].sub_osc_mode)
				value += config[o].sub_osc_amplitude * SubOscillator(config[o], state[o], 1, delta[o]);
			value += state[o].Compute(config[o], delta[o]);
			state[o].Advance(config[o], step[o]);
		}

		if (flt_config.enable)
		{
			// apply filter
			value = filter.Update(flt_config, cutoff, value, step_base);
		}

		// apply volume envelope
		value *= vol_env_amplitude;

		// plot waveform column
		int grid_y = FloorInt(-(WAVEFORM_HEIGHT - 0.5f) * value);
		int y = WAVEFORM_MIDLINE + (grid_y >> 1);
		if (value > 0.0f)
		{
			if (y >= 0)
			{
				buf[y][x] = plot[grid_y & 1];
				y += grid_y & 1;
			}
			else
			{
				y = 0;
			}

			for (int fill = y; fill < WAVEFORM_MIDLINE; ++fill)
				buf[fill][x].Attributes |= positive;
		}
		else
		{
			if (y < WAVEFORM_HEIGHT)
			{
				buf[y][x] = plot[grid_y & 1];
				--y;
				y += grid_y & 1;
			}
			else
			{
				y = WAVEFORM_HEIGHT - 1;
			}
			for (int fill = y; fill >= WAVEFORM_MIDLINE; --fill)
				buf[fill][x].Attributes |= negative;
		}
	}
	WriteConsoleOutput(hOut, &buf[0][0], size, pos, &region);
}

// show oscillator frequency
void UpdateOscillatorFrequencyDisplay(HANDLE hOut, int o)
{
	// reference key
	int const k = keyboard_most_recent;

	// key frequency (taking octave shift into account)
	float const key_freq = keyboard_frequency[k] * keyboard_timescale;

	// get attributes to use
	MenuMode menu = MenuMode(MENU_OSC1 + o);
	COORD const pos = { menu_pos[menu].X + 8, menu_pos[menu].Y };
	bool const menu_selected = (menu_active == menu);
	bool const title_selected = menu_selected && menu_item[menu] == 0;
	WORD const title_attrib = menu_title_attrib[true][menu_selected + title_selected];
	WORD const num_attrib = (title_attrib & 0xF8) | (FOREGROUND_GREEN);
	WORD const unit_attrib = (title_attrib & 0xF8) | (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	// current frequency in Hz
	float const freq = key_freq * osc_config[o].frequency;

	if (freq >= 20000.0f)
	{
		// print in kHz
		PrintConsoleWithAttribute(hOut, pos, num_attrib, "%7.2f", freq / 1000.0f);
		COORD const pos2 = { pos.X + 7, pos.Y };
		PrintConsoleWithAttribute(hOut, pos2, unit_attrib, "kHz");
	}
	else
	{
		// print in Hz
		PrintConsoleWithAttribute(hOut, pos, num_attrib, "%8.2f", freq);
		COORD const pos2 = { pos.X + 8, pos.Y };
		PrintConsoleWithAttribute(hOut, pos2, unit_attrib, "Hz");
	}
}

void UpdateLowFrequencyOscillatorDisplay(HANDLE hOut)
{
	// initialize buffer
	CHAR_INFO const negative = { 0, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
	CHAR_INFO const positive = { 0, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
	CHAR_INFO buf[18];
	for (int x = 0; x < 9; x++)
		buf[x] = negative;
	for (int x = 9; x < 18; ++x)
		buf[x] = positive;

	// plot low-frequency oscillator value
	CHAR const plot[2] = { 221, 222 };
	float const lfo = lfo_state.Update(lfo_config, 1.0f, 0.0f);
	int grid_x = Clamp(FloorInt(18.0f * lfo + 18.0f), 0, 35);
	buf[grid_x / 2].Char.AsciiChar = plot[grid_x & 1];

	// draw the gauge
	COORD const pos = { 0, 0 };
	COORD const size = { 18, 1 };
	SMALL_RECT region = { 
		menu_pos[MENU_LFO].X, menu_pos[MENU_LFO].Y + 4,
		menu_pos[MENU_LFO].X + 19, menu_pos[MENU_LFO].Y + 4
	};
	WriteConsoleOutput(hOut, &buf[0], size, pos, &region);
}

void main(int argc, char **argv)
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	int running = 1;

#ifdef WIN32
	if (IsDebuggerPresent())
	{
		// turn on floating-point exceptions
		unsigned int prev;
		_clearfp();
		_controlfp_s(&prev, 0, _EM_ZERODIVIDE|_EM_INVALID);
	}
#endif

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
	{
		fprintf(stderr, "An incorrect version of BASS.DLL was loaded");
		return;
	}

	// set the window title
	SetConsoleTitle(TEXT(title_text));

	// set the console buffer size
	static const COORD bufferSize = { 80, 50 };
	SetConsoleScreenBufferSize(hOut, bufferSize);

	// set the console window size
	static const SMALL_RECT windowSize = { 0, 0, 79, 49 };
	SetConsoleWindowInfo(hOut, TRUE, &windowSize);

	// clear the window
	Clear(hOut);

	// hide the cursor
	static const CONSOLE_CURSOR_INFO cursorInfo = { 100, FALSE };
	SetConsoleCursorInfo(hOut, &cursorInfo);

	// set input mode
	SetConsoleMode(hIn, 0);

	// 10ms update period
	const int STREAM_UPDATE_PERIOD = 10;
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, STREAM_UPDATE_PERIOD);

	// initialize BASS sound library
	if (!BASS_Init(-1, 48000, BASS_DEVICE_LATENCY, 0, NULL))
		Error("Can't initialize device");

	// get device info
	BASS_GetInfo(&info);
	DebugPrint("device latency: %dms\n", info.latency);
	DebugPrint("device minbuf: %dms\n", info.minbuf);
	DebugPrint("ds version: %d (effects %s)\n", info.dsver, info.dsver < 8 ? "disabled" : "enabled");

	// default buffer size = update period + 'minbuf' + 1ms extra margin
	BASS_SetConfig(BASS_CONFIG_BUFFER, STREAM_UPDATE_PERIOD + info.minbuf + 1);
	DebugPrint("using a %dms buffer\r", BASS_GetConfig(BASS_CONFIG_BUFFER));

	// if the device's output rate is unknown default to 44100 Hz
	if (!info.freq) info.freq = 44100;

	// create a stream, stereo so that effects sound nice
	stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);

#ifdef BANDLIMITED_SAWTOOTH
	// initialize bandlimited sawtooth tables
	InitSawtooth();
#endif

	// initialize polynomial noise tables
	InitPoly(poly4, 4, 1, 0xF, 0);
	InitPoly(poly5, 5, 2, 0x1F, 1);
	InitPoly(poly17, 17, 5, 0x1FFFF, 0);
	InitPulsePoly5();
	InitPoly4Poly5();
	InitPoly17Poly5();

	// compute frequency for each note key
	for (int k = 0; k < KEYS; ++k)
	{
		keyboard_frequency[k] = powf(2.0F, (k + 3) / 12.0f) * 220.0f;	// middle C = 261.626Hz; A above middle C = 440Hz
		//keyboard_frequency[k] = powF(2.0F, k / 12.0f) * 256.0f;		// middle C = 256Hz
	}

	// show the note keys
	for (int k = 0; k < KEYS; ++k)
	{
		char buf[] = { char(keys[k]), '\0' };
		COORD pos = { SHORT(key_pos.X + k), SHORT(key_pos.Y) };
		WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		DWORD written;
		WriteConsoleOutputAttribute(hOut, &attrib, 1, pos, &written);
		WriteConsoleOutputCharacter(hOut, buf, 1, pos, &written);
	}

	// show output scale and key octave
	PrintOutputScale(hOut);
	PrintKeyOctave(hOut);
	PrintAntialias(hOut);

	// enable the first oscillator
	osc_config[0].enable = true;

	// show all menus
	for (int i = 0; i < MENU_COUNT - (info.dsver < 8); ++i)
		menufunc[i](hOut, 0, 0);

	// start playing the audio stream
	BASS_ChannelPlay(stream, FALSE);

	// previous volume envelope state for each key
	EnvelopeState::State vol_env_display[KEYS] = { EnvelopeState::OFF };

	while (running)
	{
		// if there are any pending input events...
		DWORD numEvents = 0;
		while (GetNumberOfConsoleInputEvents(hIn, &numEvents) && numEvents > 0)
		{
			// get the next input event
			INPUT_RECORD keyin;
			ReadConsoleInput(hIn, &keyin, 1, &numEvents);
			if (keyin.EventType == KEY_EVENT)
			{
				// handle interface keys
				if (keyin.Event.KeyEvent.bKeyDown)
				{
					WORD code = keyin.Event.KeyEvent.wVirtualKeyCode;
					DWORD modifiers = keyin.Event.KeyEvent.dwControlKeyState;
					if (code == VK_ESCAPE)
					{
						running = 0;
						break;
					}
					else if (code == VK_OEM_MINUS || code == VK_SUBTRACT)
					{
						output_scale -= 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_PLUS || code == VK_ADD)
					{
						output_scale += 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_OEM_4)	// '['
					{
						if (keyboard_octave > 0)
						{
							--keyboard_octave;
							keyboard_timescale *= 0.5f;
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_OEM_6)	// ']'
					{
						if (keyboard_octave < 8)
						{
							++keyboard_octave;
							keyboard_timescale *= 2.0f;
							PrintKeyOctave(hOut);
						}
					}
					else if (code == VK_F12)
					{
						use_antialias = !use_antialias;
						PrintAntialias(hOut);
					}
					else if (code >= VK_F1 && code < VK_F1 + MENU_COUNT - (info.dsver < 8))
					{
						int prev_active = menu_active;
						menu_active = MENU_COUNT;
						menufunc[prev_active](hOut, 0, 0);
						menu_active = MenuMode(code - VK_F1);
						menufunc[menu_active](hOut, 0, 0);
					}
					else if (code == VK_TAB)
					{
						int prev_active = menu_active;
						menu_active = MENU_COUNT;
						menufunc[prev_active](hOut, 0, 0);
						if (modifiers & SHIFT_PRESSED)
							menu_active = MenuMode(prev_active == 0 ? MENU_COUNT - 1 : prev_active - 1);
						else
							menu_active = MenuMode(prev_active == MENU_COUNT - 1 ? 0 : prev_active + 1);
						menufunc[menu_active](hOut, 0, 0);
					}
					else if (code == VK_UP || code == VK_DOWN || code == VK_RIGHT || code == VK_LEFT)
					{
						menufunc[menu_active](hOut, code, modifiers);
					}
				}

				// handle note keys
				for (int k = 0; k < KEYS; k++)
				{
					if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[k])
					{
						// gate filters based on key down
						bool gate = (keyin.Event.KeyEvent.bKeyDown != 0);

						// gate the filter envelope
						flt_env_state[k].Gate(flt_env_config, gate);

						// if pressing the key
						if (gate)
						{
							// save the note key
							keyboard_most_recent = k;

							// if the volume envelope is currently off...
							if (vol_env_state[k].state == EnvelopeState::OFF)
							{
								// start the oscillator
								// (assume restart on key)
								for (int o = 0; o < NUM_OSCILLATORS; ++o)
									osc_state[k][o].Start();

								// start the filter
								flt_state[k].Reset();
							}
						}

						// gate the volume envelope
						vol_env_state[k].Gate(vol_env_config, gate);
						break;
					}
				}
			}
		}

		// update the spectrum analyzer display
		UpdateSpectrumAnalyzer(hOut);

		// update note key volume envelope display
		UpdateKeyVolumeEnvelopeDisplay(hOut, vol_env_display);

		// update the oscillator waveform display
		UpdateOscillatorWaveformDisplay(hOut);

		// update the oscillator frequency displays
		for (int o = 0; o < 2; ++o)
		{
			if (osc_config[o].enable)
				UpdateOscillatorFrequencyDisplay(hOut, o);
		}

		// update the low-frequency oscillator dispaly
		UpdateLowFrequencyOscillatorDisplay(hOut);

		// sleep for 1/60th of second
		Sleep(16);
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
