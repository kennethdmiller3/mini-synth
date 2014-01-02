/*
	BASS simple synth
	Copyright (c) 2001-2012 Un4seen Developments Ltd.
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

#define FILTER_STILSON 0
#define FILTER_VARIATION_1 1
#define FILTER_VARIATION_2 2
#define FILTER FILTER_VARIATION_1

// debug output
int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
	OutputDebugStringA(buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

// formatted write console output
int PrintConsole(HANDLE out, COORD pos, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char buf[256];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
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
#define M_PI 3.14159265358979323846
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

template<typename T> static inline T Clamp(T x, T a, T b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

static inline float Saturate(float input)
{
	return 0.5f * (fabsf(input + 1.0f) - fabsf(input - 1.0f));
}


char const title_text[] = ">>> BASS SIMPLE SYNTH";
COORD const title_pos = { 0, 0 };

WORD const keys[] = {
	'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
};
COORD const key_pos[] =
{
	{ 1, 5 }, { 2, 4 }, { 3, 5 }, { 4, 4 }, { 5, 5 }, { 7, 5 }, { 8, 4 }, { 9, 5 }, { 10, 4 }, { 11, 5 }, { 12, 4 }, { 13, 5 },
	{ 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 7, 2 }, { 8, 1 }, { 9, 2 }, { 10, 1 }, { 11, 2 }, { 12, 1 }, { 13, 2 },
};
static size_t const KEYS = ARRAY_SIZE(keys);

enum MenuMode
{
	MENU_LFO,
	MENU_OSC,
	MENU_ENV,
	MENU_FLT,
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

// low frequency oscillator properties
Wave lfo_wavetype = WAVE_SINE;
float lfo_pulsewidth = 0.5f;
float lfo_frequency = 1.0f;
// TO DO: multiple LFOs?
// TO DO: LFO key sync
// TO DO: LFO temp sync?
// TO DO: LFO keyboard follow dial?

// low frequency oscillator state
float lfo_phase;
int lfo_loops;

// oscillator properties
Wave osc_wavetype = WAVE_PULSE;
float osc_pulsewidth = 0.5f;
int osc_octave = 4;
float time_scale = 1.0f;
float osc_pulsewidth_lfo = 0.0f;
float osc_frequency_lfo = 0.0f;
float osc_amplitude_lfo = 0.0f;
// TO DO: multiple oscillators
// TO DO: hard sync
// TO DO: keyboard follow dial?

// oscillator state
float osc_frequency[KEYS];
float osc_phase[KEYS];
int osc_loops[KEYS];

// envelope generator
enum EnvelopeState
{
	ENVELOPE_OFF,

	ENVELOPE_ATTACK,
	ENVELOPE_DECAY,
	ENVELOPE_SUSTAIN,
	ENVELOPE_RELEASE,

	ENVELOPE_COUNT
};
float env_attack_rate = 256.0f;
float env_decay_rate = 256.0f;
float env_sustain_level = 1.0f;
float env_release_rate = 256.0f;
bool env_gate[KEYS];
EnvelopeState env_state[KEYS];
float env_amplitude[KEYS];
float const ENV_ATTACK_BIAS = 1.0f/(1.0f-expf(-1.0f))-1.0f;	// 1x time constant
float const ENV_DECAY_BIAS = 1.0f-1.0f/(1.0f-expf(-3.0f));	// 3x time constant
// TO DO: multiple envelopes?  filter, amplifier, global

WORD const env_attrib[ENVELOPE_COUNT] =
{
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,	// ENVELOPE_OFF
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,				// ENVELOPE_ATTACK,
	BACKGROUND_RED | BACKGROUND_INTENSITY,					// ENVELOPE_DECAY,
	BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,	// ENVELOPE_SUSTAIN,
	BACKGROUND_INTENSITY,									// ENVELOPE_RELEASE,
};

// resonant lowpass filter
bool flt_enable = false;
float flt_cutoff = 0.0f;
float flt_cutoff_lfo = 0.0f;
float flt_cutoff_env = 0.0f;
float flt_resonance = 0.0f;

// filter state
struct FilterState
{
#if FILTER == FILTER_STILSON
	float output;
	float state[4];
	float p;
	float q;
#elif FILTER == FILTER_VARIATION_1
	float f, p, q;
	float b0, b1, b2, b3, b4;
#else
	float f;
	float fb;
	float scale;
	float in[4];
	float out[4];
#endif

	void Clear(void);
	void Setup(float cutoff, float resonance);
	float Apply(float input);
};
FilterState flt_state[KEYS];

// output scale factor
float output_scale = 0.5f;

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

// multiply oscillator time scale based on wave type
// - tune the pitch of short-period poly oscillators
// - raise the pitch of poly oscillators by two octaves
float const wave_time_scale[WAVE_COUNT] =
{
	1.0f, 1.0f, 1.0f, 1.0f, 4.0f * 15 / 16, 4.0f * 31 / 32, 4.0f, 1.0f
};

// restart the oscillator loop index after this many phase cycles
// - poly oscillators using the loop index to look up precomputed values
int const wave_loop_cycle[WAVE_COUNT] =
{
	1, 1, 1, 1, ARRAY_SIZE(poly4), ARRAY_SIZE(poly5), ARRAY_SIZE(poly17), 1
};

// sine oscillator
float OscillatorSine(float amplitude, float phase, int loops, float param)
{
	return amplitude * sinf(M_PI * 2.0f * phase);
}

// pulse oscillator
// - param controls pulse width
// - pulse width 0.5 is square wave
// - sum k=0..infinity sin((2*k+1)*2*pi*phase)/(2*k+1)
float OscillatorPulse(float amplitude, float phase, int loops, float param)
{
	return phase < param ? amplitude : -amplitude;
	//return amplitude * 2.0f * (ceilf(phase) - roundf(phase));
}

// sawtooth oscillator
// - sum k=1..infinity sin(k*2*pi*phase)/n
float OscillatorSawtooth(float amplitude, float phase, int loops, float param)
{
#if 1
	return amplitude * (1.0f - 2.0f * phase);
#else
	if (phase < 0.5f)
		return amplitude * 2.0f * phase;
	else
		return amplitude * 2.0f * (phase - 1.0f);
#endif
}

// triangle oscillator
// - sum k=0..infinity (-1)**k sin((2*k+1)*2*pi*phase)/(2*k+1)**2
float OscillatorTriangle(float amplitude, float phase, int loops, float param)
{
	if (phase < 0.25f)
		return amplitude * 4.0f * phase;
	else if (phase < 0.75f)
		return amplitude * 4.0f * (0.5f - phase);
	else
		return amplitude * 4.0f * (phase - 1.0f);
	//return amplitude * (4.0f * fabsf(phase + 0.25f - roundf(phase + 0.25f)) - 1.0f);
}

// poly4 oscillator
// 4-bit linear feedback shift register noise
float OscillatorPoly4(float amplitude, float phase, int loops, float param)
{
	return poly4[loops] ? amplitude : -amplitude;
}

// poly5 oscillator
// 5-bit linear feedback shift register noise
float OscillatorPoly5(float amplitude, float phase, int loops, float param)
{
	return poly5[loops] ? amplitude : -amplitude;
}

// poly17 oscillator
// 17-bit linear feedback shift register noise
float OscillatorPoly17(float amplitude, float phase, int loops, float param)
{
	return poly17[loops] ? amplitude : -amplitude;
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

	// random range value
	inline float Value(float aAverage, float aVariance)
	{
		return (2.0f * Float() - 1.0f) * aVariance + aAverage;
	}
}

// white noise oscillator
float OscillatorNoise(float amplitude, float phase, int loops, float param)
{
	return Random::Value(0.0f, 2.0f * amplitude);
	//return amplitude * 2.0f * (Random::Float() - Random::Float());
}

// map wave type enumeration to oscillator function
typedef float(*OscillatorFunc)(float amplitude, float phase, int loops, float param);
OscillatorFunc const oscillator[WAVE_COUNT] =
{
	OscillatorSine, OscillatorPulse, OscillatorSawtooth, OscillatorTriangle, OscillatorPoly4, OscillatorPoly5, OscillatorPoly17, OscillatorNoise
};

void FilterState::Clear()
{
#if FILTER == FILTER_STILSON
	output = 0.0f;
	memset(state, 0, sizeof(state));
#elif FILTER == FILTER_VARIATION_1
	f = p = q = 0.0f;
	b0 = b1 = b2 = b3 = b4 = 0.0f;
#else
	f = 0.0f;
	fb = 0.0f;
	scale = 0.0f;
	memset(in, 0, sizeof(in));
	memset(out, 0, sizeof(out));
#endif
}

#if FILTER == FILTER_STILSON
static float const gaintable[199] = {
	0.999969f, 0.990082f, 0.980347f, 0.970764f, 0.961304f, 0.951996f, 0.94281f, 0.933777f, 0.924866f, 0.916077f,
	0.90741f, 0.898865f, 0.890442f, 0.882141f, 0.873962f, 0.865906f, 0.857941f, 0.850067f, 0.842346f, 0.834686f,
	0.827148f, 0.819733f, 0.812378f, 0.805145f, 0.798004f, 0.790955f, 0.783997f, 0.77713f, 0.770355f, 0.763672f,
	0.75708f, 0.75058f, 0.744141f, 0.737793f, 0.731537f, 0.725342f, 0.719238f, 0.713196f, 0.707245f, 0.701355f,
	0.695557f, 0.689819f, 0.684174f, 0.678558f, 0.673035f, 0.667572f, 0.66217f, 0.65686f, 0.651581f, 0.646393f,
	0.641235f, 0.636169f, 0.631134f, 0.62619f, 0.621277f, 0.616425f, 0.611633f, 0.606903f, 0.602234f, 0.597626f,
	0.593048f, 0.588531f, 0.584045f, 0.579651f, 0.575287f, 0.570953f, 0.566681f, 0.562469f, 0.558289f, 0.554169f,
	0.550079f, 0.546051f, 0.542053f, 0.538116f, 0.53421f, 0.530334f, 0.52652f, 0.522736f, 0.518982f, 0.515289f,
	0.511627f, 0.507996f, 0.504425f, 0.500885f, 0.497375f, 0.493896f, 0.490448f, 0.487061f, 0.483704f, 0.480377f,
	0.477081f, 0.473816f, 0.470581f, 0.467377f, 0.464203f, 0.46109f, 0.457977f, 0.454926f, 0.451874f, 0.448883f,
	0.445892f, 0.442932f, 0.440033f, 0.437134f, 0.434265f, 0.431427f, 0.428619f, 0.425842f, 0.423096f, 0.42038f,
	0.417664f, 0.415009f, 0.412354f, 0.409729f, 0.407135f, 0.404572f, 0.402008f, 0.399506f, 0.397003f, 0.394501f,
	0.392059f, 0.389618f, 0.387207f, 0.384827f, 0.382477f, 0.380127f, 0.377808f, 0.375488f, 0.37323f, 0.370972f,
	0.368713f, 0.366516f, 0.364319f, 0.362122f, 0.359985f, 0.357849f, 0.355713f, 0.353607f, 0.351532f, 0.349457f,
	0.347412f, 0.345398f, 0.343384f, 0.34137f, 0.339417f, 0.337463f, 0.33551f, 0.333588f, 0.331665f, 0.329773f,
	0.327911f, 0.32605f, 0.324188f, 0.322357f, 0.320557f, 0.318756f, 0.316986f, 0.315216f, 0.313446f, 0.311707f,
	0.309998f, 0.308289f, 0.30658f, 0.304901f, 0.303223f, 0.301575f, 0.299927f, 0.298309f, 0.296692f, 0.295074f,
	0.293488f, 0.291931f, 0.290375f, 0.288818f, 0.287262f, 0.285736f, 0.284241f, 0.282715f, 0.28125f, 0.279755f,
	0.27829f, 0.276825f, 0.275391f, 0.273956f, 0.272552f, 0.271118f, 0.269745f, 0.268341f, 0.266968f, 0.265594f,
	0.264252f, 0.262909f, 0.261566f, 0.260223f, 0.258911f, 0.257599f, 0.256317f, 0.255035f, 0.25375f
};
#endif

void FilterState::Setup(float cutoff, float resonance)
{
#if FILTER == FILTER_STILSON
	// compute pole parameter
	float const fn = 0.5f * info.freq;
	float const fc = cutoff < fn ? cutoff / fn : 1.0f;
	p = ((-0.69346f * fc - 0.59515f) * fc + 3.2937f) * fc - 1.0072f;

#if 0
	// compute feedback parameter
	q = resonance * ((((0.0332f * p - 0.0782f) * p + 0.1443f) * p - 0.2931f) * p + 0.449f);
#else
	// compute feedback parameter
	float ix = p * 99;
	int ixint = int(floor(ix));
	float ixfrac = ix - ixint;
	q = resonance * ((1.0f - ixfrac) * gaintable[ixint + 99] + ixfrac * gaintable[ixint + 100]);
#endif
#elif FILTER == FILTER_VARIATION_1
	float fn = 0.5f * info.freq;
	float fc = cutoff < fn ? cutoff / fn : 1.0f;
	float fq = 1.0f - fc;
	p = fc + 0.8f * fc * fq;
	f = p + p - 1.0f;
	q = resonance * (1.0f + fq * (0.5f + fq * (-0.5f + 2.8f * fq)));
#else
	f = cutoff * 1.16f * 2.0f / info.freq;
	if (f > 1.0f)
		f = 1.0f;
	fb = resonance * (1.0f - 0.15f * (f * f));
	scale = 0.35013f * (f * f) * (f * f);
#endif
}

float FilterState::Apply(float input)
{
#if FILTER == FILTER_STILSON
	// feedback
	output = 0.25f * (input - output);

	// four-pole low-pass filter
	for (int pole = 0; pole < 4; ++pole)
	{
		float temp = state[pole];
		output = Saturate(output + p * (output - temp));
		state[pole] = output;
		output = Saturate(output + temp);
	}

	float const value = output;
	output *= q;
	return value;
#elif FILTER == FILTER_VARIATION_1
	float t1, t2;

	// feedback
	float const in = input - q * b4;

	// four-pole low-pass filter
	t1 = b1;
	b1 = Saturate((in + b0) * p - b1 * f);
	t2 = b2;
	b2 = Saturate((b1 + t1) * p - b2 * f);
	t1 = b3;
	b3 = Saturate((b2 + t2) * p - b2 * f);
	b4 = Saturate((b3 + t1) * p - b4 * f);
	b0 = in;

	//return b4;				// lowpass
	return in - b4;		// highpass
	//return 3 * (b3 - b4)	// bandpass
#else
	// feedback
	input = Saturate(scale * (input - out[3] * fb));

	// four-pole low-pass filter
	out[0] = Saturate(input + 0.3f * in[0] + (1.0f - f) * out[0]);
	in[0] = input;
	out[1] = Saturate(out[0] + 0.3f * in[1] + (1.0f - f) * out[1]);
	in[1] = out[0];
	out[2] = Saturate(out[1] + 0.3f * in[2] + (1.0f - f) * out[2]);
	in[2] = out[1];
	out[3] = Saturate(out[2] + 0.3f * in[3] + (1.0f - f) * out[3]);
	in[3] = out[2];

	return out[3];
#endif
}

DWORD CALLBACK WriteStream(HSTREAM handle, short *buffer, DWORD length, void *user)
{
	// get active oscillators
	int index[KEYS];
	int active = 0;
	for (int k = 0; k < ARRAY_SIZE(keys); k++)
	{
		if (env_state[k] != ENVELOPE_OFF)
		{
			index[active++] = k;
		}
	}

	// low-frequency oscillator loops after this many cycles
	float const lfo_loop_cycle = wave_loop_cycle[lfo_wavetype];

	if (active == 0)
	{
		// clear buffer
		memset(buffer, 0, length);

		// advance low-frequency oscillator
		lfo_phase += lfo_frequency * (length / 2);
		while (lfo_phase >= 1.0f)
		{
			lfo_phase -= 1.0f;
			if (++lfo_loops >= lfo_loop_cycle)
				lfo_loops = 0;
		}

		return length;
	}

	// time step per output sample
	float const step = 1.0f / info.freq;

	// adjust time step based octave time scale and wave type
	float const adjusted_step = step * time_scale * wave_time_scale[osc_wavetype];

	// oscillator loops after this many cycles
	float const osc_loop_cycle = wave_loop_cycle[osc_wavetype];

	// oscillator evaluation functions
	OscillatorFunc lfo_func = oscillator[lfo_wavetype];
	OscillatorFunc osc_func = oscillator[osc_wavetype];

	// for each output sample...
	for (int c = 0; c < length; c += 2 * sizeof(short))
	{
		// get low-frequency oscillator value
		float const lfo = lfo_func(1.0f, lfo_phase, lfo_loops, lfo_pulsewidth);

		// accumulate low-frequency oscillator phase
		// (not subject to time scale)
		lfo_phase += lfo_frequency * step;
		while (lfo_phase >= 1.0f)
		{
			lfo_phase -= 1.0f;
			if (++lfo_loops >= lfo_loop_cycle)
				lfo_loops = 0;
		}

		// LFO amplitude modulation
		float const modulate_amplitude = powf(2.0f, osc_amplitude_lfo * lfo);
			
		// LFO frequency modulation
		float const modulate_frequency = powf(2.0f, osc_frequency_lfo * lfo);

		// LFO pulse width modulation
		float pulsewidth = osc_pulsewidth + osc_pulsewidth_lfo * lfo;

		// modulated time scale
		float const modulated_step = adjusted_step * modulate_frequency;

		// accumulated sample value
		float sample = 0.0f;

		// for each active oscillator...
		for (int i = 0; i < active; ++i)
		{
			int k = index[i];

			// update envelope generator
			float env_target;
			switch (env_state[k])
			{
			case ENVELOPE_ATTACK:
				env_target = 1.0f + ENV_ATTACK_BIAS;
				env_amplitude[k] += (env_target - env_amplitude[k]) * env_attack_rate * step;
				if (env_amplitude[k] >= 1.0f)
				{
					env_amplitude[k] = 1.0f;
					if (env_sustain_level < 1.0f)
						env_state[k] = ENVELOPE_DECAY;
					else
						env_state[k] = ENVELOPE_SUSTAIN;
				}
				break;

			case ENVELOPE_DECAY:
				env_target = env_sustain_level + (1.0f - env_sustain_level) * ENV_DECAY_BIAS;
				env_amplitude[k] += (env_target - env_amplitude[k]) * env_decay_rate * step;
				if (env_amplitude[k] <= env_sustain_level)
				{
					env_amplitude[k] = env_sustain_level;
					env_state[k] = ENVELOPE_SUSTAIN;
				}
				break;

			case ENVELOPE_RELEASE:
				env_target = ENV_DECAY_BIAS;
				if (env_amplitude[k] < env_sustain_level || env_decay_rate < env_release_rate)
					env_amplitude[k] += (env_target - env_amplitude[k]) * env_release_rate * step;
				else
					env_amplitude[k] += (env_target - env_amplitude[k]) * env_decay_rate * step;
				if (env_amplitude[k] <= 0.0f)
				{
					env_amplitude[k] = 0.0f;
					env_state[k] = ENVELOPE_OFF;

					// clear filter state
					flt_state[k].Clear();

					// remove from active oscillators
					--active;
					index[i] = index[active];
					--i;
					continue;
				}
				break;
			}

			// get oscillator value
			float const osc = osc_func(env_amplitude[k] * modulate_amplitude, osc_phase[k], osc_loops[k], pulsewidth);

			// accumulate oscillator phase
			osc_phase[k] += osc_frequency[k] * modulated_step;
			while (osc_phase[k] >= 1.0f)
			{
				osc_phase[k] -= 1.0f;
				if (++osc_loops[k] >= osc_loop_cycle)
					osc_loops[k] = 0;
			}

			if (flt_enable)
			{
				// cutoff frequency
				float cutoff = osc_frequency[k] * time_scale * powf(2, (flt_cutoff + flt_cutoff_lfo * lfo + flt_cutoff_env * env_amplitude[k]) / 12.0f);

				// compute filter values
				flt_state[k].Setup(cutoff, flt_resonance);

				// accumulate the filtered oscillator value
				sample += flt_state[k].Apply(osc);
			}
			else
			{
				// accumulate unfiltered oscillator
				sample += osc;
			}
		}

		// left and right channels are the same
		//short output = (short)Clamp(sample * output_scale * 32768, SHRT_MIN, SHRT_MAX);
		short output = (short)(tanhf(sample * output_scale) * 32768);
		*buffer++ = output;
		*buffer++ = output;
	}

	return length;
}

void PrintBufferSize(HANDLE hOut, DWORD buflen)
{
	COORD pos = { 1, 7 };
	PrintConsole(hOut, pos, "- + Buffer: %3dms", buflen);
}

void PrintOutputScale(HANDLE hOut)
{
	COORD pos = { 1, 8 };
	PrintConsole(hOut, pos, "    Output: %5.1f%%", output_scale * 100.0f);
}

void MenuLFO(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 1, 12 };
	DWORD written;

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
		switch (menu_item[MENU_LFO])
		{
		case 0:
			if (lfo_wavetype > 0)
			{
				lfo_wavetype = Wave(lfo_wavetype - 1);
				lfo_phase = 0.0f;
				lfo_loops = 0;
			}
			break;
		case 1:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				lfo_pulsewidth -= 16.0f / 256.0f;
			else
				lfo_pulsewidth -= 1.0f / 256.0f;
			if (lfo_pulsewidth < 0.0f)
				lfo_pulsewidth = 0.0f;
			break;
		case 2:
			lfo_frequency *= 0.5f;
			break;
		}
		break;
	case VK_RIGHT:
		switch (menu_item[MENU_LFO])
		{
		case 0:
			if (lfo_wavetype < WAVE_COUNT - 1)
			{
				lfo_wavetype = Wave(lfo_wavetype + 1);
				lfo_phase = 0.0f;
				lfo_loops = 0;
			}
			break;
		case 1:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				lfo_pulsewidth += 16.0f / 256.0f;
			else
				lfo_pulsewidth += 1.0f / 256.0f;
			if (lfo_pulsewidth > 1.0f)
				lfo_pulsewidth = 1.0f;
			break;
		case 2:
			lfo_frequency *= 2.0f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_LFO], 18, pos, &written);

	for (int i = 0; i < 3; ++i)
	{
		COORD p = { pos.X, pos.Y + 1 + i };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_LFO && menu_item[MENU_LFO] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F1 | LFO");

	++pos.Y;
	PrintConsole(hOut, pos, "%-10s", wave_name[lfo_wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width: %5.1f%%", lfo_pulsewidth * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq:   %4gHz", lfo_frequency);
}

void MenuOSC(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 21, 12 };
	DWORD written;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_OSC] < 0)
			menu_item[MENU_OSC] = 5;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_OSC] > 5)
			menu_item[MENU_OSC] = 0;
		break;

	case VK_LEFT:
		switch (menu_item[MENU_OSC])
		{
		case 0:
			if (osc_wavetype > 0)
			{
				osc_wavetype = Wave(osc_wavetype - 1);
				memset(osc_phase, 0, KEYS * sizeof(osc_phase[0]));
				memset(osc_loops, 0, KEYS * sizeof(osc_loops[0]));
			}
			break;
		case 1:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_pulsewidth -= 16.0f / 256.0f;
			else
				osc_pulsewidth -= 1.0f / 256.0f;
			if (osc_pulsewidth < 0.0f)
				osc_pulsewidth = 0.0f;
			break;
		case 2:
			if (osc_octave > 0)
			{
				--osc_octave;
				time_scale *= 0.5f;
			}
			break;
		case 3:
			if (modifiers & (SHIFT_PRESSED))
				osc_frequency_lfo -= 100.0f / 1200.0f;
			else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_frequency_lfo -= 10.0f / 1200.0f;
			else
				osc_frequency_lfo -= 1.0f / 1200.0f;
			if (osc_frequency_lfo < 0.0f)
				osc_frequency_lfo = 0.0f;
			break;
		case 4:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_amplitude_lfo -= 16.0f / 256.0f;
			else
				osc_amplitude_lfo -= 1.0f / 256.0f;
			if (osc_amplitude_lfo < 0.0f)
				osc_amplitude_lfo = 0.0f;
			break;
		case 5:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_pulsewidth_lfo -= 16.0f / 256.0f;
			else
				osc_pulsewidth_lfo -= 1.0f / 256.0f;
			if (osc_pulsewidth_lfo < 0.0f)
				osc_pulsewidth_lfo = 0.0f;
		}
		break;
	case VK_RIGHT:
		switch (menu_item[MENU_OSC])
		{
		case 0:
			if (osc_wavetype < WAVE_COUNT - 1)
			{
				osc_wavetype = Wave(osc_wavetype + 1);
				memset(osc_phase, 0, KEYS * sizeof(osc_phase[0]));
				memset(osc_loops, 0, KEYS * sizeof(osc_loops[0]));
			}
			break;
		case 1:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_pulsewidth += 16.0f / 256.0f;
			else
				osc_pulsewidth += 1.0f / 256.0f;
			if (osc_pulsewidth > 1.0f)
				osc_pulsewidth = 1.0f;
			break;
		case 2:
			if (osc_octave < 9)
			{
				++osc_octave;
				time_scale *= 2.0f;
			}
			break;
		case 3:
			if (modifiers & (SHIFT_PRESSED))
				osc_frequency_lfo += 100.0f / 1200.0f;
			else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_frequency_lfo += 10.0f / 1200.0f;
			else
				osc_frequency_lfo += 1.0f / 1200.0f;
			break;
		case 4:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_amplitude_lfo += 16.0f / 256.0f;
			else
				osc_amplitude_lfo += 1.0f / 256.0f;
			if (osc_amplitude_lfo > 1.0f)
				osc_amplitude_lfo = 1.0f;
			break;
		case 5:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				osc_pulsewidth_lfo += 16.0f / 256.0f;
			else
				osc_pulsewidth_lfo += 1.0f / 256.0f;
			if (osc_pulsewidth_lfo > 1.0f)
				osc_pulsewidth_lfo = 1.0f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_OSC], 18, pos, &written);

	for (int i = 0; i < 6; ++i)
	{
		COORD p = { pos.X, pos.Y + 1 + i };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_OSC && menu_item[MENU_OSC] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F2 | OSC");

	++pos.Y;
	PrintConsole(hOut, pos, "%-10s", wave_name[osc_wavetype]);

	++pos.Y;
	PrintConsole(hOut, pos, "Width:     %5.1f%%", osc_pulsewidth * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Octave:        %1d", osc_octave);

	++pos.Y;
	PrintConsole(hOut, pos, "Freq LFO:   %4g", roundf(osc_frequency_lfo * 1200.0f));

	++pos.Y;
	PrintConsole(hOut, pos, "Amp LFO:   %5.1f%%", osc_amplitude_lfo * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Width LFO: %5.1f%%", osc_pulsewidth_lfo * 100.0f);
}

void MenuENV(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 41, 12 };
	DWORD written;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_ENV] < 0)
			menu_item[MENU_ENV] = 3;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_ENV] > 3)
			menu_item[MENU_ENV] = 0;
		break;

	case VK_LEFT:
		switch (menu_item[MENU_ENV])
		{
		case 0:
			env_attack_rate *= 0.5f;
			break;
		case 1:
			env_decay_rate *= 0.5f;
			break;
		case 2:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				env_sustain_level -= 16.0f / 256.0f;
			else
				env_sustain_level -= 1.0f / 256.0f;
			if (env_sustain_level < 0.0f)
				env_sustain_level = 0.0f;
			break;
		case 3:
			env_release_rate *= 0.5f;
			break;
		}
		break;

	case VK_RIGHT:
		switch (menu_item[MENU_ENV])
		{
		case 0:
			env_attack_rate *= 2.0f;
			break;
		case 1:
			env_decay_rate *= 2.0f;
			break;
		case 2:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				env_sustain_level += 16.0f / 256.0f;
			else
				env_sustain_level += 1.0f / 256.0f;
			if (env_sustain_level > 1.0f)
				env_sustain_level = 1.0f;
			break;
		case 3:
			env_release_rate *= 2.0f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_ENV], 18, pos, &written);

	for (int i = 0; i < 4; ++i)
	{
		COORD p = { pos.X, pos.Y + 1 + i };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_ENV && menu_item[MENU_ENV] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F3 | ENV");

	++pos.Y;
	PrintConsole(hOut, pos, "Attack:  %5g", env_attack_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Decay:   %5g", env_decay_rate);

	++pos.Y;
	PrintConsole(hOut, pos, "Sustain: %5.1f%%", env_sustain_level * 100.0f);

	++pos.Y;
	PrintConsole(hOut, pos, "Release: %5g", env_release_rate);
}

void MenuFLT(HANDLE hOut, WORD key, DWORD modifiers)
{
	COORD pos = { 61, 12 };
	DWORD written;

	switch (key)
	{
	case VK_UP:
		if (--menu_item[MENU_FLT] < 0)
			menu_item[MENU_FLT] = 4;
		break;

	case VK_DOWN:
		if (++menu_item[MENU_FLT] > 4)
			menu_item[MENU_FLT] = 0;
		break;

	case VK_LEFT:
		switch (menu_item[MENU_FLT])
		{
		case 0:
			flt_enable = false;
			break;
		case 1:
			if (modifiers & (SHIFT_PRESSED))
				flt_resonance -= 256.0f / 256.0f;
			else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_resonance -= 16.0f / 256.0f;
			else
				flt_resonance -= 1.0f / 256.0f;
			if (flt_resonance < 0.0f)
				flt_resonance = 0.0f;
			break;
		case 2:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff -= 12.0f;
			else
				flt_cutoff -= 1.0f;
			break;
		case 3:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff_lfo -= 12.0f;
			else
				flt_cutoff_lfo -= 1.0f;
			break;
		case 4:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff_env -= 12.0f;
			else
				flt_cutoff_env -= 1.0f;
			break;
		}
		break;

	case VK_RIGHT:
		switch (menu_item[MENU_FLT])
		{
		case 0:
			flt_enable = true;
			break;
		case 1:
			if (modifiers & SHIFT_PRESSED)
				flt_resonance += 256.f / 256.0f;
			else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_resonance += 16.0f / 256.0f;
			else
				flt_resonance += 1.0f / 256.0f;
			if (flt_resonance > 10.0f)
				flt_resonance = 10.0f;
			break;
		case 2:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff += 12.0f;
			else
				flt_cutoff += 1.0f;
			break;
		case 3:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff_lfo += 12.0f;
			else
				flt_cutoff_lfo += 1.0f;
			break;
		case 4:
			if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
				flt_cutoff_env += 12.0f;
			else
				flt_cutoff_env += 1.0f;
			break;
		}
		break;
	}

	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_FLT], 18, pos, &written);

	for (int i = 0; i < 5; ++i)
	{
		COORD p = { pos.X, pos.Y + 1 + i };
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_FLT && menu_item[MENU_FLT] == i], 18, p, &written);
	}

	PrintConsole(hOut, pos, "F4 | FLT");

	++pos.Y;
	PrintConsole(hOut, pos, "Filter:       %3s", flt_enable ? "ON" : "off");

	++pos.Y;
	PrintConsole(hOut, pos, "Resonance:  %5.3f", flt_resonance);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff:     %5.0f", flt_cutoff);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff LFO: %5.0f", flt_cutoff_lfo);

	++pos.Y;
	PrintConsole(hOut, pos, "Cutoff ENV: %5.0f", flt_cutoff_env);
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
	char buf[64];
	COORD pos = { 41, 1 };
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

	PrintConsole(hOut, pos, "F5 | FX");
	FillConsoleOutputAttribute(hOut, menu_title_attrib[menu_active == MENU_FX], 18, pos, &written);

	for (int i = 0; i < ARRAY_SIZE(fx); ++i)
	{
		++pos.Y;
		PrintConsole(hOut, pos, "%-11s %-3s", fx_name[i], fx[i] ? "ON" : "off");
		FillConsoleOutputAttribute(hOut, menu_item_attrib[menu_active == MENU_FX && menu_item[MENU_FX] == i], 18, pos, &written);
	}
}

typedef void(*MenuFunc)(HANDLE hOut, WORD key, DWORD modifiers);
MenuFunc menufunc[MENU_COUNT] =
{
	MenuLFO, MenuOSC, MenuENV, MenuFLT, MenuFX
};

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
	INPUT_RECORD keyin;
	DWORD r, buflen;
	int k;
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

#if 0
	// set the console buffer size
	static const COORD bufferSize = { 80, 25 };
	SetConsoleScreenBufferSize(hOut, bufferSize);

	// set the console window size
	static const SMALL_RECT windowSize = { 0, 0, 79, 24 };
	SetConsoleWindowInfo(hOut, TRUE, &windowSize);
#endif

	// clear the window
	Clear(hOut);

	// hide the cursor
	static const CONSOLE_CURSOR_INFO cursorInfo = { 100, FALSE };
	SetConsoleCursorInfo(hOut, &cursorInfo);

	// set input mode
	SetConsoleMode(hIn, 0);

	// 10ms update period
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);

	// setup output - get latency
	//if (!BASS_Init(-1, 44100, BASS_DEVICE_LATENCY, 0, NULL))
	if (!BASS_Init(-1, 48000, BASS_DEVICE_LATENCY, 0, NULL))
		Error("Can't initialize device");

	BASS_GetInfo(&info);
	// default buffer size = update period + 'minbuf' + 1ms extra margin
	BASS_SetConfig(BASS_CONFIG_BUFFER, 10 + info.minbuf + 1);
	buflen = BASS_GetConfig(BASS_CONFIG_BUFFER);
	// if the device's output rate is unknown default to 44100 Hz
	if (!info.freq) info.freq = 44100;
	// create a stream, stereo so that effects sound nice
	stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);

	// initialize polynomial noise tables
	InitPoly(poly4, 4, 1, 0xF, 0);
	InitPoly(poly5, 5, 2, 0x1F, 1);
	InitPoly(poly17, 17, 5, 0x1FFFF, 0);

	// compute phase advance per sample per key
	for (k = 0; k < KEYS; ++k)
	{
		osc_frequency[k] = pow(2.0, (k + 3) / 12.0) * 220.0;	// middle C = 261.626Hz; A above middle C = 440Hz
		//osc_frequency[k] = pow(2.0, k / 12.0) * 256.0f;		// middle C = 256Hz
	}

	DebugPrint("device latency: %dms\n", info.latency);
	DebugPrint("device minbuf: %dms\n", info.minbuf);
	DebugPrint("ds version: %d (effects %s)\n", info.dsver, info.dsver < 8 ? "disabled" : "enabled");

	// show the note keys
	for (k = 0; k < KEYS; ++k)
	{
		char buf[] = { keys[k], '\0' };
		WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		DWORD written;
		WriteConsoleOutputAttribute(hOut, &attrib, 1, key_pos[k], &written);
		WriteConsoleOutputCharacter(hOut, buf, 1, key_pos[k], &written);
	}

	PrintBufferSize(hOut, buflen);
	PrintOutputScale(hOut);

	MenuLFO(hOut, 0, 0);
	MenuOSC(hOut, 0, 0);
	MenuENV(hOut, 0, 0);
	MenuFLT(hOut, 0, 0);
	if (info.dsver >= 8)
		MenuFX(hOut, 0, 0);

	/*
	printf(
		"press -/+ to de/increase the buffer\n"
		"press [/] to de/increase the octave\n"
		"press ,/. to switch the waveform\n"
		"press escape to quit\n\n");
	if (info.dsver >= 8) // DX8 effects available
		printf("press F1-F9 to toggle effects\n\n");
	printf("using a %dms buffer\r", buflen);
	*/

	BASS_ChannelPlay(stream, FALSE);

	EnvelopeState env_display[KEYS] = { ENVELOPE_OFF };

	while (running)
	{
		// if there are any pending input events
		DWORD numEvents = 0;
		while (GetNumberOfConsoleInputEvents(hIn, &numEvents) && numEvents > 0)
		{
			INPUT_RECORD keyin;
			ReadConsoleInput(hIn, &keyin, 1, &numEvents);
			if (keyin.EventType == KEY_EVENT)
			{
				if (keyin.Event.KeyEvent.bKeyDown)
				{
					WORD code = keyin.Event.KeyEvent.wVirtualKeyCode;
					DWORD modifiers = keyin.Event.KeyEvent.dwControlKeyState;
					if (code == VK_ESCAPE)
					{
						running = 0;
						break;
					}
					else if (code == VK_F11)
					{
						output_scale -= 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_F12)
					{
						output_scale += 1.0f / 16.0f;
						PrintOutputScale(hOut);
					}
					else if (code == VK_SUBTRACT || code == VK_ADD)
					{
						// recreate stream with smaller/larger buffer
						BASS_StreamFree(stream);
						if (code == VK_SUBTRACT)
							BASS_SetConfig(BASS_CONFIG_BUFFER, buflen - 1); // smaller buffer
						else
							BASS_SetConfig(BASS_CONFIG_BUFFER, buflen + 1); // larger buffer
						buflen = BASS_GetConfig(BASS_CONFIG_BUFFER);
						PrintBufferSize(hOut, buflen);
						stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);
						// set effects on the new stream
						for (r = 0; r < 9; r++) if (fx[r]) fx[r] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + r, 0);
						BASS_ChannelPlay(stream, FALSE);
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

				for (k = 0; k < KEYS; k++)
				{
					if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[k])
					{
						if (env_gate[k] != bool(keyin.Event.KeyEvent.bKeyDown))
						{
							env_gate[k] = bool(keyin.Event.KeyEvent.bKeyDown);

							if (env_gate[k])
							{
								if (env_state[k] == ENVELOPE_OFF)
								{
									osc_phase[k] = 0;
									osc_loops[k] = 0;
								}

								env_state[k] = ENVELOPE_ATTACK;

								WORD attrib = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
								DWORD written;
								WriteConsoleOutputAttribute(hOut, &attrib, 1, key_pos[k], &written);
							}
							else
							{
								env_state[k] = ENVELOPE_RELEASE;

								WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
								DWORD written;
								WriteConsoleOutputAttribute(hOut, &attrib, 1, key_pos[k], &written);
							}
						}
						break;
					}
				}
			}
		}

		// update envelope display
		for (k = 0; k < KEYS; k++)
		{
			if (env_display[k] != env_state[k])
			{
				env_display[k] = env_state[k];
				DWORD written;
				WriteConsoleOutputAttribute(hOut, &env_attrib[env_display[k]], 1, key_pos[k], &written);
			}
		}
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
