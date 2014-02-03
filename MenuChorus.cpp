#include "StdAfx.h"

#include "Menu.h"
#include "MenuChorus.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	static const char * const waveform[] = { "Triangle", "Sine" };
	static const char * const phase[] = { "-180", "-90", "0", "90", "180" };

	Chorus menu_fx_chorus({ 1, 12 }, "F1 CHORUS", Chorus::COUNT);

	void Chorus::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_CHORUS] = sign > 0;
			EnableEffect(BASS_FX_DX8_CHORUS, fx_active[BASS_FX_DX8_CHORUS]);
			break;
		case WET_DRY_MIX:
			UpdateProperty(fx_chorus.fWetDryMix, sign, modifiers, 10, time_step, 0, 100);
			break;
		case DEPTH:
			UpdateProperty(fx_chorus.fDepth, sign, modifiers, 10, time_step, 0, 100);
			break;
		case FEEDBACK:
			UpdateProperty(fx_chorus.fFeedback, sign, modifiers, 10, time_step, -99, 99);
			break;
		case FREQUENCY:
			UpdateProperty(fx_chorus.fFrequency, sign, modifiers, 100, time_step, 0, 10);
			break;
		case WAVEFORM:
			fx_chorus.lWaveform = !fx_chorus.lWaveform;
			break;
		case DELAY:
			UpdateProperty(fx_chorus.fDelay, sign, modifiers, 100, time_step, 0, 20);
			break;
		case PHASE:
			fx_chorus.lPhase = (fx_chorus.lPhase + 5 + sign) % 5;
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_CHORUS);
	}

	void Chorus::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_CHORUS], flags, " ON", "OFF");
			break;
		case WET_DRY_MIX:
			PrintItemFloat(hOut, pos, flags, "Wet/Dry:   % 6.1f%%", fx_chorus.fWetDryMix);
			break;
		case DEPTH:
			PrintItemFloat(hOut, pos, flags, "Depth:     % 6.1f%%", fx_chorus.fDepth);
			break;
		case FEEDBACK:
			PrintItemFloat(hOut, pos, flags, "Feedback:  % 6.1f%%", fx_chorus.fFeedback);
			break;
		case FREQUENCY:
			PrintItemFloat(hOut, pos, flags, "Freq:      %5.2fHz", fx_chorus.fFrequency);
			break;
		case WAVEFORM:
			PrintItemString(hOut, pos, flags, "Waveform: %8s", waveform[fx_chorus.lWaveform]);
			break;
		case DELAY:
			PrintItemFloat(hOut, pos, flags, "Delay:    %6.2fms", fx_chorus.fDelay);
			break;
		case PHASE:
			PrintItemString(hOut, pos, flags, "L/R Phase: %7s", phase[fx_chorus.lPhase]);
			break;
		default:
			__assume(0);
		}
	}
}
