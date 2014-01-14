#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Note Oscillator Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace OSC
	{
		enum Item
		{
			OSC_TITLE,
			OSC_WAVETYPE,
			OSC_WAVEPARAM_BASE,
			OSC_FREQUENCY_BASE,
			OSC_AMPLITUDE_BASE,
			OSC_WAVEPARAM_LFO,
			OSC_FREQUENCY_LFO,
			OSC_AMPLITUDE_LFO,
			OSC_SUB_OSC_MODE,
			OSC_SUB_OSC_AMPLITUDE,
			OSC_HARD_SYNC,
			OSC_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}
