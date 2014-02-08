#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Wave Types
*/

// waveform antialiasing
#define ANTIALIAS_NONE 0
#define ANTIALIAS_POLYBLEP 1
#define ANTIALIAS 1
extern bool use_antialias;

// oscillator wave types
enum Wave
{
	WAVE_SINE,
	WAVE_PULSE,
	WAVE_SAWTOOTH,
	WAVE_TRIANGLE,
	WAVE_NOISE,
	WAVE_NOISE_HOLD,
	WAVE_NOISE_SLOPE,

	WAVE_POLY4,			// Atari POKEY AUDC 12
	WAVE_POLY5,			// Atari TIA AUDC 7 or 9
	WAVE_PERIOD93,		// NES Ricoh 2A03 period-93 noise
	WAVE_POLY9,			// Atari TIA AUDC 8
	WAVE_POLY17,		// Atari POKEY AUDC 8
	WAVE_PULSE_POLY5,	// Atari POKEY AUDC 2, 6
	WAVE_POLY4_POLY5,	// Atari POKEY AUDC 4
	WAVE_POLY17_POLY5,	// Atari POKEY AUDC 0

	WAVE_COUNT
};

class OscillatorConfig;
class OscillatorState;

// wave evaluation function: returns wave value
typedef float(*WaveEvaluate)(OscillatorConfig const &config, OscillatorState &state, float step);

// map wave type to wave evaluator
extern WaveEvaluate const wave_evaluate[WAVE_COUNT];

// names for wave types
extern char const * const wave_name[WAVE_COUNT];

// multiply oscillator time scale based on wave type
extern float const wave_adjust_frequency[WAVE_COUNT];

// restart the oscillator loop index after this many phase cycles
extern int const wave_loop_cycle[WAVE_COUNT];

// initialize wave
extern void InitWave();
