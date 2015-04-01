#pragma once

#include "Menu.h"

namespace Menu
{
	class Echo : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			WET_DRY_MIX,
			FEEDBACK,
			LEFT_DELAY,
			RIGHT_DELAY,
			PAN_DELAY,
			COUNT
		};

		// constructor
		Echo(SMALL_RECT rect, const char *name, int count)
			: Menu(rect, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern Echo menu_fx_echo;
}
