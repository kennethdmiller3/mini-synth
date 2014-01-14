#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Effects Menu
*/

#include "Menu.h"

namespace Menu
{
	namespace FX
	{
		enum Item
		{
			FX_TITLE,
			FX_CHORUS,
			FX_COMPRESSOR,
			FX_DISTORTION,
			FX_ECHO,
			FX_FLANGER,
			FX_GARGLE,
			FX_I3DL2REVERB,
			FX_PARAMEQ,
			FX_REVERB,
			FX_COUNT
		};

		extern void Update(MenuMode menu, int index, int sign, DWORD modifiers);
		extern void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
	}
}
