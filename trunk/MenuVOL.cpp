/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Volume Envelope Menu
*/
#include "StdAfx.h"

#include "MenuVol.h"
#include "Console.h"
#include "Math.h"
#include "Envelope.h"

namespace Menu
{
	namespace VOL
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			switch (index)
			{
			case VOL_TITLE:
				vol_env_config.enable = sign > 0;
				break;
			case VOL_ENV_ATTACK:
				UpdateTimeProperty(vol_env_config.attack_time, sign, modifiers, 0, 10);
				vol_env_config.attack_rate = 1.0f / Max(vol_env_config.attack_time, 0.0001f);
				break;
			case VOL_ENV_DECAY:
				UpdateTimeProperty(vol_env_config.decay_time, sign, modifiers, 0, 10);
				vol_env_config.decay_rate = 1.0f / Max(vol_env_config.decay_time, 0.0001f);
				break;
			case VOL_ENV_SUSTAIN:
				UpdatePercentageProperty(vol_env_config.sustain_level, sign, modifiers, 0, 1);
				break;
			case VOL_ENV_RELEASE:
				UpdateTimeProperty(vol_env_config.release_time, sign, modifiers, 0, 10);
				vol_env_config.release_rate = 1.0f / Max(vol_env_config.release_time, 0.0001f);
				break;
			default:
				__assume(0);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			switch (index)
			{
			case VOL_TITLE:
				PrintMenuTitle(menu, hOut, pos, vol_env_config.enable, flags, " ON", "OFF");
				break;
			case VOL_ENV_ATTACK:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Attack:   %7.3fs", vol_env_config.attack_time);
				break;
			case VOL_ENV_DECAY:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Decay:    %7.3fs", vol_env_config.decay_time);
				break;
			case VOL_ENV_SUSTAIN:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Sustain:    %5.1f%%", vol_env_config.sustain_level * 100.0f);
				break;
			case VOL_ENV_RELEASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Release:  %7.3fs", vol_env_config.release_time);
				break;
			default:
				__assume(0);
			}
		}
	}

}