#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Note Oscillator
*/

#include "Oscillator.h"
#include "SubOscillator.h"
#include "Wave.h"

// oscillators per voice
#define NUM_OSCILLATORS 2

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

extern NoteOscillatorConfig osc_config[NUM_OSCILLATORS];
// TO DO: keyboard follow dial?
// TO DO: oscillator mixer

// note oscillator state
extern OscillatorState osc_state[][NUM_OSCILLATORS];
