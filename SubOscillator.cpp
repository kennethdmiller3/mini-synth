/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sub-Oscillator
*/
#include "StdAfx.h"

#include "SubOscillator.h"
#include "OscillatorNote.h"
#include "PolyBLEP.h"

// sub-oscillator mode names
const char * const sub_osc_name[SUBOSC_COUNT] =
{
	"None",
	"\xDF\xDC",
	"\xDF\xDF\xDC\xDC",
	"\xDF\xDC\xDC\xDC",
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
