#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

White Noise
*/

class OscillatorConfig;
class OscillatorState;

float OscillatorNoise(OscillatorConfig const &config, OscillatorState &state, float step);
