/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Filter Menu
*/
#include "StdAfx.h"

#include "MenuFLT.h"
#include "Console.h"
#include "Math.h"
#include "Filter.h"
#include "Envelope.h"

namespace Menu
{
	FLT menu_flt({ 61, page_pos.Y }, "F4 FLT", FLT::COUNT);

	void FLT::Update(int index, int sign, DWORD modifiers)
	{
		switch (index)
		{
		case TITLE:
			flt_config.enable = sign > 0;
			flt_env_config.enable = sign > 0;
			break;
		case MODE:
			flt_config.SetMode(FilterConfig::Mode((flt_config.mode + FilterConfig::COUNT + sign) % FilterConfig::COUNT));
			break;
		case DRIVE:
			UpdatePercentageProperty(flt_config.drive, sign, modifiers, 0, 10);
			break;
		case RESONANCE:
			UpdatePercentageProperty(flt_config.resonance, sign, modifiers, 0, 4);
			break;
		case CUTOFF_BASE:
			UpdatePitchProperty(flt_config.cutoff_base, sign, modifiers, -10, 10);
			break;
		case CUTOFF_LFO:
			UpdatePitchProperty(flt_config.cutoff_lfo, sign, modifiers, -10, 10);
			break;
		case CUTOFF_ENV:
			UpdatePitchProperty(flt_config.cutoff_env, sign, modifiers, -10, 10);
			break;
		case CUTOFF_ENV_VEL:
			UpdatePitchProperty(flt_config.cutoff_env_vel, sign, modifiers, -10, 10);
			break;
		case KEY_FOLLOW:
			UpdatePercentageProperty(flt_config.key_follow, sign, modifiers, -2, 2);
			break;
		case ENV_ATTACK:
			UpdateTimeProperty(flt_env_config.attack_time, sign, modifiers, 0, 10);
			flt_env_config.attack_rate = 1.0f / (flt_env_config.attack_time + FLT_MIN);
			break;
		case ENV_DECAY:
			UpdateTimeProperty(flt_env_config.decay_time, sign, modifiers, 0, 10);
			flt_env_config.decay_rate = 1.0f / (flt_env_config.decay_time + FLT_MIN);
			break;
		case ENV_SUSTAIN:
			UpdatePercentageProperty(flt_env_config.sustain_level, sign, modifiers, 0, 1);
			break;
		case ENV_RELEASE:
			UpdateTimeProperty(flt_env_config.release_time, sign, modifiers, 0, 10);
			flt_env_config.release_rate = 1.0f / (flt_env_config.release_time + FLT_MIN);
			break;
		default:
			__assume(0);
		}
	}

	void FLT::Print(int index, HANDLE hOut, COORD pos, DWORD flags)
	{
		switch (index)
		{
		case TITLE:
			PrintTitle(hOut, flt_config.enable, flags, NULL, "OFF");
			break;
		case MODE:
			PrintItemString(hOut, pos, flags, "%-18s", filter_name[flt_config.mode]);
			break;
		case DRIVE:
			PrintItemFloat(hOut, pos, flags, "Drive:    % 7.1f%%", flt_config.drive * 100.0f);
			break;
		case RESONANCE:
			PrintItemFloat(hOut, pos, flags, "Resonance:% 7.1f%%", flt_config.resonance * 100.0f);
			break;
		case CUTOFF_BASE:
			PrintItemFloat(hOut, pos, flags, "Cutoff:    % 7.2f", flt_config.cutoff_base * 12.0f);
			break;
		case CUTOFF_LFO:
			PrintItemFloat(hOut, pos, flags, "Cutoff LFO:% 7.2f", flt_config.cutoff_lfo * 12.0f);
			break;
		case CUTOFF_ENV:
			PrintItemFloat(hOut, pos, flags, "Cutoff ENV:% 7.2f", flt_config.cutoff_env * 12.0f);
			break;
		case CUTOFF_ENV_VEL:
			PrintItemFloat(hOut, pos, flags, "Cutoff VEL:% 7.2f", flt_config.cutoff_env_vel * 12.0f);
			break;
		case KEY_FOLLOW:
			PrintItemFloat(hOut, pos, flags, "Key Follow:% 6.1f%%", flt_config.key_follow * 100.0f);
			break;
		case ENV_ATTACK:
			PrintItemFloat(hOut, pos, flags, "Attack:   %7.3fs", flt_env_config.attack_time);
			break;
		case ENV_DECAY:
			PrintItemFloat(hOut, pos, flags, "Decay:    %7.3fs", flt_env_config.decay_time);
			break;
		case ENV_SUSTAIN:
			PrintItemFloat(hOut, pos, flags, "Sustain:    %5.1f%%", flt_env_config.sustain_level * 100.0f);
			break;
		case ENV_RELEASE:
			PrintItemFloat(hOut, pos, flags, "Release:  %7.3fs", flt_env_config.release_time);
			break;
		default:
			__assume(0);
		}
	}
}
