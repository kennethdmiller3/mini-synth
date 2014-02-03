#include "StdAfx.h"

#include "Menu.h"
#include "MenuCompressor.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	Compressor menu_fx_compressor({ 21, 12 }, "F2 COMPRESS", Compressor::COUNT);

	void Compressor::Update(int index, int sign, DWORD modifiers)
	{
		float value;
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_COMPRESSOR] = sign > 0;
			EnableEffect(BASS_FX_DX8_COMPRESSOR, fx_active[BASS_FX_DX8_COMPRESSOR]);
			break;
		case GAIN:
			UpdateProperty(fx_compressor.fGain, sign, modifiers, 100, time_step, -60, 60);
			break;
		case ATTACK:
			UpdateProperty(fx_compressor.fAttack, sign, modifiers, 10, time_step, 0.01f, 500);
			break;
		case RELEASE:
			UpdateProperty(fx_compressor.fRelease, sign, modifiers, 10, time_step, 50, 3000);
			break;
		case THRESHOLD:
			UpdateProperty(fx_compressor.fThreshold, sign, modifiers, 100, time_step, -60, 0);
			break;
		case RATIO:
			UpdateProperty(fx_compressor.fRatio, sign, modifiers, 100, time_step, 1, 100);
			break;
		case PREDELAY:
			UpdateProperty(fx_compressor.fPredelay, sign, modifiers, 100, time_step, 0, 4);
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_COMPRESSOR);
	}

	void Compressor::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_COMPRESSOR], flags, " ON", "OFF");
			break;
		case GAIN:
			PrintItemFloat(hOut, pos, flags, "Gain:     %+6.2fdB", fx_compressor.fGain);
			break;
		case ATTACK:
			PrintItemFloat(hOut, pos, flags, "Attack:   %6.1fms", fx_compressor.fAttack);
			break;
		case RELEASE:
			PrintItemFloat(hOut, pos, flags, "Release:  %6.1fms", fx_compressor.fRelease);
			break;
		case THRESHOLD:
			PrintItemFloat(hOut, pos, flags, "Thresh:   %+6.2fdB", fx_compressor.fThreshold);
			break;
		case RATIO:
			PrintItemFloat(hOut, pos, flags, "Ratio:     %7.3f", fx_compressor.fRatio);
			break;
		case PREDELAY:
			PrintItemFloat(hOut, pos, flags, "Pre Delay:  %4.2fms", fx_compressor.fPredelay);
			break;
		default:
			__assume(0);
		}
	}
}
