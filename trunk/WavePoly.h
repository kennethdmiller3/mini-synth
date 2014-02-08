#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Poly-Noise Wave
*/

class OscillatorConfig;
class OscillatorState;

// precomputed linear feedback shift register output
extern char poly4[(1 << 4) - 1];
extern char poly5[(1 << 5) - 1];
extern char period93[93];
extern char poly9[(1 << 9) - 1];
extern char poly17[(1 << 17) - 1];

// precomputed wave tables for poly5-clocked waveforms
extern char pulsepoly5[ARRAY_SIZE(poly5) * 2];
extern char poly4poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly4)];
extern char poly17poly5[ARRAY_SIZE(poly5) * ARRAY_SIZE(poly17)];

// initialize polynomial noise tables
extern void InitPoly();

// poly waveform
extern float OscillatorPoly(OscillatorConfig const &config, OscillatorState &state, float step);
