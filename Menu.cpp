/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/
#include "StdAfx.h"
#include "Math.h"
#include "Console.h"
#include "Menu.h"
#include "MenuOSC.h"
#include "MenuLFO.h"
#include "MenuFLT.h"
#include "MenuAMP.h"
#include "MenuFX.h"

namespace Menu
{
	// selected menu
	MenuMode active = MENU_OSC1;

	// selected item in each menu
	int item[MENU_COUNT];

	// menu names
	char const * const name[MENU_COUNT] =
	{
		"OSC1",
		"OSC2",
		"LFO",
		"FLT",
		"AMP",
		"FX",
	};

	// position of each menu
	COORD const pos[MENU_COUNT] =
	{
		{ 1, 12 },	// MENU_OSC1,
		{ 21, 12 },	// MENU_OSC2,
		{ 41, 12 },	// MENU_LFO,
		{ 41, 18 },	// MENU_FLT,
		{ 61, 10 },	// MENU_AMP,
		{ 61, 18 },	// MENU_FX,
	};
	// menu attributes
	WORD const title_attrib[2][3] =
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
	WORD const item_attrib[3] =
	{
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
	};

	// set the active menu
	void SetActive(HANDLE hOut, MenuMode menu)
	{
		MenuMode prev_active = active;
		active = MENU_COUNT;
		Handler(hOut, 0, 0, prev_active);
		active = menu;
		Handler(hOut, 0, 0, active);
	}

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
	void PrintMenuTitle(MenuMode menu, HANDLE hOut, COORD pos, bool enable, DWORD flags, char const *on_text, char const *off_text)
	{
		// print menu name
		WORD const attrib = title_attrib[enable][flags];
		PrintConsoleWithAttribute(hOut, pos, attrib, "F%d %-15s", menu + 1, name[menu]);

		// print component enabled
		if (char const *text = enable ? on_text : off_text)
		{
			WORD const enable_attrib = (attrib & 0xF8) | (enable ? FOREGROUND_GREEN : (flags ? FOREGROUND_RED : 0));
			PrintConsoleWithAttribute(hOut, { pos.X + 15, pos.Y }, enable_attrib, text);
		}
	}

	// print marker
	void PrintMarker(HANDLE hOut, MenuMode menu, int item, CHAR_INFO left, CHAR_INFO right)
	{
		COORD const zero = { 0, 0 };
		COORD const size = { 1, 1 };
		SMALL_RECT region;

		region.Left = region.Right = pos[menu].X - 1;
		region.Top = region.Bottom = pos[menu].Y + SHORT(item);
		WriteConsoleOutput(hOut, &left, size, zero, &region);
		region.Left = region.Right = pos[menu].X + 18;
		WriteConsoleOutput(hOut, &right, size, zero, &region);
	}

	// marker data
	CHAR_INFO const marker_left = { 0x10, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	CHAR_INFO const marker_right = { 0x11, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	CHAR_INFO const marker_blank = { 0, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };

	// show selection marker on an item
	inline void ShowMarker(HANDLE hOut, MenuMode menu, int item)
	{
		PrintMarker(hOut, menu, item, marker_left, marker_right);
	}

	// hide selection marker on an item
	inline void HideMarker(HANDLE hOut, MenuMode menu, int item)
	{
		PrintMarker(hOut, menu, item, marker_blank, marker_blank);
	}

	// 
	struct Data
	{
		void(*Update)(MenuMode menu, int index, int sign, DWORD modifiers);
		void(*Print)(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags);
		int count;
	};
	Data const data[MENU_COUNT] =
	{
		{ Menu::OSC::Update, Menu::OSC::Print, Menu::OSC::OSC_COUNT - 1 },
		{ Menu::OSC::Update, Menu::OSC::Print, Menu::OSC::OSC_COUNT },
		{ Menu::LFO::Update, Menu::LFO::Print, Menu::LFO::LFO_COUNT },
		{ Menu::FLT::Update, Menu::FLT::Print, Menu::FLT::FLT_COUNT },
		{ Menu::AMP::Update, Menu::AMP::Print, Menu::AMP::AMP_COUNT },
		{ Menu::FX::Update, Menu::FX::Print, Menu::FX::FX_COUNT },
	};

	// general menu handler
	void Handler(HANDLE hOut, WORD key, DWORD modifiers, MenuMode menu)
	{
		int &item = Menu::item[menu];
		COORD const &pos = Menu::pos[menu];
		Menu::Data const &data = Menu::data[menu];
		int const count = data.count;

		switch (key)
		{
		case 0:
			if (active == menu)
			{
				ShowMarker(hOut, menu, item);
				for (int i = 0; i < count; ++i)
					data.Print(menu, i, hOut, { pos.X, pos.Y + SHORT(i) }, item == i ? 2 : 1);
			}
			else
			{
				HideMarker(hOut, menu, item);
				for (int i = 0; i < count; ++i)
					data.Print(menu, i, hOut, { pos.X, pos.Y + SHORT(i) }, 0);
			}
			break;

		case VK_UP:
			HideMarker(hOut, menu, item);
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 1);
			if (--item < 0)
				item = count - 1;
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 2);
			ShowMarker(hOut, menu, item);
			break;

		case VK_DOWN:
			HideMarker(hOut, menu, item);
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 1);
			if (++item >= count)
				item = 0;
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 2);
			ShowMarker(hOut, menu, item);
			break;

		case VK_LEFT:
			data.Update(menu, item, -1, modifiers);
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 2);
			break;

		case VK_RIGHT:
			data.Update(menu, item, +1, modifiers);
			data.Print(menu, item, hOut, { pos.X, pos.Y + SHORT(item) }, 2);
			break;
		}
	}
}
