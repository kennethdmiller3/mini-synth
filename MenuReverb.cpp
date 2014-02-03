#include "StdAfx.h"

#include "Menu.h"
#include "MenuReverb.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	Reverb menu_fx_reverb({ 61, 21 }, "F8 REVERB", Reverb::COUNT);

	void Reverb::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_REVERB] = sign > 0;
			EnableEffect(BASS_FX_DX8_REVERB, fx_active[BASS_FX_DX8_REVERB]);
			break;
		case GAIN:
			UpdateProperty(fx_reverb.fInGain, sign, modifiers, 100, time_step, -96, 0);
			break;
		case REVERB_MIX:
			UpdateProperty(fx_reverb.fReverbMix, sign, modifiers, 100, time_step, -96, 0);
			break;
		case DECAY_TIME:
			UpdateProperty(fx_reverb.fReverbTime, sign, modifiers, 10, time_step, 0.001f, 3000);
			break;
		case DECAY_HF_RATIO:
			UpdateProperty(fx_reverb.fHighFreqRTRatio, sign, modifiers, 10000, time_step, 0.001f, 0.999f);
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_REVERB);
	}

	void Reverb::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_REVERB], flags, " ON", "OFF");
			break;
		case GAIN:
			PrintItemFloat(hOut, pos, flags, "Gain:     %+6.2fdB", fx_reverb.fInGain);
			break;
		case REVERB_MIX:
			PrintItemFloat(hOut, pos, flags, "Mix:      %+6.2fdB", fx_reverb.fReverbMix);
			break;
		case DECAY_TIME:
			PrintItemFloat(hOut, pos, flags, "Decay:   %7.2fms", fx_reverb.fReverbTime);
			break;
		case DECAY_HF_RATIO:
			PrintItemFloat(hOut, pos, flags, "HF Ratio:   %6.4f", fx_reverb.fHighFreqRTRatio);
			break;
		default:
			__assume(0);
		}
	}
}
