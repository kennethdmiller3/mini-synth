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

// note keys held
bool key_down[KEYS];

// keyboard octave
int keyboard_octave = 4;
