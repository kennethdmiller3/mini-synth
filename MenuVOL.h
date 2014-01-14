#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Volume Envelope Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace VOL
	{
		enum Item
		{
			VOL_TITLE,
			VOL_ENV_ATTACK,
			VOL_ENV_DECAY,
			VOL_ENV_SUSTAIN,
			VOL_ENV_RELEASE,
			VOL_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}
