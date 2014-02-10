#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Note Oscillator Menu
*/

#include "Menu.h"

namespace Menu
{
	class OSC : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			WAVETYPE,
			WAVEPARAM_BASE,
			FREQUENCY_BASE,
			AMPLITUDE_BASE,
			WAVEPARAM_LFO,
			FREQUENCY_LFO,
			AMPLITUDE_LFO,
			KEY_FOLLOW,
			SUB_OSC_MODE,
			SUB_OSC_AMPLITUDE,
			HARD_SYNC,
			COUNT
		};

		int osc;	// oscillator index

		// constructor
		OSC(int osc, COORD pos, const char *name, int count)
			: Menu(pos, name, count)
			, osc(osc)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern OSC menu_osc[];
}
