#include "StdAfx.h"

#include "Menu.h"
#include "MenuFlanger.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	static const char * const waveform[] = { "Triangle", "Sine" };
	static const char * const phase[] = { "-180", "-90", "0", "90", "180" };

	Flanger menu_fx_flanger({ 1, page_pos.Y + 9 }, "F5 FLANGER", Flanger::COUNT);

	void Flanger::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_FLANGER] = sign > 0;
			EnableEffect(BASS_FX_DX8_FLANGER, fx_active[BASS_FX_DX8_FLANGER]);
			break;
		case WET_DRY_MIX:
			UpdateProperty(fx_flanger.fWetDryMix, sign, modifiers, 10, time_step, 0, 100);
			break;
		case DEPTH:
			UpdateProperty(fx_flanger.fDepth, sign, modifiers, 10, time_step, 0, 100);
			break;
		case FEEDBACK:
			UpdateProperty(fx_flanger.fFeedback, sign, modifiers, 10, time_step, -99, 99);
			break;
		case FREQUENCY:
			UpdateProperty(fx_flanger.fFrequency, sign, modifiers, 100, time_step, 0, 10);
			break;
		case WAVEFORM:
			fx_flanger.lWaveform = !fx_flanger.lWaveform;
			break;
		case DELAY:
			UpdateProperty(fx_flanger.fDelay, sign, modifiers, 100, time_step, 0, 20);
			break;
		case PHASE:
			fx_flanger.lPhase = (fx_flanger.lPhase + 5 + sign) % 5;
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_FLANGER);
	}

	void Flanger::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_FLANGER], flags, " ON", "OFF");
			break;
		case WET_DRY_MIX:
			PrintItemFloat(hOut, pos, flags, "Wet/Dry:   % 6.1f%%", fx_flanger.fWetDryMix);
			break;
		case DEPTH:
			PrintItemFloat(hOut, pos, flags, "Depth:     % 6.1f%%", fx_flanger.fDepth);
			break;
		case FEEDBACK:
			PrintItemFloat(hOut, pos, flags, "Feedback:  % 6.1f%%", fx_flanger.fFeedback);
			break;
		case FREQUENCY:
			PrintItemFloat(hOut, pos, flags, "Freq:      %5.2fHz", fx_flanger.fFrequency);
			break;
		case WAVEFORM:
			PrintItemString(hOut, pos, flags, "Waveform: %8s", waveform[fx_flanger.lWaveform]);
			break;
		case DELAY:
			PrintItemFloat(hOut, pos, flags, "Delay:    %6.2fms", fx_flanger.fDelay);
			break;
		case PHASE:
			PrintItemString(hOut, pos, flags, "L/R Phase: %7s", phase[fx_flanger.lPhase]);
			break;
		default:
			__assume(0);
		}
	}
}
