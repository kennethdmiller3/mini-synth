#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Effects Menu
*/

#include "Menu.h"

namespace Menu
{
#if 0
	class FX : public Menu
	{
		enum Item
		{
			TITLE,
			CHORUS,
			COMPRESSOR,
			DISTORTION,
			ECHO,
			FLANGER,
			GARGLE,
			I3DL2REVERB,
			PARAMEQ,
			REVERB,
			COUNT
		};

		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	}
#endif
}
