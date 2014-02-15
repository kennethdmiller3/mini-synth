#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/

namespace Menu
{
	class Menu;

	// pages
	enum Page
	{
		PAGE_MAIN,
		PAGE_FX,
		
		PAGE_COUNT
	};

	// main menu modes
	enum MainMode
	{
		MAIN_OSC1,
		MAIN_OSC2,
		MAIN_LFO,
		MAIN_FLT,
		MAIN_AMP,

		MAIN_COUNT
	};

	struct PageInfo
	{
		Menu * const *menu;
		int count;
	};
	extern PageInfo const page_info[];
	extern COORD const page_pos;

	// menu attributes
	extern WORD const title_attrib[2][3];
	extern WORD const item_attrib[3];

	// active page
	extern Page active_page;

	// active menu index
	extern int active_menu;

	// update a property
	// sign: direction to change the property (+ or -)
	// modifiers: keys that determine speed (tiny, small, normal, large)
	// scale: quantizes the property to 1/scale
	// step: step size for each speed; scale acts as the denominator
	// minimum: smallest value the property can have
	// maximum: largest value the property can have
	extern void UpdateProperty(float &property, int const sign, DWORD const modifiers, float const scale, float const step[], float const minimum, float const maximum);
	extern void UpdateProperty(int &property, int const sign, DWORD const modifiers, int const scale, int const step[], int const minimum, int const maximum);

	// update a logarthmic-frequency property
	extern float const pitch_step[];
	inline void UpdatePitchProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
	{
		UpdateProperty(property, sign, modifiers, 1200, pitch_step, minimum, maximum);
	}

	// update a linear percentage property
	extern float const percent_step[];
	inline void UpdatePercentageProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
	{
		UpdateProperty(property, sign, modifiers, 256, percent_step, minimum, maximum);
	}

	// update a linear time property
	extern float const time_step[];
	inline void UpdateTimeProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
	{
		UpdateProperty(property, sign, modifiers, 1000, time_step, minimum, maximum);
	}

	// set the active page
	extern void SetActivePage(HANDLE hOut, Page page);

	// set the active menu
	extern void SetActiveMenu(HANDLE hOut, int menu);
	extern void NextMenu(HANDLE hOut);
	extern void PrevMenu(HANDLE hOut);

	// menu input handler
	extern void Handler(HANDLE hOut, WORD key, DWORD modifiers);

	// print all menus on the active page
	extern void PrintActivePage();

	// utility functions for printing menu items
	extern void PrintItemFloat(HANDLE hOut, COORD pos, DWORD flags, char const *format, float value);
	extern void PrintItemString(HANDLE hOut, COORD pos, DWORD flags, char const *format, char const *value);
	extern void PrintItemBool(HANDLE hOut, COORD pos, DWORD flags, char const *format, bool value);

	class Menu
	{
	public:
		COORD pos;			// screen position
		const char *name;	// menu title
		int count;			// number of items

		int item;			// selected item

		// constructor
		Menu(COORD pos, const char *name, int count)
			: pos(pos)
			, name(name)
			, count(count)
			, item(0)
		{
		}

		// print the whole menu
		void Print(HANDLE hOut);

		// shared input handler
		void Handler(HANDLE hOut, WORD key, DWORD modifiers);

	protected:
		// update a single item
		virtual void Update(int index, int sign, DWORD modifiers) = 0;

		// print a single item
		virtual void Print(int index, HANDLE hOut, COORD pos, DWORD flags) = 0;

		// utility function for printing title bar with attributes based on enable and flags
		void PrintTitle(HANDLE hOut, bool enable, DWORD flags, char const *on_text, char const *off_text);
	};
}
