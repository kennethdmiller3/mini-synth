#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Sawtooth Wave
*/

class OscillatorConfig;
class OscillatorState;

extern float OscillatorSawtooth(OscillatorConfig const &config, OscillatorState &state, float step);
