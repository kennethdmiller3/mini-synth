#include "StdAfx.h"

#include "Menu.h"
#include "MenuReverbI3D.h"
#include "Effect.h"
#include "Console.h"

namespace Menu
{
	ReverbI3D menu_fx_reverb3d({ 41, page_pos.Y + 9, 41 + 18, page_pos.Y + 9 + ReverbI3D::COUNT }, "REVERB I3D", ReverbI3D::COUNT);

	static int atten_step[] = { 1, 10, 100, 1000 };

	void ReverbI3D::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			fx_active[BASS_FX_DX8_I3DL2REVERB] = sign > 0;
			EnableEffect(BASS_FX_DX8_I3DL2REVERB, fx_active[BASS_FX_DX8_I3DL2REVERB]);
			break;
		case ROOM:
			UpdateProperty(fx_reverb3d.lRoom, sign, modifiers, 1, atten_step, -10000, 0);
			break;
		case ROOM_HF:
			UpdateProperty(fx_reverb3d.lRoomHF, sign, modifiers, 1, atten_step, -10000, 0);
			break;
		case ROOM_ROLLOFF:
			UpdateProperty(fx_reverb3d.flRoomRolloffFactor, sign, modifiers, 1000, time_step, 0, 10);
			break;
		case DECAY_TIME:
			UpdateProperty(fx_reverb3d.flDecayTime, sign, modifiers, 1000, time_step, 0.1f, 20);
			break;
		case DECAY_HF_RATIO:
			UpdateProperty(fx_reverb3d.flDecayHFRatio, sign, modifiers, 10000, time_step, 0.1f, 2);
			break;
		case REFLECTIONS:
			UpdateProperty(fx_reverb3d.lReflections, sign, modifiers, 1, atten_step, -10000, 1000);
			break;
		case REFLECTIONS_DELAY:
			UpdateProperty(fx_reverb3d.flReflectionsDelay, sign, modifiers, 10000, time_step, 0, 0.3f);
			break;
		case REVERB:
			UpdateProperty(fx_reverb3d.lReverb, sign, modifiers, 1, atten_step, -10000, 2000);
			break;
		case REVERB_DELAY:
			UpdateProperty(fx_reverb3d.flReverbDelay, sign, modifiers, 10000, time_step, 0, 0.1f);
			break;
		case DIFFUSION:
			UpdateProperty(fx_reverb3d.flDiffusion, sign, modifiers, 100, time_step, 0, 100);
			break;
		case DENSITY:
			UpdateProperty(fx_reverb3d.flDensity, sign, modifiers, 100, time_step, 0, 100);
			break;
		case HF_REFERENCE:
			UpdateProperty(fx_reverb3d.flHFReference, sign, modifiers, 1, time_step, 20, 20000);
			break;
		default:
			__assume(0);
		}
		UpdateEffect(BASS_FX_DX8_I3DL2REVERB);
	}

	void ReverbI3D::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, fx_active[BASS_FX_DX8_I3DL2REVERB], flags, " ON", "OFF");
			break;
		case ROOM:
			PrintItemFloat(hOut, pos, flags, "Room:     %+6.2fdB", fx_reverb3d.lRoom / 100.0f);
			break;
		case ROOM_HF:
			PrintItemFloat(hOut, pos, flags, "Room HF:  %+6.2fdB", fx_reverb3d.lRoomHF / 100.0f);
			break;
		case ROOM_ROLLOFF:
			PrintItemFloat(hOut, pos, flags, "Rolloff:    %6.3f", fx_reverb3d.flRoomRolloffFactor);
			break;
		case DECAY_TIME:
			PrintItemFloat(hOut, pos, flags, "Decay:   %7.2fms", fx_reverb3d.flDecayTime * 1000);
			break;
		case DECAY_HF_RATIO:
			PrintItemFloat(hOut, pos, flags, "HF Ratio:   %6.4f", fx_reverb3d.flDecayHFRatio);
			break;
		case REFLECTIONS:
			PrintItemFloat(hOut, pos, flags, "Reflect:  %+6.2fdB", fx_reverb3d.lReflections / 100.0f);
			break;
		case REFLECTIONS_DELAY:
			PrintItemFloat(hOut, pos, flags, "Ref Dly: %7.2fms", fx_reverb3d.flReflectionsDelay * 1000);
			break;
		case REVERB:
			PrintItemFloat(hOut, pos, flags, "Reverb:   %+6.2fdB", fx_reverb3d.lReverb / 100.0f);
			break;
		case REVERB_DELAY:
			PrintItemFloat(hOut, pos, flags, "Rev Dly: %7.2fms", fx_reverb3d.flReverbDelay * 1000);
			break;
		case DIFFUSION:
			PrintItemFloat(hOut, pos, flags, "Diffusion:  %5.1f%%", fx_reverb3d.flDiffusion);
			break;
		case DENSITY:
			PrintItemFloat(hOut, pos, flags, "Density:    %5.1f%%", fx_reverb3d.flDiffusion);
			break;
		case HF_REFERENCE:
			PrintItemFloat(hOut, pos, flags, "HF Ref:  %7.1fHz", fx_reverb3d.flHFReference);
			break;
		default:
			__assume(0);
		}
	}
}
