/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator
*/
#include "StdAfx.h"

#include "OscillatorLFO.h"

LFOOscillatorConfig lfo_config;
// TO DO: multiple LFOs?
// TO DO: LFO key sync
// TO DO: LFO temp sync?
// TO DO: LFO keyboard follow dial?

// low-frequency oscillator state
OscillatorState lfo_state;
