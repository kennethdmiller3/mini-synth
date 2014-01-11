#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/

enum MenuMode;

// menu modes
enum MenuMode
{
	MENU_OSC1,
	MENU_OSC2,
	MENU_LFO,
	MENU_FLT,
	MENU_VOL,
	MENU_FX,

	MENU_COUNT
};

// menu attributes
extern WORD const menu_title_attrib[2][3];
extern WORD const menu_item_attrib[2];

// selected menu
extern MenuMode menu_active;

// selected item in each menu
extern int menu_item[MENU_COUNT];

// menu names
extern char const * const menu_name[MENU_COUNT];

// position of each menu
extern COORD const menu_pos[MENU_COUNT];

// update a logarthmic-frequency property
extern void UpdateFrequencyProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

// update a linear percentage property
extern void UpdatePercentageProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

// update a linear time property
extern void UpdateTimeProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum);

// print a menu title
extern void PrintMenuTitle(HANDLE hOut, MenuMode menu, bool component_enabled, char const * const on_text, char const * const off_text);

// show selection marker on an item
extern void ShowMarker(HANDLE hOut, MenuMode menu, int item);

// hide selection marker on an item
extern void HideMarker(HANDLE hOut, MenuMode menu, int item);
