#include "StdAfx.h"

#include "Menu.h"
#include "MenuEcho.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	Echo menu_fx_echo({ 61, page_pos.Y }, "F4 ECHO", Echo::COUNT);

	void Echo::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_ECHO] = sign > 0;
			EnableEffect(BASS_FX_DX8_ECHO, fx_active[BASS_FX_DX8_ECHO]);
			break;
		case WET_DRY_MIX:
			UpdateProperty(fx_echo.fWetDryMix, sign, modifiers, 10, time_step, 0, 100);
			break;
		case FEEDBACK:
			UpdateProperty(fx_echo.fFeedback, sign, modifiers, 10, time_step, 0, 100);
			break;
		case LEFT_DELAY:
			UpdateProperty(fx_echo.fLeftDelay, sign, modifiers, 10, time_step, 1, 2000);
			break;
		case RIGHT_DELAY:
			UpdateProperty(fx_echo.fRightDelay, sign, modifiers, 10, time_step, 1, 2000);
			break;
		case PAN_DELAY:
			fx_echo.lPanDelay = !fx_echo.lPanDelay;
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_ECHO);
	}

	void Echo::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_ECHO], flags, " ON", "OFF");
			break;
		case WET_DRY_MIX:
			PrintItemFloat(hOut, pos, flags, "Wet/Dry:   % 6.1f%%", fx_echo.fWetDryMix);
			break;
		case FEEDBACK:
			PrintItemFloat(hOut, pos, flags, "Feedback:  % 6.1f%%", fx_echo.fFeedback);
			break;
		case LEFT_DELAY:
			PrintItemFloat(hOut, pos, flags, "L Delay: %7.2fms", fx_echo.fLeftDelay);
			break;
		case RIGHT_DELAY:
			PrintItemFloat(hOut, pos, flags, "R Delay: %7.2fms", fx_echo.fRightDelay);
			break;
		case PAN_DELAY:
			PrintItemBool(hOut, pos, flags, "Pan Delay:     ", fx_echo.lPanDelay != 0);
			break;
		default:
			__assume(0);
		}
	}
}
