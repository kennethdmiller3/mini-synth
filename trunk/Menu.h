#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/

namespace Menu
{
	// menu modes
	enum MenuMode
	{
		MENU_OSC1,
		MENU_OSC2,
		MENU_LFO,
		MENU_FLT,
		MENU_AMP,
		MENU_FX,

		MENU_COUNT
	};

	// menu attributes
	extern WORD const title_attrib[2][3];
	extern WORD const item_attrib[3];

	// selected menu
	extern MenuMode active;

	// selected item in each menu
	extern int item[MENU_COUNT];

	// menu names
	extern char const * const name[MENU_COUNT];

	// position of each menu
	extern COORD const pos[MENU_COUNT];

	// set the active menu
	extern void SetActive(HANDLE hOut, MenuMode menu);

	// update a logarthmic-frequency property
	extern void UpdateFrequencyProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

	// update a linear percentage property
	extern void UpdatePercentageProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

	// update a linear time property
	extern void UpdateTimeProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

	// print a menu title
	extern void PrintMenuTitle(MenuMode menu, HANDLE hOut, COORD pos, bool enable, DWORD flags, char const *on_text, char const *off_text);

	// general menu handler
	extern void Handler(HANDLE hOut, WORD key, DWORD modifiers, MenuMode menu);
}
