#pragma once

#include "Menu.h"

namespace Menu
{
	class Reverb : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			GAIN,
			REVERB_MIX,
			DECAY_TIME,
			DECAY_HF_RATIO,
			COUNT
		};

		// constructor
		Reverb(COORD pos, const char *name, int count)
			: Menu(pos, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern Reverb menu_fx_reverb;
}
