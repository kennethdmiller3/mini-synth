#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Oscillator Waveform Display
*/

#include "OscillatorNote.h"
#include "Filter.h"

class DisplayOscillatorWaveform
{
public:
	void Init();
	void Update(HANDLE hOut, BASS_INFO const &info, int const v);

private:
	float UpdateOscillatorOutput(NoteOscillatorConfig const config[]);
	float UpdateWaveformStep(int oversample, NoteOscillatorConfig const config[]);

	// compute phase steps and deltas for each oscillator
	float step[NUM_OSCILLATORS];
	float delta[NUM_OSCILLATORS];

	// local oscillator state for plot
	OscillatorState state[NUM_OSCILLATORS];

	// local filter for plot
	FilterState filter;

	// detect when to reset the filter
	bool prev_active;
	int prev_v;

	// previous frame time
	DWORD prevTime;

	// leftover cycles
	float cyclesLeftOver;
};
