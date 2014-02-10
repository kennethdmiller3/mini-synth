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
	AMP menu_amp({ 41, 18 }, "F5 AMP", AMP::COUNT);

	void AMP::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			amp_env_config.enable = sign > 0;
			break;
		case LEVEL_ENV:
			UpdatePercentageProperty(amp_config.level_env, sign, modifiers, 0, 1);
			break;
		case LEVEL_ENV_VEL:
			UpdatePercentageProperty(amp_config.level_env_vel, sign, modifiers, 0, 1);
			break;
		case ENV_ATTACK:
			UpdateTimeProperty(amp_env_config.attack_time, sign, modifiers, 0, 10);
			amp_env_config.attack_rate = 1.0f / (amp_env_config.attack_time + FLT_MIN);
			break;
		case ENV_DECAY:
			UpdateTimeProperty(amp_env_config.decay_time, sign, modifiers, 0, 10);
			amp_env_config.decay_rate = 1.0f / (amp_env_config.decay_time + FLT_MIN);
			break;
		case ENV_SUSTAIN:
			UpdatePercentageProperty(amp_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case ENV_RELEASE:
			UpdateTimeProperty(amp_env_config.release_time, sign, modifiers, 0, 10);
			amp_env_config.release_rate = 1.0f / (amp_env_config.release_time + FLT_MIN);
			break;
		default:
			__assume(0);
		}
	}

	void AMP::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, amp_env_config.enable, flags, " ON", "OFF");
			break;
		case LEVEL_ENV:
			PrintItemFloat(hOut, pos, flags, "Level ENV:  %5.1f%%", amp_config.level_env * 100.0f);
			break;
		case LEVEL_ENV_VEL:
			PrintItemFloat(hOut, pos, flags, "Level VEL:  %5.1f%%", amp_config.level_env_vel * 100.0f);
			break;
		case ENV_ATTACK:
			PrintItemFloat(hOut, pos, flags, "Attack:   %7.3fs", amp_env_config.attack_time);
			break;
		case ENV_DECAY:
			PrintItemFloat(hOut, pos, flags, "Decay:    %7.3fs", amp_env_config.decay_time);
			break;
		case ENV_SUSTAIN:
			PrintItemFloat(hOut, pos, flags, "Sustain:    %5.1f%%", amp_env_config.sustain_level * 100.0f);
			break;
		case ENV_RELEASE:
			PrintItemFloat(hOut, pos, flags, "Release:  %7.3fs", amp_env_config.release_time);
			break;
		default:
			__assume(0);
		}
	}
}