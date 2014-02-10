#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Controllers
*/

namespace Control
{
	// pitch wheel value
	extern int pitch_wheel;
	extern float pitch_offset;

	// set pitch wheel value
	extern void SetPitchWheel(int value);

	// reset all controllers
	extern void ResetAll();
}
