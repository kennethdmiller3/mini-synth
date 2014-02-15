#include "StdAfx.h"

#include "Menu.h"
#include "MenuGargle.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	static const char * const waveform[] = { "Triangle", "Square" };

	Gargle menu_fx_gargle({ 21, page_pos.Y + 9 }, "F6 GARGLE", Gargle::COUNT);

	int rate_step[] = { 0, 1, 10, 100 };

	void Gargle::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_GARGLE] = sign > 0;
			EnableEffect(BASS_FX_DX8_GARGLE, fx_active[BASS_FX_DX8_GARGLE]);
			break;
		case FREQUENCY:
			UpdateProperty(*reinterpret_cast<int *>(&fx_gargle.dwRateHz), sign, modifiers, 1, rate_step, 0, 1000);
			break;
		case WAVEFORM:
			fx_gargle.dwWaveShape = !fx_gargle.dwWaveShape;
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_GARGLE);
	}

	void Gargle::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_GARGLE], flags, " ON", "OFF");
			break;
		case FREQUENCY:
			PrintItemFloat(hOut, pos, flags, "Freq:       %4.0fHz", float(fx_gargle.dwRateHz));
			break;
		case WAVEFORM:
			PrintItemString(hOut, pos, flags, "Waveform: %8s", waveform[fx_gargle.dwWaveShape]);
			break;
		default:
			__assume(0);
		}
	}
}
