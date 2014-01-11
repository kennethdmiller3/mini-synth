#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Pulse Wave
*/

class OscillatorConfig;
class OscillatorState;

extern float OscillatorPulse(OscillatorConfig const &config, OscillatorState &state, float step);
