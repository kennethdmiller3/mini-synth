#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Triangle Wave
*/

class OscillatorConfig;
class OscillatorState;

extern float OscillatorTriangle(OscillatorConfig const &config, OscillatorState &state, float step);
