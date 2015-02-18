#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Key-Related Values
*/

// number of keys
// TO DO: separate "voices" from "keys"
#define KEYS 29

// note key bindings
extern WORD const keys[KEYS];

// note keys held
extern bool key_down[KEYS];

// keyboard octave shift
extern int keyboard_octave;
