/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Menu Functions
*/
#include "StdAfx.h"
#include "Math.h"
#include "Console.h"
#include "OscillatorNote.h"
#include "Menu.h"
#include "MenuOSC.h"
#include "MenuLFO.h"
#include "MenuFLT.h"
#include "MenuAMP.h"
#include "MenuChorus.h"
#include "MenuCompressor.h"
#include "MenuDistortion.h"
#include "MenuEcho.h"
#include "MenuFlanger.h"
#include "MenuGargle.h"
#include "MenuReverbI3D.h"
#include "MenuReverb.h"
#include "DisplaySpectrumAnalyzer.h"

namespace Menu
{
	static Menu * const menu_main[] =
	{
		&menu_osc[0],
#if NUM_OSCILLATORS >= 2
		&menu_osc[1],
#if NUM_OSCILLATORS >= 3
		&menu_osc[2],
#if NUM_OSCILLATORS >= 4
		&menu_osc[3],
#endif
#endif
#endif
		&menu_lfo,
		&menu_flt,
		&menu_amp,
	};
	static Menu * const menu_fx[] =
	{
		&menu_fx_chorus,
		&menu_fx_compressor,
		&menu_fx_distortion,
		&menu_fx_echo,
		&menu_fx_flanger,
		&menu_fx_gargle,
		&menu_fx_reverb3d,
		&menu_fx_reverb,
	};

	PageInfo const page_info[] =
	{
		{ menu_main, ARRAY_SIZE(menu_main) },
		{ menu_fx, ARRAY_SIZE(menu_fx) },
	};

	COORD const page_pos = { 0, SPECTRUM_HEIGHT + 4 };

	// active page
	Page active_page = PAGE_MAIN;

	// active menu
	int active_menu = 0;
	int save_menu[PAGE_COUNT];

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

	void UpdateProperty(float &property, int const sign, DWORD const modifiers, float const scale, float const step[], float const minimum, float const maximum)
	{
		float value = roundf(property * scale);
		if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
			value += sign * step[0];	// tiny step
		else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
			value += sign * step[1];	// small step
		else if (!(modifiers & (SHIFT_PRESSED)))
			value += sign * step[2];	// normal step
		else
			value += sign * step[3];	// large step
		property = Clamp(value / scale, minimum, maximum);
	}
	
