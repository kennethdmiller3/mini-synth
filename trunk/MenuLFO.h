#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Low-Frequency Oscillator Menu
*/

#include "Menu.h"

namespace Menu
{
	class LFO : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			WAVETYPE,
			WAVEPARAM,
			FREQUENCY,
			COUNT
		};

		// constructor
		LFO(COORD pos, const char *name, int count)
			: Menu(pos, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern LFO menu_lfo;
}
