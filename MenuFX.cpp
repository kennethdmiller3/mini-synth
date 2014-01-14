/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Effects Menu
*/
#include "StdAfx.h"

#include "MenuFX.h"
#include "Console.h"
#include "Effect.h"

namespace Menu
{
	namespace FX
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			if (index == FX_TITLE)
			{
				// set master switch
				fx_enable = sign > 0;
				for (int i = 0; i < ARRAY_SIZE(fx); ++i)
					EnableEffect(i, fx_enable && fx_active[i]);
			}
			else
			{
				fx_active[index - 1] = sign > 0;
				EnableEffect(index - 1, fx_enable && fx_active[index - 1]);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			if (index == FX_TITLE)
			{
				PrintMenuTitle(menu, hOut, pos, fx_enable, flags, " ON", "OFF");
			}
			else
			{
				WORD const attrib = item_attrib[flags];
				PrintConsoleWithAttribute(hOut, pos, attrib, "%-15s", fx_name[index - 1]);
				if (fx_active[index - 1])
					PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, (attrib & 0xF8) | FOREGROUND_GREEN, " ON");
				else
					PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, (attrib & 0xF8) | FOREGROUND_RED, "OFF");
			}
		}
	}
}
