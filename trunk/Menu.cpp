/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/
#include "StdAfx.h"
#include "Math.h"
#include "Console.h"
#include "Menu.h"

// menu attributes
WORD const menu_title_attrib[2][3] =
{
	{
		// component turned off
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_RED,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
	},
	{
		// component turned on
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
	}
};
WORD const menu_item_attrib[2] =
{
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
};


// selected menu
MenuMode menu_active = MENU_OSC1;

// selected item in each menu
int menu_item[MENU_COUNT];

// menu names
char const * const menu_name[MENU_COUNT] =
{
	"OSC1",
	"OSC2",
	"LFO",
	"FLT",
	"VOL",
	"FX",
};

// position of each menu
COORD const menu_pos[MENU_COUNT] =
{
	{ 1, 12 },	// MENU_OSC1,
	{ 21, 12 },	// MENU_OSC2,
	{ 41, 12 },	// MENU_LFO,
	{ 41, 18 },	// MENU_FLT,
	{ 61, 12 },	// MENU_VOL,
	{ 61, 18 },	// MENU_FX,
};

// update a logarthmic-frequency property
void UpdateFrequencyProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
{
	float value = roundf(property * 1200.0f);
	if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		value += sign * 1;		// tiny step: 1 cent
	else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		value += sign * 10;		// small step: 10 cents
	else if (!(modifiers & (SHIFT_PRESSED)))
		value += sign * 100;	// normal step: 1 semitone
	else
		value += sign * 1200;	// large step: 1 octave
	property = Clamp(value / 1200.0f, minimum, maximum);
}

// update a linear percentage property
void UpdatePercentageProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
{
	float value = roundf(property * 256.0f);
	if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		value += sign * 1;	// tiny step: 1/256
	else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		value += sign * 4;	// small step: 4/256
	else if (!(modifiers & (SHIFT_PRESSED)))
		value += sign * 16;	// normal step: 16/256
	else
		value += sign * 64;	// large step: 64/256
	property = Clamp(value / 256.0f, minimum, maximum);
}

// update a linear time property
void UpdateTimeProperty(float &property, int const sign, DWORD const modifiers, float const minimum, float const maximum)
{
	float value = roundf(property * 1000.0f);
	if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		value += sign * 1;		// tiny step: 1ms
	else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		value += sign * 10;		// small step: 10ms
	else if (!(modifiers & (SHIFT_PRESSED)))
		value += sign * 100;	// normal step: 100ms
	else
		value += sign * 1000;	// large step: 1s
	property = Clamp(value / 1000.0f, minimum, maximum);
}

// print a menu title
void PrintMenuTitle(HANDLE hOut, MenuMode menu, bool component_enabled, char const * const on_text, char const * const off_text)
{
	DWORD written;
	bool const menu_selected = (menu_active == menu);
	bool const title_selected = menu_selected && menu_item[menu] == 0;
	WORD const title_attrib = menu_title_attrib[component_enabled][menu_selected + title_selected];

	// print menu name
	COORD const pos = menu_pos[menu];
	FillConsoleOutputAttribute(hOut, title_attrib, 18, pos, &written);
	PrintConsole(hOut, pos, "F%d %-15s", menu + 1, menu_name[menu]);

	if (char const *text = component_enabled ? on_text : off_text)
	{
		// print component enabled
		COORD const pos = { menu_pos[menu].X + 15, menu_pos[menu].Y };
		WORD const enable_attrib = (title_attrib & 0xF8) | (component_enabled ? FOREGROUND_GREEN : (menu_selected ? FOREGROUND_RED : 0));
		FillConsoleOutputAttribute(hOut, enable_attrib, 3, pos, &written);
		PrintConsole(hOut, pos, "%3s", text);
	}
}

// show selection marker on an item
void ShowMarker(HANDLE hOut, MenuMode menu, int item)
{
	COORD const zero = { 0, 0 };
	CHAR_INFO const left = { 0x10, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	CHAR_INFO const right = { 0x11, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	COORD const size = { 1, 1 };
	SMALL_RECT region;

	region.Left = region.Right = menu_pos[menu].X - 1;
	region.Top = region.Bottom = menu_pos[menu].Y + item;
	WriteConsoleOutput(hOut, &left, size, zero, &region);
	region.Left = region.Right = menu_pos[menu].X + 18;
	WriteConsoleOutput(hOut, &right, size, zero, &region);
}

// hide selection marker on an item
void HideMarker(HANDLE hOut, MenuMode menu, int item)
{
	COORD const zero = { 0, 0 };
	CHAR_INFO const blank = { 0, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
	COORD const size = { 1, 1 };
	SMALL_RECT region;

	region.Left = region.Right = menu_pos[menu].X - 1;
	region.Top = region.Bottom = menu_pos[menu].Y + item;
	WriteConsoleOutput(hOut, &blank, size, zero, &region);
	region.Left = region.Right = menu_pos[menu].X + 18;
	WriteConsoleOutput(hOut, &blank, size, zero, &region);
}