	void UpdateProperty(int &property, int const sign, DWORD const modifiers, int const scale, int const step[], int const minimum, int const maximum)
	{
		int value = property * scale;
		if (modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
			value += sign * step[0];	// tiny step
		else if (modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
			value += sign * step[1];	// small step
		else if (!(modifiers & (SHIFT_PRESSED)))
			value += sign * step[2];	// normal step
		else
			value += sign * step[3];	// large step
		property = Clamp(value / scale, minimum, maximum);
	}

	// update a logarthmic-frequency property
	float const pitch_step[] = { 1, 10, 100, 1200 };

	// update a linear percentage property
	float const percent_step[] = { 1, 4, 16, 64 };

	// update a linear time property
	float const time_step[] = { 1, 10, 100, 1000 };

	// initialize the menu system
	void Init()
	{
		for (int page = 0; page < ARRAY_SIZE(page_info); ++page)
		{
			for (int i = 0; i < page_info[page].count; ++i)
			{
				page_info[page].menu[i]->menu = i;
			}
		}
	}

	// set the active page
	void SetActivePage(HANDLE hOut, Page page)
	{
		// clear the area
		static DWORD const size = (WINDOW_HEIGHT - 1 - page_pos.Y) * WINDOW_WIDTH;
		DWORD written;
		FillConsoleOutputCharacter(hOut, 0, size, page_pos, &written);
		FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, size, page_pos, &written);

		// switch pages
		save_menu[active_page] = active_menu;
		active_page = page;
		active_menu = save_menu[active_page];

		// print menus on the new page
		for (int i = 0; i < page_info[page].count; ++i)
			page_info[page].menu[i]->Print(hOut);
	}

	// set the active menu
	void SetActiveMenu(HANDLE hOut, int menu)
	{
		if (menu >= 0 && menu < page_info[active_page].count)
		{
			int active_prev = active_menu;
			active_menu = -1;
			page_info[active_page].menu[active_prev]->Print(hOut);
			active_menu = menu;
			page_info[active_page].menu[active_menu]->Print(hOut);
		}
	}

	// switch to the next menu
	void NextMenu(HANDLE hOut)
	{
		SetActiveMenu(hOut, active_menu < page_info[active_page].count - 1 ? active_menu + 1 : 0);
	}

	// switch to the previous menu
	void PrevMenu(HANDLE hOut)
	{
		SetActiveMenu(hOut, active_menu > 0 ? active_menu - 1 : page_info[active_page].count - 1);
	}

	// menu key press handler
	void Handler(HANDLE hOut, WORD key, DWORD modifiers)
	{
		page_info[active_page].menu[active_menu]->Handler(hOut, key, modifiers);
	}

	// menu mouse click handler
	void Click(HANDLE hOut, COORD pos)
	{
		// try dispatching to menus
		for (int menu = 0; menu < page_info[active_page].count; ++menu)
		{
			if (page_info[active_page].menu[menu]->Click(hOut, pos))
				break;
		}
	}


	// print menu title bar
	void Menu::PrintTitle(HANDLE hOut, bool enable, DWORD flags, char const *on_text, char const *off_text)
	{
		// print menu name
		WORD const attrib = title_attrib[enable][flags];
		PrintConsoleWithAttribute(hOut, { rect.Left, rect.Top }, attrib, "F%d %-*s", menu + 1, rect.Right - rect.Left - 3, name);

		// print component enabled
		if (char const *text = enable ? on_text : off_text)
		{
			WORD const enable_attrib = (attrib & 0xF8) | (enable ? FOREGROUND_GREEN : (flags ? FOREGROUND_RED : 0));
			PrintConsoleWithAttribute(hOut, { rect.Right - 3, rect.Top }, enable_attrib, text);
		}
	}

	// print menu item
	void PrintItemFloat(HANDLE hOut, COORD pos, DWORD flags, char const *format, float value)
	{
		WORD const attrib = item_attrib[flags];
		PrintConsoleWithAttribute(hOut, pos, attrib, format, value);
	}
	void PrintItemString(HANDLE hOut, COORD pos, DWORD flags, char const *format, char const *value)
	{
		WORD const attrib = item_attrib[flags];
		PrintConsoleWithAttribute(hOut, pos, attrib, format, value);
	}
	void PrintItemBool(HANDLE hOut, COORD pos, SHORT width, DWORD flags, char const *format, bool value)
	{
		WORD const attrib = item_attrib[flags];
		PrintConsoleWithAttribute(hOut, pos, attrib, format);
		WORD const value_attrib = (attrib & 0xF8) | (value ? FOREGROUND_GREEN : FOREGROUND_RED);
		char const *value_text = value ? " ON" : "OFF";
		PrintConsoleWithAttribute(hOut, { pos.X + width - 3, pos.Y }, value_attrib, value_text);
	}

	// print marker
	void Menu::PrintMarker(HANDLE hOut, int index, CHAR_INFO left, CHAR_INFO right)
	{
		static COORD const zero = { 0, 0 };
		static COORD const size = { 1, 1 };
		SMALL_RECT region;

		region.Left = region.Right = rect.Left - 1;
		region.Top = region.Bottom = rect.Top + SHORT(index);
		WriteConsoleOutput(hOut, &left, size, zero, &region);
		region.Left = region.Right = rect.Right;
		WriteConsoleOutput(hOut, &right, size, zero, &region);
	}

	// marker data
	static CHAR_INFO const marker_left = { 0x10, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	static CHAR_INFO const marker_right = { 0x11, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
	static CHAR_INFO const marker_blank = { 0, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };

	// show selection marker on an item
	inline void Menu::ShowMarker(HANDLE hOut, int index)
	{
		PrintMarker(hOut, index, marker_left, marker_right);
	}

	// hide selection marker on an item
	inline void Menu::HideMarker(HANDLE hOut, int index)
	{
		PrintMarker(hOut, index, marker_blank, marker_blank);
	}

	// print the whole menu
	void Menu::Print(HANDLE hOut)
	{
		int flags[2];
		if (active_menu >= 0 && this == page_info[active_page].menu[active_menu])
		{
			ShowMarker(hOut, item);
			flags[0] = 1; flags[1] = 2;
		}
		else
		{
			HideMarker(hOut, item);
			flags[0] = 0; flags[1] = 0;
		}

		for (int i = 0; i < count; ++i)
		{
			Print(i, hOut, { rect.Left, rect.Top + SHORT(i) }, flags[item == i]);
		}
	}

	// general key press handler
	void Menu::Handler(HANDLE hOut, WORD key, DWORD modifiers)
	{
		int sign;
		COORD pos = { rect.Left, rect.Top };
		COORD p = pos;
		switch (key)
		{
		case VK_UP:
		case VK_DOWN:
			sign = key == VK_DOWN ? 1 : -1;
			p.Y = pos.Y + SHORT(item);
			HideMarker(hOut, item);
			Print(item, hOut, p, 1);
			item = (item + count + sign) % count;
			p.Y = pos.Y + SHORT(item);
			Print(item, hOut, p, 2);
			ShowMarker(hOut, item);
			break;

		case VK_LEFT:
		case VK_RIGHT:
			sign = key == VK_RIGHT ? 1 : -1;
			Update(item, sign, modifiers);
			p.Y = pos.Y + SHORT(item);
			Print(item, hOut, p, 2);
			break;
		}
	}

	// general mouse click handler
	bool Menu::Click(HANDLE hOut, COORD pos)
	{
		if (!Inside(pos))
			return false;
		SetActiveMenu(hOut, menu);
		HideMarker(hOut, item);
		Print(item, hOut, { rect.Left, rect.Top + SHORT(item) }, 1);
		item = Clamp(pos.Y - rect.Top, 0, count - 1);
		Print(item, hOut, { rect.Left, rect.Top + SHORT(item) }, 2);
		ShowMarker(hOut, item);
		return true;
	}
}
