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
			DRIVE,
			COMPENSATION,
			RESONANCE,
			CUTOFF_BASE,
			CUTOFF_LFO,
			CUTOFF_ENV,
			CUTOFF_ENV_VEL,
			KEY_FOLLOW,
			ENV_ATTACK,
			ENV_DECAY,
			ENV_SUSTAIN,
			ENV_RELEASE,
			COUNT
		};

		// constructor
		FLT(SMALL_RECT rect, const char *name, int count)
			: Menu(rect, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern FLT menu_flt;
}