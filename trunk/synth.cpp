/*
MINI VIRTUAL ANALOG SYNTHESIZER
Derived from BASS simple synth demo
*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include "bass.h"

BASS_INFO info;
HSTREAM stream; // the stream

#define FILTER_IMPROVED_MOOG 0
#define FILTER_NONLINEAR_MOOG 1
#define FILTER 0

#define ANTIALIAS_NONE 0
#define ANTIALIAS_POLYBLEP 1
#define ANTIALIAS 1

#define SPECTRUM_WIDTH 80
#define SPECTRUM_HEIGHT 8
CHAR_INFO const bar_full = { 0, BACKGROUND_GREEN };
CHAR_INFO const bar_top = { 223, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
CHAR_INFO const bar_bottom = { 220, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
CHAR_INFO const bar_empty = { 0, BACKGROUND_BLUE };
CHAR_INFO const bar_nyquist = { 0, BACKGROUND_RED };

// debug output
int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf_s(buf, sizeof(buf), format, ap);
	OutputDebugStringA(buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

// get the last error message
void GetLastErrorMessage(CHAR buf[], DWORD size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, size, NULL);
}

// formatted write console output
int PrintConsole(HANDLE out, COORD pos, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char buf[256];
	vsnprintf_s(buf, sizeof(buf), format, ap);
	DWORD written;
	WriteConsoleOutputCharacter(out, buf, strlen(buf), pos, &written);
	return written;
}

// display error messages
void Error(char const *text)
{
	fprintf(stderr, "Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

template<typename T> static inline T Squared(T x)
{
	return x * x;
}
template<typename T> static inline T Clamp(T x, T a, T b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}
template<typename T> static inline T Min(T a, T b)
{
	return a < b ? a : b;
}
template<typename T> static inline T Max(T a, T b)
{
	return a > b ? a : b;
}

static inline float Saturate(float x)
{
	return 0.5f * (fabsf(x + 1.0f) - fabsf(x - 1.0f));
}

static inline float Fmod1(float x)
{
	return x - floorf(x);
}

static inline float FastTanhUnclamped(float x)
{
	return x * (27 + x * x) / (27 + 9 * x * x);
}

static inline float FastTanh(float x)
{
	if (x < -3)
		return -1;
	if (x > 3)
		return 1;
	return x * (27 + x * x) / (27 + 9 * x * x);
}

#if ANTIALIAS == ANTIALIAS_POLYBLEP
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

char const title_text[] = ">>> MINI VIRTUAL ANALOG SYNTHESIZER";
COORD const title_pos = { 0, 0 };

WORD const keys[] = {
	'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
};
COORD const key_pos = { 12, SPECTRUM_HEIGHT };
//{
//	{ 1, 5 }, { 2, 4 }, { 3, 5 }, { 4, 4 }, { 5, 5 }, { 7, 5 }, { 8, 4 }, { 9, 5 }, { 10, 4 }, { 11, 5 }, { 12, 4 }, { 13, 5 },
//	{ 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 7, 2 }, { 8, 1 }, { 9, 2 }, { 10, 1 }, { 11, 2 }, { 12, 1 }, { 13, 2 },
//	{ 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 7, 2 }, { 8, 1 }, { 9, 2 }, { 10, 1 }, { 11, 2 }, { 12, 1 }, { 13, 2 },
//	{ 15, 2 }, { 16, 1 }, { 17, 2 }, { 18, 1 }, { 19, 2 }, { 21, 2 }, { 22, 1 }, { 23, 2 }, { 24, 1 }, { 25, 2 }, { 26, 1 }, { 27, 2 },
//};
static size_t const KEYS = ARRAY_SIZE(keys);

enum MenuMode
{
	MENU_LFO,
	MENU_OSC,
	MENU_FLT,
	MENU_VOL,
	MENU_FX,

	MENU_COUNT
};

WORD const menu_title_attrib[] =
{
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
};
WORD const menu_item_attrib[] =
{
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
};

// selected menu
MenuMode menu_active = MENU_OSC;

// selected item in each menu
int menu_item[MENU_COUNT];

char const * const fx_name[9] =
{
	"Chorus", "Compressor", "Distortion", "Echo", "Flanger", "Gargle", "I3DL2Reverb", "ParamEQ", "Reverb"
};

// effect handles
HFX fx[9];

// keyboard base frequencies
float keyboard_frequency[KEYS];

// keyboard octave
int keyboard_octave = 4;
float keyboard_timescale = 1.0f;

// oscillator wave types
enum Wave
{
	WAVE_SINE,
	WAVE_PULSE,
	WAVE_SAWTOOTH,
	WAVE_TRIANGLE,
	WAVE_POLY4,
	WAVE_POLY5,
	WAVE_POLY17,
	WAVE_NOISE,

	WAVE_COUNT
};

// precomputed linear feedback shift register output
static char poly4[(1 << 4) - 1];
static char poly5[(1 << 5) - 1];
static char poly17[(1 << 17) - 1];

// oscillator update functions
struct OscillatorConfig;
struct OscillatorState;
typedef float(*OscillatorFunc)(OscillatorConfig const &config, OscillatorState &state, float step);
typedef void(*OscillatorLoopFunc)(OscillatorConfig const &config, OscillatorState &state, float step);


// base frequency oscillator configuration
struct OscillatorConfig
{
	Wave wavetype;
	float waveparam;
	float frequency;
	float amplitude;

	OscillatorConfig()
		: wavetype(WAVE_SINE)
		, waveparam(0.5f)
		, frequency(1.0f)
		, amplitude(1.0f)
	{
	}
};

// oscillator state
struct OscillatorState
{
	float value;
	float phase;
	int loops;

	float Update(OscillatorConfig const &config, float frequency, float const step);
};

// low-frequency oscillator configuration
struct LFOOscillatorConfig : public OscillatorConfig
{
	float frequency_base;	// logarithmic offset from 1 Hz
};
LFOOscillatorConfig lfo_config;
// TO DO: multiple LFOs?
// TO DO: LFO key sync
// TO DO: LFO temp sync?
// TO DO: LFO keyboard follow dial?

// low-frequency oscillator state
OscillatorState lfo_state;

// note oscillator configuration
struct NoteOscillatorConfig : public OscillatorConfig
{
	// base parameters
	float waveparam_base;
	float frequency_base;	// logarithmic offset
	float amplitude_base;

	// LFO modulation parameters
	float waveparam_lfo;
	float frequency_lfo;
	float amplitude_lfo;

	NoteOscillatorConfig()
		: OscillatorConfig()
		, waveparam_base(0.5f)
		, frequency_base(0.0f)
		, amplitude_base(1.0f)
		, waveparam_lfo(0.0f)
		, frequency_lfo(0.0f)
		, amplitude_lfo(0.0f)
	{
		wavetype = WAVE_SAWTOOTH;
	}
};
NoteOscillatorConfig osc_config;
// TO DO: multiple oscillators
// TO DO: hard sync
// TO DO: keyboard follow dial?
// TO DO: oscillator mixer

// note oscillator state
OscillatorState osc_state[KEYS];

// envelope generator configuration
struct EnvelopeConfig
{
	float attack_rate;
	float decay_rate;
	float sustain_level;
	float release_rate;
};
EnvelopeConfig flt_env_config = { 256.0f, 16.0f, 0.0f, 256.0f };
EnvelopeConfig vol_env_config = { 256.0f, 16.0f, 1.0f, 256.0f };

// envelope generator state
struct EnvelopeState
{
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

	float Update(EnvelopeConfig const &config, float const step);
};

EnvelopeState flt_env_state[KEYS];
EnvelopeState vol_env_state[KEYS];
static float const ENV_ATTACK_BIAS = 1.0f/(1.0f-expf(-1.0f))-1.0f;	// 1x time constant
static float const ENV_DECAY_BIAS = 1.0f-1.0f/(1.0f-expf(-3.0f));	// 3x time constant
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
struct FilterConfig
{
	enum Mode
	{
		NONE,
		ALLPASS,
		LOWPASS_1,
		LOWPASS_2,
		LOWPASS_3,
		LOWPASS_4,
		HIGHPASS_1,
		HIGHPASS_2,
		HIGHPASS_3,
		HIGHPASS_4,
		BANDPASS,
		NOTCH,

		COUNT
	};
	Mode mode;
	float cutoff;
	float cutoff_lfo;
	float cutoff_env;
	float resonance;
};
FilterConfig flt_config = { FilterConfig::NONE, 0.0f, 0.0f, 0.0f, 0.0f };

const char * const filter_name[FilterConfig::COUNT] =
{
	"None", "All-Pass",
	"Low-Pass 1", "Low-Pass 2", "Low-Pass 3", "Low-Pass 4",
	"High-Pass 1", "High-Pass 2", "High-Pass 3", "High-Pass 4",
	"Band-Pass", "Notch"
};

// filter state
struct FilterState
{
#if FILTER == FILTER_NONLINEAR_MOOG

#define FILTER_OVERSAMPLE 2

	float feedback;
	float tune;
	float ytan[4];
	float y[5];

#else

#define FILTER_OVERSAMPLE 2

	// feedback coefficient
	float feedback;

	// filter stage IIR coefficients
	// H(z) = (b0 * z + b1) / (z + a1)
	// H(z) = (b0 + b1 * z^-1) / (1 + a1 * z-1)
	// H(z) = Y(z) / X(z)
	// Y(z) = b0 + b1 * z^-1
	// X(z) = 1 + a1 * z^-1
	// (1 + a1 * z^-1) * Y(z) = (b0 + b1 * z^-1) * X(z)
	// y[n] + a1 * y[n - 1] = b0 * x[n] + b1 * x[n - 1]
	// y[n] = b0 * x[n] + b1 * 0.3 * x[n-1] - a1 * y[n-1]
	float b0, b1, a1;

	// output values from each stage
	// (y[0] is input to the first stage)
	float y[5];

#endif

	void Clear(void);
	void Setup(float cutoff, float resonance);
	float Apply(float input);
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

// names for wave types
char const * const wave_name[WAVE_COUNT] =
{
	"Sine", "Pulse", "Sawtooth", "Triangle", "Poly4", "Poly5", "Poly17", "Noise"
};

// starting value
float const wave_start_value[WAVE_COUNT] =
{
	0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f
};

// multiply oscillator time scale based on wave type
// - tune the pitch of short-period poly oscillators
// - raise the pitch of poly oscillators by a factor of two
// - Atari POKEY pitch 255 corresponds to key 9 (N) in octave 2
float const wave_adjust_frequency[WAVE_COUNT] =
{
	1.0f, 1.0f, 1.0f, 1.0f, 2.0f * 15.0f / 16.0f, 2.0f * 31.0f / 32.0f, 2.0f, 1.0f
};

// restart the oscillator loop index after this many phase cycles
// - poly oscillators using the loop index to look up precomputed values
int const wave_loop_cycle[WAVE_COUNT] =
{
	1, 1, 1, 1, ARRAY_SIZE(poly4), ARRAY_SIZE(poly5), ARRAY_SIZE(poly17), 1
};

// sine oscillator
float OscillatorSine(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float const value = sinf(M_PI * 2.0f * state.phase);
	return value;
}

bool use_antialias = true;

// sawtooth oscillator
// - 2/pi sum k=1..infinity sin(k*2*pi*phase)/n
// - smoothed transition to reduce aliasing
float OscillatorSawtooth(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = 1.0f - 2.0f * state.phase;
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);
		value += PolyBLEP(state.phase, w);
		value += PolyBLEP(state.phase - 1, w);
	}
#endif
	return value;
}

// pulse oscillator
// - param controls pulse width
// - pulse width 0.5 is square wave
// - 4/pi sum k=0..infinity sin((2*k+1)*2*pi*phase)/(2*k+1)
// - smoothed transition to reduce aliasing
float OscillatorPulse(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	if (config.waveparam <= 0.0f)
		return -1.0f;
	if (config.waveparam >= 1.0f)
		return 1.0f;
	float value = state.phase < config.waveparam ? 1.0f : -1.0f;
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * POLYBLEP_WIDTH, 1.0f);
		value -= PolyBLEP(state.phase + 1 - config.waveparam, w);
		value += PolyBLEP(state.phase, w);
		value -= PolyBLEP(state.phase - config.waveparam, w);
		value += PolyBLEP(state.phase - 1, w);
		value -= PolyBLEP(state.phase - 1 - config.waveparam, w);
	}
#endif
	return value;
}

// triangle oscillator
// - 8/pi**2 sum k=0..infinity (-1)**k sin((2*k+1)*2*pi*phase)/(2*k+1)**2
float OscillatorTriangle(OscillatorConfig const &config, OscillatorState &state, float step)
{
	if (step > 0.5f)
		return 0.0f;
	float value = fabsf(2 - fabsf(4 * state.phase - 1)) - 1;
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float const w = Min(step * INTEGRATED_POLYBLEP_WIDTH, 0.5f);
		value -= IntegratedPolyBLEP(state.phase + 0.75f, w);
		value += IntegratedPolyBLEP(state.phase + 0.25f, w);
		value -= IntegratedPolyBLEP(state.phase - 0.25f, w);
		value += IntegratedPolyBLEP(state.phase - 0.75f, w);
	}
#endif
	return value;
}

// shared poly oscillator
float OscillatorPoly(OscillatorConfig const &config, OscillatorState &state, char poly[], int cycle, float step)
{
	float value = state.value;
#if ANTIALIAS == ANTIALIAS_POLYBLEP
	if (use_antialias)
	{
		float w = Min(step * POLYBLEP_WIDTH, 1.0f);
		if (state.phase < w)
		{
			if (poly[state.loops])
				value += state.value * PolyBLEP(state.phase, w);
		}
		if (state.phase > 1.0f - w)
		{
			int next_loop = state.loops + 1;
			if (next_loop >= cycle)
				next_loop = 0;
			if (poly[next_loop])
				value -= state.value * PolyBLEP(state.phase - 1, w);
		}
	}
#endif
	return value;
}
void OscillatorPolyLoop(OscillatorConfig const &config, OscillatorState &state, char poly[], int cycle, float step)
{
	if (++state.loops >= cycle)
		state.loops = 0;

	// toggle output value when the poly table value is true
	if (poly[state.loops])
		state.value = -state.value;
}

// poly4 oscillator
// 4-bit linear feedback shift register noise
float OscillatorPoly4(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly4, wave_loop_cycle[WAVE_POLY4], step);
}
void OscillatorPoly4Loop(OscillatorConfig const &config, OscillatorState &state, float step)
{
	OscillatorPolyLoop(config, state, poly4, wave_loop_cycle[WAVE_POLY4], step);
}

// poly5 oscillator
// 5-bit linear feedback shift register noise
float OscillatorPoly5(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly5, wave_loop_cycle[WAVE_POLY5], step);
}
void OscillatorPoly5Loop(OscillatorConfig const &config, OscillatorState &state, float step)
{
	OscillatorPolyLoop(config, state, poly5, wave_loop_cycle[WAVE_POLY5], step);
}

// poly17 oscillator
// 17-bit linear feedback shift register noise
float OscillatorPoly17(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return OscillatorPoly(config, state, poly17, wave_loop_cycle[WAVE_POLY17], step);
}
void OscillatorPoly17Loop(OscillatorConfig const &config, OscillatorState &state, float step)
{
	OscillatorPolyLoop(config, state, poly17, wave_loop_cycle[WAVE_POLY17], step);
}

namespace Random
{
	// random seed
	unsigned int gSeed = 0x92D68CA2;

	// set seed
	inline void Seed(unsigned int aSeed)
	{
		gSeed = aSeed;
	}

	// random unsigned long
	inline unsigned int Int()
	{
#if 1
		gSeed ^= (gSeed << 13);
		gSeed ^= (gSeed >> 17);
		gSeed ^= (gSeed << 5);
#else
		gSeed = 1664525L * gSeed + 1013904223L;
#endif
		return gSeed;
	}

	// random uniform float
	inline float Float()
	{
		union { float f; unsigned u; } floatint;
		floatint.u = 0x3f800000 | (Int() >> 9);
		return floatint.f - 1.0f;
	}
}

// white noise oscillator
float OscillatorNoise(OscillatorConfig const &config, OscillatorState &state, float step)
{
	return Random::Float() * 2.0f - 1.0f;
}

// map wave type enumeration to oscillator function
OscillatorFunc const oscillator[WAVE_COUNT] =
{
	OscillatorSine, OscillatorPulse, OscillatorSawtooth, OscillatorTriangle, OscillatorPoly4, OscillatorPoly5, OscillatorPoly17, OscillatorNoise
};
OscillatorLoopFunc const oscillator_loop[WAVE_COUNT] =
{
	NULL, NULL, NULL, NULL, OscillatorPoly4Loop, OscillatorPoly5Loop, OscillatorPoly17Loop, NULL
};

// update oscillator
float OscillatorState::Update(OscillatorConfig const &config, float const frequency, float const step)
{
	float const delta = config.frequency * frequency * step;

	// get oscillator value
	float const value = config.amplitude * oscillator[config.wavetype](config, *this, delta);

	// accumulate oscillator phase
	phase += delta;
	if (phase >= 1.0f)
	{
		phase -= 1.0f;
		if (oscillator_loop[config.wavetype])
			oscillator_loop[config.wavetype](config, *this, delta);
	}
	else if (phase < 0.0f)
	{
		phase += 1.0f;
		if (oscillator_loop[config.wavetype])
			oscillator_loop[config.wavetype](config, *this, delta);
	}

	return value;
}

// update envelope generator
float EnvelopeState::Update(EnvelopeConfig const &config, float const step)
{
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

void FilterState::Clear()
{
#if FILTER == FILTER_NONLINEAR_MOOG
	feedback = 0.0f;
	tune = 0.0f;
	memset(ytan, 0, sizeof(ytan));
	memset(y, 0, sizeof(y));
#else
	feedback = 0.0f;
	a1 = 0.0f; b0 = 0.0f; b1 = 0.0f;
	memset(y, 0, sizeof(y));
#endif
}

void FilterState::Setup(float cutoff, float resonance)
{
#if FILTER == FILTER_IMPROVED_MOOG

	// Based on Improved Moog Filter description
	// http://www.music.mcgill.ca/~ich/research/misc/papers/cr1071.pdf

	float const fn = FILTER_OVERSAMPLE * 0.5f * info.freq;
	float const fc = cutoff < fn ? cutoff / fn : 1.0f;
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

	float const fn = FILTER_OVERSAMPLE * 0.5f * info.freq;
	float const fc = cutoff < fn ? cutoff / fn : 1.0f;
	float const fcr = ((1.8730f * fc + 0.4955f) * fc + -0.6490f) * fc + 0.9988f;
	float const acr = (-3.9364f * fc + 1.8409f) * fc + 0.9968f;
	feedback = resonance * 4.0f * acr;
	tune = (1.0f - expf(-M_PI * fc * fcr)) * 1.22070313f;

#endif
}

float FilterState::Apply(float input)
{
#if FILTER == FILTER_IMPROVED_MOOG

#if FILTER_OVERSAMPLE > 1
	for (int i = 0; i < FILTER_OVERSAMPLE; ++i)
#endif
	{
		// feedback
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

	return y[4];

#elif FILTER == FILTER_NONLINEAR_MOOG

	// feedback
	float const in = input - feedback * y[4];

	float output;
#if FILTER_OVERSAMPLE > 1
	for (int i = 0; i < FILTER_OVERSAMPLE; ++i)
#endif
	{
		float last_stage = y[4];

		y[0] = in;
		ytan[0] = FastTanh(y[0] * 0.8192f);
		y[1] += tune * (ytan[0] - ytan[1]);
		ytan[1] = FastTanh(y[1] * 0.8192f);
		y[2] += tune * (ytan[1] - ytan[2]);
		ytan[2] = FastTanh(y[2] * 0.8192f);
		y[3] += tune * (ytan[2] - ytan[3]);
		ytan[3] = FastTanh(y[3] * 0.8192f);
		y[4] += tune * (ytan[3] - FastTanh(y[4] * 0.8192f));

		output = 0.5f * (y[4] + last_stage);
	}
	return output;

#endif
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
		lfo_state.Update(lfo_config, 1.0f, float(count) / info.freq);

		return length;
	}

	// time step per output sample
	float const step = 1.0f / info.freq;

	// for each output sample...
	for (size_t c = 0; c < count; ++c)
	{
		// get low-frequency oscillator value
		float const lfo = lfo_state.Update(lfo_config, 1.0f, step);

		// LFO wave parameter modulation
		osc_config.waveparam = osc_config.waveparam_base + osc_config.waveparam_lfo * lfo;

		// LFO frequency modulation
		osc_config.frequency = powf(2.0f, osc_config.frequency_base + osc_config.frequency_lfo * lfo) * wave_adjust_frequency[osc_config.wavetype];

		// LFO amplitude modulation
		osc_config.amplitude = osc_config.amplitude_base * powf(2.0f, osc_config.amplitude_lfo * lfo);

		// accumulated sample value
		float sample = 0.0f;

		// for each active oscillator...
		for (int i = 0; i < active; ++i)
		{
			int k = index[i];

			// key frequency (taking octave shift into account)
			float key_freq = keyboard_frequency[k] * keyboard_timescale;

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

			// update oscillator
			// (assume key follow)
			float const osc_value = osc_state[k].Update(osc_config, key_freq, step);

			float flt_value;
			if (flt_config.mode)
			{
				// compute cutoff frequency
				// (assume key follow)
				float cutoff = key_freq * powf(2, osc_config.frequency_base + flt_config.cutoff + flt_config.cutoff_lfo * lfo + flt_config.cutoff_env * flt_env_amplitude);

				// compute filter values
				flt_state[k].Setup(cutoff, flt_config.resonance);

				// accumulate the filtered oscillator value
#if 1
				flt_state[k].Apply(osc_value);
				switch (flt_config.mode)
				{
				default:
				case FilterConfig::ALLPASS:		flt_value = flt_state[k].y[0]; break;
				case FilterConfig::LOWPASS_1:	flt_value = flt_state[k].y[1]; break;
				case FilterConfig::LOWPASS_2:	flt_value = flt_state[k].y[2]; break;
				case FilterConfig::LOWPASS_3:	flt_value = flt_state[k].y[3]; break;
				case FilterConfig::LOWPASS_4:	flt_value = flt_state[k].y[4]; break;
				case FilterConfig::HIGHPASS_1:	flt_value = flt_state[k].y[0] - flt_state[k].y[4]; break;
				case FilterConfig::HIGHPASS_2:	flt_value = flt_state[k].y[0] - flt_state[k].y[3]; break;
				case FilterConfig::HIGHPASS_3:	flt_value = flt_state[k].y[0] - flt_state[k].y[2]; break;
				case FilterConfig::HIGHPASS_4:	flt_value = flt_state[k].y[0] - flt_state[k].y[1]; break;
				case FilterConfig::BANDPASS:	flt_value = 2.0f * (flt_state[k].y[4] - flt_state[k].y[2]); break;
				case FilterConfig::NOTCH:		flt_value = 0.666667f * osc_value + (flt_state[k].y[4] - flt_state[k].y[2]); break;
				}
#else
				flt_value = flt_state[k].Apply(osc_value);
#endif
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

// update a logarthmic-frequency property
void UpdateFrequencyProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
{
	float value = roundf(property * 1200.0f);
	if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		value += sign * 1;		// tiny step: 1 cent
	else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		value += sign * 10;		// small step: 10 cents
	else if (!(modifiers & (SHIFT_PRESSED)))
		value += sign * 100;	// normal step: 1 semitone
	else
		value += sign * 1200;	// large step: 1 octave
	property = Clamp(value / 1200.0f, minimum, maximum);
}

// update a linear percentage property
void UpdatePercentageProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
{
	float value = roundf(property * 256.0f);
	if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		value += sign * 1;	// tiny step: 1/256
	else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		value += sign * 4;	// small step: 4/256
	else if (!(modifiers & (SHIFT_PRESSED)))
		value += sign * 16;	// normal step: 16/256
	else
		value += sign * 64;	// large step: 64/256
	property = Clamp(value / 256.0f, minimum, maximum);
}

void MenuLFO(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 1, 12 };
	DWORD written;
	int sign;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_LFO] < 0)
			menu_item[MENU_LFO] = 2;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_LFO] > 2)
			menu_item[MENU_LFO] = 0;
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[MENU_LFO])
		{
		case 0:
			lfo_config.wavetype = Wave((lfo_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			lfo_state.phase = 0.0f;
			lfo_state.loops = 0;
			lfo_state.value = wave_start_value[lfo_config.wavetype];
			break;
		case 1:
			UpdatePercentageProperty(lfo_config.waveparam, sign, modifiers, 0, 1);
			break;
		case 2:
			UpdateFrequencyProperty(lfo_config.frequency_base, sign, modifiers, -8, 14);
			lfo_config.frequency = powf(2.0f, lfo_config.frequency_base);
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_LFO], 18, pos, &written);

	for (int i = 0; i < 3; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + 1 + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_LFO && menu_item[MENU_LFO] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d\xB3 LFO", MENU_LFO + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "%-10s", wave_name[lfo_config.wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", lfo_config.waveparam * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq: % 10.3fHz", lfo_config.frequency);
}

void MenuOSC(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 21, 12 };
	DWORD written;
	int sign;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_OSC] < 0)
			menu_item[MENU_OSC] = 6;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_OSC] > 6)
			menu_item[MENU_OSC] = 0;
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[MENU_OSC])
		{
		case 0:
			osc_config.wavetype = Wave((osc_config.wavetype + WAVE_COUNT + sign) % WAVE_COUNT);
			for (int k = 0; k < KEYS; ++k)
			{
				osc_state[k].phase = 0.0f;
				osc_state[k].loops = 0;
				osc_state[k].value = wave_start_value[osc_config.wavetype];
			}
			break;
		case 1:
			UpdatePercentageProperty(osc_config.waveparam_base, sign, modifiers, 0, 1);
			break;
		case 2:
			UpdateFrequencyProperty(osc_config.frequency_base, sign, modifiers, -10, 10);
			break;
		case 3:
			UpdatePercentageProperty(osc_config.amplitude_base, sign, modifiers, -FLT_MAX, FLT_MAX);
			break;
		case 4:
			UpdatePercentageProperty(osc_config.waveparam_lfo, sign, modifiers, -FLT_MAX, FLT_MAX);
			break;
		case 5:
			UpdateFrequencyProperty(osc_config.frequency_lfo, sign, modifiers, -10, 10);
			break;
		case 6:
			UpdatePercentageProperty(osc_config.amplitude_lfo, sign, modifiers, -FLT_MAX, FLT_MAX);
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_OSC], 18, pos, &written);

	for (int i = 0; i < 7; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + 1 + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_OSC && menu_item[MENU_OSC] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d\xB3 OSC", MENU_OSC + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "%-10s", wave_name[osc_config.wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     % 6.1f%%", osc_config.waveparam_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Frequency: % 7.2f", osc_config.frequency_base * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Amplitude: % 6.1f%%", osc_config.amplitude_base * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Width LFO: % 6.1f%%", osc_config.waveparam_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq LFO:  % 7.2f", osc_config.frequency_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Ampl LFO:  % 6.1f%%", osc_config.amplitude_lfo * 100.0f);
}

void MenuFLT(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 41, 12 };
	DWORD written;
	int sign;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_FLT] < 0)
			menu_item[MENU_FLT] = 8;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_FLT] > 8)
			menu_item[MENU_FLT] = 0;
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[MENU_FLT])
		{
		case 0:
			flt_config.mode = FilterConfig::Mode((flt_config.mode + FilterConfig::COUNT + sign) % FilterConfig::COUNT);
			break;
		case 1:
			UpdatePercentageProperty(flt_config.resonance, sign, modifiers, 0, 4);
			break;
		case 2:
			UpdateFrequencyProperty(flt_config.cutoff, sign, modifiers, -10, 10);
			break;
		case 3:
			UpdateFrequencyProperty(flt_config.cutoff_lfo, sign, modifiers, -10, 10);
			break;
		case 4:
			UpdateFrequencyProperty(flt_config.cutoff_env, sign, modifiers, -10, 10);
			break;
		case 5:
			if (key == VK_RIGHT)
				flt_env_config.attack_rate *= 2.0f;
			else
				flt_env_config.attack_rate *= 0.5f;
			break;
		case 6:
			if (key == VK_RIGHT)
				flt_env_config.decay_rate *= 2.0f;
			else
				flt_env_config.decay_rate *= 0.5f;
			break;
		case 7:
			UpdatePercentageProperty(flt_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 8:
			if (key == VK_RIGHT)
				flt_env_config.release_rate *= 2.0f;
			else
				flt_env_config.release_rate *= 0.5f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_FLT], 18, pos, &written);

	for (int i = 0; i < 9; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + 1 + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_FLT && menu_item[MENU_FLT] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d\xB3 FLT", MENU_FLT + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "%-18s", filter_name[flt_config.mode]);

	++pos.Y;
	PrintConsole(hOut, pos, "Resonance: % 7.3f", flt_config.resonance);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff:    % 7.2f", flt_config.cutoff * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff LFO:% 7.2f", flt_config.cutoff_lfo * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff ENV:% 7.2f", flt_config.cutoff_env * 12.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %5g", flt_env_config.attack_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %5g", flt_env_config.decay_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", flt_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %5g", flt_env_config.release_rate);
}

void MenuVOL(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 61, 12 };
	DWORD written;
	int sign;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_VOL] < 0)
			menu_item[MENU_VOL] = 3;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_VOL] > 3)
			menu_item[MENU_VOL] = 0;
		break;

	case VK_LEFT:
	case VK_RIGHT:
		sign = (key == VK_RIGHT) ? 1 : -1;
		switch (menu_item[MENU_VOL])
		{
		case 0:
			if (key == VK_RIGHT)
				vol_env_config.attack_rate *= 2.0f;
			else
				vol_env_config.attack_rate *= 0.5f;
			break;
		case 1:
			if (key == VK_RIGHT)
				vol_env_config.decay_rate *= 2.0f;
			else
				vol_env_config.decay_rate *= 0.5f;
			break;
		case 2:
			UpdatePercentageProperty(vol_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case 3:
			if (key == VK_RIGHT)
				vol_env_config.release_rate *= 2.0f;
			else
				vol_env_config.release_rate *= 0.5f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_VOL], 18, pos, &written);

	for (int i = 0; i < 4; ++i)
	{
		COORD p = { pos.X, SHORT(pos.Y + 1 + i) };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_VOL && menu_item[MENU_VOL] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F%d\xB3 VOL", MENU_VOL + 1);

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:   %5g", vol_env_config.attack_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:    %5g", vol_env_config.decay_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain:    %5.1f%%", vol_env_config.sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release:  %5g", vol_env_config.release_rate);
}

static void EnableEffect(int index)
{
	if (!fx[index])
	{
		// set the effect, not bothering with parameters (use defaults)
		fx[index] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + index, 0);
	}
}

static void DisableEffect(int index)
{
	if (fx[index])
	{
		BASS_ChannelRemoveFX(stream, fx[index]);
		fx[index] = 0;
	}
}

void MenuFX(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 61, 18 };
	DWORD written;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_FX] < 0)
			menu_item[MENU_FX] = ARRAY_SIZE(fx) - 1;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_FX] > ARRAY_SIZE(fx) - 1)
			menu_item[MENU_FX] = 0;
		break;

	case VK_LEFT:
		DisableEffect(menu_item[MENU_FX]);
		break;

	case VK_RIGHT:
		EnableEffect(menu_item[MENU_FX]);
		break;
	}

	PrintConsole(hOut, pos, "F%d\xB3 FX", MENU_FX + 1);
	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_FX], 18, pos, &written);

	for (int i = 0; i < ARRAY_SIZE(fx); ++i)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "%-11s    %3s", fx_name[i], fx[i] ? "ON" : "OFF");
		WORD const attrib = menu_item_attrib[menu_active == MENU_FX && menu_item[MENU_FX] == i];
		FillConsoleOutputAttribute(hOut, attrib, 15, pos, &written);
		COORD const pos2 = { pos.X + 15, pos.Y };
		FillConsoleOutputAttribute(hOut, (attrib & 0xF0) | (fx[i] ? FOREGROUND_GREEN : FOREGROUND_RED), 3, pos2, &written);
	}
}

typedef void(*MenuFunc)(HANDLE hOut, WORD key, DWORD modifiers);
MenuFunc menufunc[MENU_COUNT] =
{
	MenuLFO, MenuOSC, MenuFLT, MenuVOL, MenuFX
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
	int b0 = Max(int(freq), 0);

	// get power in each semitone band
	static float spectrum[SPECTRUM_WIDTH] = { 0 };
	int xlimit = SPECTRUM_WIDTH;
	for (int x = 0; x < SPECTRUM_WIDTH; ++x)
	{
		// get upper frequency bin for the current semitone
		freq *= semitone;
		int const b1 = Min(int(freq), FREQUENCY_BINS);

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
	int xinaudible = int(logf(20000 / freq_min) * 12 / logf(2)) - 1;

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

void UpdateOscillatorWaveformDisplay(HANDLE hOut)
{
	// show the oscillator wave shape
	// (using the lowest key frequency as reference)
	CHAR_INFO const fill = { 0, BACKGROUND_BLUE };
	CHAR_INFO const plot[2] = {
		{ 223, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE },
		{ 220, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE }
	};
	CHAR_INFO buf[21][80] = { 0 };
	COORD const pos = { 0, 0 };
	COORD const size = { 80, 21 };
	SMALL_RECT region = { 0, 50 - 21, 79, 49 };

	// local copy of oscillator config
	NoteOscillatorConfig config = osc_config;

	// get low-frequency oscillator value
	float const lfo = lfo_state.Update(lfo_config, 1.0f, 0.0f);

	// LFO wave parameter modulation
	config.waveparam = config.waveparam_base + config.waveparam_lfo * lfo;

	// LFO frequency modulation
	config.frequency = powf(2.0f, config.frequency_base + config.frequency_lfo * lfo) * wave_adjust_frequency[config.wavetype];

	// LFO amplitude modulation
	config.amplitude = config.amplitude_base * powf(2.0f, config.amplitude_lfo * lfo);

	// phase step for plot
	float const step = Min(float(wave_loop_cycle[config.wavetype]) / 80.0f, 1.0f);

	// local copy of oscillator state
	OscillatorState state = { wave_start_value[config.wavetype], 0.5f * step, 0 };

	// delta time as seen by the oscillator
	float const delta = keyboard_frequency[0] * keyboard_timescale * config.frequency * wave_adjust_frequency[config.wavetype] / info.freq;

	for (int x = 0; x < 80; ++x)
	{
		float const osc = config.amplitude * oscillator[config.wavetype](config, state, delta);
		int grid_y = int(20 - osc * 20);
		int y = grid_y >> 1;
		if (y < 0)
			y = -1;
		else if (y < 21)
			buf[y][x] = plot[grid_y & 1];
		for (y += 1; y < 21; ++y)
			buf[y][x] = fill;
		state.phase += step;
		if (state.phase >= 1.0f)
		{
			state.phase -= 1.0f;
			if (oscillator_loop[config.wavetype])
				oscillator_loop[config.wavetype](config, state, step);
		}

	}
	WriteConsoleOutput(hOut, &buf[0][0], size, pos, &region);
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
	int grid_x = Clamp(int(18.0f * lfo + 18.0f), 0, 35);
	buf[grid_x / 2].Char.AsciiChar = plot[grid_x & 1];

	// draw the gauge
	COORD const pos = { 0, 0 };
	COORD const size = { 18, 1 };
	SMALL_RECT region = { 1, 16, 18, 16 };
	WriteConsoleOutput(hOut, &buf[0], size, pos, &region);
}

void Clear(HANDLE hOut)
{
	CONSOLE_SCREEN_BUFFER_INFO bufInfo;
	COORD osc_phase = { 0, 0 };
	DWORD written;
	DWORD size;
	GetConsoleScreenBufferInfo(hOut, &bufInfo);
	size = bufInfo.dwSize.X * bufInfo.dwSize.Y;
	FillConsoleOutputCharacter(hOut, (TCHAR)' ', size, osc_phase, &written);
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, size, osc_phase, &written);
	SetConsoleCursorPosition(hOut, osc_phase);
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
						bool gate = (keyin.Event.KeyEvent.bKeyDown != 0);
						if (flt_env_state[k].gate != gate)
						{
							flt_env_state[k].gate = gate;

							if (flt_env_state[k].gate)
							{
								flt_env_state[k].state = EnvelopeState::ATTACK;
							}
							else
							{
								flt_env_state[k].state = EnvelopeState::RELEASE;
							}
						}
						if (vol_env_state[k].gate != gate)
						{
							vol_env_state[k].gate = gate;

							if (vol_env_state[k].gate)
							{
								if (vol_env_state[k].state == EnvelopeState::OFF)
								{
									// start the oscillator
									// (assume restart on key)
									osc_state[k].phase = 0;
									osc_state[k].loops = 0;

									// start the filter
									flt_state[k].Clear();
								}

								vol_env_state[k].state = EnvelopeState::ATTACK;
							}
							else
							{
								vol_env_state[k].state = EnvelopeState::RELEASE;
							}
						}
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

		// update the low-frequency oscillator dispaly
		UpdateLowFrequencyOscillatorDisplay(hOut);

		// sleep for 1/60th of second
		Sleep(16);
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
