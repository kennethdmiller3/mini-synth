#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Amplifier Menu
*/

#include "Menu.h"

namespace Menu
{
	class AMP : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			LEVEL_ENV,
			LEVEL_ENV_VEL,
			ENV_ATTACK,
			ENV_DECAY,
			ENV_SUSTAIN,
			ENV_RELEASE,
			COUNT
		};

		// constructor
		AMP(SMALL_RECT rect, const char *name, int count)
			: Menu(rect, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern AMP menu_amp;
}
