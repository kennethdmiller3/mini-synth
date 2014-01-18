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
	namespace FLT
	{
		void Update(MenuMode menu, int index, int sign, DWORD modifiers)
		{
			switch (index)
			{
			case FLT_TITLE:
				flt_config.enable = sign > 0;
				flt_env_config.enable = sign > 0;
				break;
			case FLT_MODE:
				flt_config.mode = FilterConfig::Mode((flt_config.mode + FilterConfig::COUNT + sign) % FilterConfig::COUNT);
				break;
			case FLT_RESONANCE:
				UpdatePercentageProperty(flt_config.resonance, sign, modifiers, 0, 4);
				break;
			case FLT_CUTOFF_BASE:
				UpdateFrequencyProperty(flt_config.cutoff_base, sign, modifiers, -10, 10);
				break;
			case FLT_CUTOFF_LFO:
				UpdateFrequencyProperty(flt_config.cutoff_lfo, sign, modifiers, -10, 10);
				break;
			case FLT_CUTOFF_ENV:
				UpdateFrequencyProperty(flt_config.cutoff_env, sign, modifiers, -10, 10);
				break;
			case FLT_CUTOFF_ENV_VEL:
				UpdateFrequencyProperty(flt_config.cutoff_env_vel, sign, modifiers, -10, 10);
				break;
			case FLT_ENV_ATTACK:
				UpdateTimeProperty(flt_env_config.attack_time, sign, modifiers, 0, 10);
				flt_env_config.attack_rate = 1.0f / Max(flt_env_config.attack_time, 0.0001f);
				break;
			case FLT_ENV_DECAY:
				UpdateTimeProperty(flt_env_config.decay_time, sign, modifiers, 0, 10);
				flt_env_config.decay_rate = 1.0f / Max(flt_env_config.decay_time, 0.0001f);
				break;
			case FLT_ENV_SUSTAIN:
				UpdatePercentageProperty(flt_env_config.sustain_level, sign, modifiers, 0, 1);
				break;
			case FLT_ENV_RELEASE:
				UpdateTimeProperty(flt_env_config.release_time, sign, modifiers, 0, 10);
				flt_env_config.release_rate = 1.0f / Max(flt_env_config.release_time, 0.0001f);
				break;
			default:
				__assume(0);
			}
		}

		void Print(MenuMode menu, int index, HANDLE hOut, COORD pos, DWORD flags)
		{
			switch (index)
			{
			case FLT_TITLE:
				PrintMenuTitle(menu, hOut, pos, flt_config.enable, flags, " ON", "OFF");
				break;
			case FLT_MODE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "%-18s", filter_name[flt_config.mode]);
				break;
			case FLT_RESONANCE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Resonance: % 7.3f", flt_config.resonance);
				break;
			case FLT_CUTOFF_BASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Cutoff:    % 7.2f", flt_config.cutoff_base * 12.0f);
				break;
			case FLT_CUTOFF_LFO:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Cutoff LFO:% 7.2f", flt_config.cutoff_lfo * 12.0f);
				break;
			case FLT_CUTOFF_ENV:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Cutoff ENV:% 7.2f", flt_config.cutoff_env * 12.0f);
				break;
			case FLT_CUTOFF_ENV_VEL:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Cutoff VEL:% 7.2f", flt_config.cutoff_env_vel * 12.0f);
				break;
			case FLT_ENV_ATTACK:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Attack:   %7.3fs", flt_env_config.attack_time);
				break;
			case FLT_ENV_DECAY:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Decay:    %7.3fs", flt_env_config.decay_time);
				break;
			case FLT_ENV_SUSTAIN:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Sustain:    %5.1f%%", flt_env_config.sustain_level * 100.0f);
				break;
			case FLT_ENV_RELEASE:
				PrintConsoleWithAttribute(hOut, pos, item_attrib[flags], "Release:  %7.3fs", flt_env_config.release_time);
				break;
			default:
				__assume(0);
			}
		}
	}
}
