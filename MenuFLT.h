#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace FLT
	{
		enum Item
		{
			FLT_TITLE,
			FLT_MODE,
			FLT_RESONANCE,
			FLT_CUTOFF_BASE,
			FLT_CUTOFF_LFO,
			FLT_CUTOFF_ENV,
			FLT_CUTOFF_ENV_VEL,
			FLT_ENV_ATTACK,
			FLT_ENV_DECAY,
			FLT_ENV_SUSTAIN,
			FLT_ENV_RELEASE,
			FLT_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}