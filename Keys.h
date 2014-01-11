#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Key-Related Values
*/

// number of keys
// TO DO: separate "voices" from "keys"
#define KEYS 24

// note key bindings
extern WORD const keys[KEYS];

// keyboard base frequencies
extern float keyboard_frequency[KEYS];

// keyboard octave
extern int keyboard_octave;
extern float keyboard_timescale;

// most recent note key pressed
extern int keyboard_most_recent;
