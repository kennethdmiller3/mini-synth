#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Amplifier Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace AMP
	{
		enum Item
		{
			AMP_TITLE,
			AMP_LEVEL_ENV,
			AMP_LEVEL_ENV_VEL,
			AMP_ENV_ATTACK,
			AMP_ENV_DECAY,
			AMP_ENV_SUSTAIN,
			AMP_ENV_RELEASE,
			AMP_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}
