#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter Menu
*/

#include "Menu.h"

namespace Menu
{
	class FLT : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			MODE,
			RESONANCE,
			CUTOFF_BASE,
			CUTOFF_LFO,
			CUTOFF_ENV,
			CUTOFF_ENV_VEL,
			ENV_ATTACK,
			ENV_DECAY,
			ENV_SUSTAIN,
			ENV_RELEASE,
			COUNT
		};

		// constructor
		FLT(COORD pos, const char *name, int count)
			: Menu(pos, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern FLT menu_flt;
}