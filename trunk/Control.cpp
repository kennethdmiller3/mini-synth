#include "StdAfx.h"
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Controllers
*/

namespace Control
{
	// pitch wheel value
	int pitch_wheel;
	float pitch_offset;

	void SetPitchWheel(int value)
	{
		pitch_wheel = value;
		pitch_offset = float(pitch_wheel * 2) / float(0x2000 * 12);
	}

	// reset all controllers
	void ResetAll()
	{
		pitch_wheel = 0;
		pitch_offset = 0;
	}
}
