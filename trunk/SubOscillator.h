#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sub-Oscillator
*/

class NoteOscillatorConfig;
class OscillatorState;

// sub-oscillator modes
enum SubOscillatorMode
{
	SUBOSC_NONE,
	SUBOSC_SQUARE_1OCT,
	SUBOSC_SQUARE_2OCT,
	SUBOSC_PULSE_2OCT,
	SUBOSC_COUNT
};

// sub-oscillator mode names
extern const char * const sub_osc_name[SUBOSC_COUNT];

// compute sub-oscillator value
extern float SubOscillator(NoteOscillatorConfig const &config, OscillatorState const &state, float frequency, float step);
