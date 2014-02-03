#include "StdAfx.h"

#include "Menu.h"
#include "MenuDistortion.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	Distortion menu_fx_distortion({ 41, 12 }, "F3 DISTORT", Distortion::COUNT);

	void Distortion::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_DISTORTION] = sign > 0;
			EnableEffect(BASS_FX_DX8_DISTORTION, fx_active[BASS_FX_DX8_DISTORTION]);
			break;
		case GAIN:
			UpdateProperty(fx_distortion.fGain, sign, modifiers, 100, time_step, -60, 60);
			break;
		case EDGE:
			UpdateProperty(fx_distortion.fEdge, sign, modifiers, 10, time_step, 0, 100);
			break;
		case POST_EQ_CENTER:
			UpdateProperty(fx_distortion.fPostEQCenterFrequency, sign, modifiers, 1, time_step, 100, 8000);
			break;
		case POST_EQ_BANDWIDTH:
			UpdateProperty(fx_distortion.fPostEQBandwidth, sign, modifiers, 1, time_step, 100, 8000);
			break;
		case PRE_LOWPASS_CUTOFF:
			UpdateProperty(fx_distortion.fPreLowpassCutoff, sign, modifiers, 1, time_step, 100, 8000);
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_DISTORTION);
	}

	void Distortion::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_DISTORTION], flags, " ON", "OFF");
			break;
		case GAIN:
			PrintItemFloat(hOut, pos, flags, "Gain:     %+6.2fdB", fx_distortion.fGain);
			break;
		case EDGE:
			PrintItemFloat(hOut, pos, flags, "Edge:      %6.1f%%", fx_distortion.fEdge);
			break;
		case POST_EQ_CENTER:
			PrintItemFloat(hOut, pos, flags, "Center:  %7.1fHz", fx_distortion.fPostEQCenterFrequency);
			break;
		case POST_EQ_BANDWIDTH:
			PrintItemFloat(hOut, pos, flags, "Width:   %7.1fHz", fx_distortion.fPostEQBandwidth);
			break;
		case PRE_LOWPASS_CUTOFF:
			PrintItemFloat(hOut, pos, flags, "Cutuff:  %7.1fHz", fx_distortion.fPreLowpassCutoff);
			break;
		default:
			__assume(0);
		}
	}
}
