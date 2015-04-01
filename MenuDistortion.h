#pragma once

#include "Menu.h"

namespace Menu
{
	class Distortion : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			GAIN,
			EDGE,
			POST_EQ_CENTER,
			POST_EQ_BANDWIDTH,
			PRE_LOWPASS_CUTOFF,
			COUNT
		};

		// constructor
		Distortion(SMALL_RECT rect, const char *name, int count)
			: Menu(rect, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern Distortion menu_fx_distortion;
}
