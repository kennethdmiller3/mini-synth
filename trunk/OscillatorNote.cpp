/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Note Oscillator
*/
#include "StdAfx.h"

#include "OscillatorNote.h"
#include "Voice.h"

// note oscillator config
NoteOscillatorConfig osc_config[NUM_OSCILLATORS];
// TO DO: keyboard follow dial?
// TO DO: oscillator mixer

// note oscillator state
OscillatorState osc_state[VOICES][NUM_OSCILLATORS];

// modulate note oscillator
void NoteOscillatorConfig::Modulate(float lfo)
{
	// LFO wave wave parameter modulation
	waveparam = waveparam_base + waveparam_lfo * lfo;

	// LFO frequency modulation
	frequency = powf(2.0f, frequency_base + frequency_lfo * lfo);

	// LFO amplitude modulation
	amplitude = amplitude_base + amplitude_lfo * lfo;
}
