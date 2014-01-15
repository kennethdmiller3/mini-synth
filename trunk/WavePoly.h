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

// poly4 waveform
// 4-bit linear feedback shift register noise
extern float OscillatorPoly4(OscillatorConfig const &config, OscillatorState &state, float step);

// poly5 waveform
// 5-bit linear feedback shift register noise
extern float OscillatorPoly5(OscillatorConfig const &config, OscillatorState &state, float step);

// period-93 waveform
// 15-bit linear feedback shift register noise with variant tap position
extern float OscillatorPeriod93(OscillatorConfig const &config, OscillatorState &state, float step);

// poly9 waveform
// 9-bit linear feedback shift register noise
extern float OscillatorPoly9(OscillatorConfig const &config, OscillatorState &state, float step);

// poly17 waveform
// 17-bit linear feedback shift register noise
extern float OscillatorPoly17(OscillatorConfig const &config, OscillatorState &state, float step);

// pulse wave clocked by poly5
// (what the Atari POKEY actually does with poly5)
extern float OscillatorPulsePoly5(OscillatorConfig const &config, OscillatorState &state, float step);

// poly4 clocked by poly5
extern float OscillatorPoly4Poly5(OscillatorConfig const &config, OscillatorState &state, float step);

// poly17 clocked by poly5
extern float OscillatorPoly17Poly5(OscillatorConfig const &config, OscillatorState &state, float step);
