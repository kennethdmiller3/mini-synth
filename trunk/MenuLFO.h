#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace LFO
	{
		enum Item
		{
			LFO_TITLE,
			LFO_WAVETYPE,
			LFO_WAVEPARAM,
			LFO_FREQUENCY,
			LFO_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}
