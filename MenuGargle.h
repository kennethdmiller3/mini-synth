#pragma once

#include "Menu.h"

namespace Menu
{
	class Gargle : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			FREQUENCY,
			WAVEFORM,
			COUNT
		};

		// constructor
		Gargle(COORD pos, const char *name, int count)
			: Menu(pos, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern Gargle menu_fx_gargle;
}
