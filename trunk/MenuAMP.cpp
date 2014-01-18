/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Amplifier Menu
*/
#include "StdAfx.h"

#include "MenuAMP.h"
#include "Console.h"
#include "Math.h"
#include "Amplifier.h"

namespace Menu
{
	namespace AMP
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			switch (index)
			{
			case AMP_TITLE:
				amp_env_config.enable = sign > 0;
				break;
			case AMP_LEVEL_ENV:
				UpdatePercentageProperty(amp_config.level_env, sign, modifiers, 0, 1);
				break;
			case AMP_LEVEL_ENV_VEL:
				UpdatePercentageProperty(amp_config.level_env_vel, sign, modifiers, 0, 1);
				break;
			case AMP_ENV_ATTACK:
				UpdateTimeProperty(amp_env_config.attack_time, sign, modifiers, 0, 10);
				amp_env_config.attack_rate = 1.0f / Max(amp_env_config.attack_time, 0.0001f);
				break;
			case AMP_ENV_DECAY:
				UpdateTimeProperty(amp_env_config.decay_time, sign, modifiers, 0, 10);
				amp_env_config.decay_rate = 1.0f / Max(amp_env_config.decay_time, 0.0001f);
				break;
			case AMP_ENV_SUSTAIN:
				UpdatePercentageProperty(amp_env_config.sustain_level, sign, modifiers, 0, 1);
				break;
			case AMP_ENV_RELEASE:
				UpdateTimeProperty(amp_env_config.release_time, sign, modifiers, 0, 10);
				amp_env_config.release_rate = 1.0f / Max(amp_env_config.release_time, 0.0001f);
				break;
			default:
				__assume(0);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			switch (index)
			{
			case AMP_TITLE:
				PrintMenuTitle(menu, hOut, pos, amp_env_config.enable, flags, " ON", "OFF");
				break;
			case AMP_LEVEL_ENV:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Level ENV:  %5.1f%%", amp_config.level_env * 100.0f);
				break;
			case AMP_LEVEL_ENV_VEL:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Level VEL:  %5.1f%%", amp_config.level_env_vel * 100.0f);
				break;
			case AMP_ENV_ATTACK:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Attack:   %7.3fs", amp_env_config.attack_time);
				break;
			case AMP_ENV_DECAY:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Decay:    %7.3fs", amp_env_config.decay_time);
				break;
			case AMP_ENV_SUSTAIN:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Sustain:    %5.1f%%", amp_env_config.sustain_level * 100.0f);
				break;
			case AMP_ENV_RELEASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Release:  %7.3fs", amp_env_config.release_time);
				break;
			default:
				__assume(0);
			}
		}
	}

}