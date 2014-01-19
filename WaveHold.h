#pragma once

// precomputed noise wavetable
extern float noise[65536];

// initialize noise wavetable
extern void InitNoise();

// sample-and-hold noise
extern float OscillatorNoiseHold(OscillatorConfig const &config, OscillatorState &state, float step);
