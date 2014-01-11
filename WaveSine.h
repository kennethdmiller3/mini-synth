#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sine Wave
*/

class OscillatorConfig;
class OscillatorState;

// sine wave
float OscillatorSine(OscillatorConfig const &config, OscillatorState &state, float step);
