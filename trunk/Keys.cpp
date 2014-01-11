/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Key-Related Values
*/
#include "StdAfx.h"

#include "Keys.h"

// note keys
// these pull double-duty as key bindings and display values
WORD const keys[KEYS] =
{
	'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
};

// keyboard base frequencies
float keyboard_frequency[KEYS];

// keyboard octave
int keyboard_octave = 4;
float keyboard_timescale = 1.0f;

// most recent note key pressed
int keyboard_most_recent = 0;
