#pragma once

#include "Menu.h"

namespace Menu
{
	class ReverbI3D : public Menu
	{
	public:
		enum Item
		{
			TITLE,
			ROOM,
			ROOM_HF,
			ROOM_ROLLOFF,
			DECAY_TIME,
			DECAY_HF_RATIO,
			REFLECTIONS,
			REFLECTIONS_DELAY,
			REVERB,
			REVERB_DELAY,
			DIFFUSION,
			DENSITY,
			HF_REFERENCE,
			COUNT
		};

		// constructor
		ReverbI3D(SMALL_RECT rect, const char *name, int count)
			: Menu(rect, name, count)
		{
		}

	protected:
		virtual void Update(int index, int sign, DWORD modifiers);
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags);
	};

	extern ReverbI3D menu_fx_reverb3d;
}
