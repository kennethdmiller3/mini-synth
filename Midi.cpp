/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

MIDI Support
*/
#include "StdAfx.h"

#include "Debug.h"
#include "Midi.h"
#include "Voice.h"
#include "Control.h"

// midi messages
// http://www.midi.org/techspecs/midimessages.php

namespace Midi
{
	// status types
	enum Status
	{
		MIDI_NOTE_OFF,
		MIDI_NOTE_ON,
		MIDI_KEY_PRESSURE,
		MIDI_CONTROL_CHANGE,
		MIDI_PROGRAM_CHANGE,
		MIDI_CHANNEL_PRESSURE,
		MIDI_PITCH_WHEEL_CHANGE,
		MIDI_SYSTEM,
		MIDI_COUNT
	};

	// special channel modes
	enum ChannelMode
	{
		MIDI_ALL_SOUND_OFF = 120,
		MIDI_RESET_ALL_CONTROLLERS = 121,
		MIDI_LOCAL_CONTROL = 122,
		MIDI_ALL_NOTES_OFF = 123,
		MIDI_OMNI_MODE_OFF = 124,
		MIDI_OMNI_MODE_ON = 125,
		MIDI_MONO_MODE_ON = 126,
		MIDI_POLY_MODE_ON = 127,
	};

	namespace Input
	{
		HMIDIIN handle;

		void HandleData(DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			unsigned char channel = ((dwParam1)& 0xF) + 1;
			unsigned char status = (dwParam1 >> 4) & 0x7;
			unsigned char data1 = (dwParam1 >> 8) & 0xFF;
			unsigned char data2 = (dwParam1 >> 16) & 0xFF;
			unsigned long timestamp = dwParam2;

			DebugPrint("%4d.%03ds ", timestamp / 1000, timestamp % 1000);

			if (status != MIDI_SYSTEM)
			{
				DebugPrint("#%02d ", channel);
			}
			switch (status)
			{
			case MIDI_NOTE_OFF:
				DebugPrint("Note Off:       note=%d velocity=%d\n", data1, data2);
				NoteOff(data1, data2);
				break;
			case MIDI_NOTE_ON:
				DebugPrint("Note On:        note=%d velocity=%d\n", data1, data2);
				if (data2)
					NoteOn(data1, data2);
				else
					NoteOff(data1);
				break;
			case MIDI_KEY_PRESSURE:
				DebugPrint("Key Pressure:   note=%d pressure=%d\n", data1, data2);
				break;
			case MIDI_CONTROL_CHANGE:
				switch (data1)
				{
				default:
					DebugPrint("Control Change: control=%d value=%d\n", data1, data2);
					break;
				case MIDI_ALL_SOUND_OFF:
					DebugPrint("All Sound Off\n");
					break;
				case MIDI_RESET_ALL_CONTROLLERS:
					DebugPrint("Reset All Controllers\n");
					Control::ResetAll();
					break;
				case MIDI_LOCAL_CONTROL:
					DebugPrint("Local Control %s\n", data2 ? "On" : "Off");
					break;
				case MIDI_ALL_NOTES_OFF:
					DebugPrint("All Notes Off\n");
					for (int v = 0; v < VOICES; ++v)
						NoteOff(voice_note[v], 0);
					break;
				case MIDI_OMNI_MODE_OFF:
					DebugPrint("Omni Mode Off\n");
					break;
				case MIDI_OMNI_MODE_ON:
					DebugPrint("Omni Mode On\n");
					break;
				case MIDI_MONO_MODE_ON:
					DebugPrint("Mono Mode On %d\n", data2);
					break;
				case MIDI_POLY_MODE_ON:
					DebugPrint("Poly Mode On\n");
					break;
				}
				break;
			case MIDI_PROGRAM_CHANGE:
				DebugPrint("Program Change: program=%d\n", data1);
				break;
			case MIDI_CHANNEL_PRESSURE:
				DebugPrint("Channel Pressure: pressure=%d\n", data1);
				break;
			case MIDI_PITCH_WHEEL_CHANGE:
				DebugPrint("Pitch Wheel Change: value=%d\n", (data2 << 7) + data1 - 0x2000);
				Control::SetPitchWheel((data2 << 7) + data1 - 0x2000);
				break;
			case MIDI_SYSTEM:
				DebugPrint("System %02x %02x %02x\n", data1, data2);
				break;
			}
		}

		void CALLBACK Proc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			switch (wMsg)
			{
			case MM_MIM_OPEN:
				DebugPrint("MIDI Input Open\n");
				break;
			case MM_MIM_CLOSE:
				DebugPrint("MIDI Input Close\n");
				break;
			case MM_MIM_DATA:
				HandleData(dwInstance, dwParam1, dwParam2);
				break;
			case MM_MIM_LONGDATA:
				DebugPrint("MIDI Input Long Data: %08x %08x\n", dwParam1, dwParam2);
				break;
			case MM_MIM_ERROR:
				DebugPrint("MIDI Input Error: %08x %08x\n", dwParam1, dwParam2);
				break;
			case MM_MIM_LONGERROR:
				DebugPrint("MIDI Input Long Error: %08x %08x\n", dwParam1, dwParam2);
				break;
			}
		}

		unsigned int GetNumDevices()
		{
			return midiInGetNumDevs();
		}

		MMRESULT GetDeviceCaps(unsigned int id, MIDIINCAPS &caps)
		{
			return midiInGetDevCaps(id, &caps, sizeof(MIDIINCAPS));
		}

		MMRESULT Open(unsigned int deviceId)
		{
			// open midi input
			MMRESULT mmResult;
			mmResult = midiInOpen(&handle, deviceId, DWORD_PTR(Proc), NULL, CALLBACK_FUNCTION);
			if (mmResult)
			{
				char buf[256];
				GetLastErrorMessage(buf, 256);
				DebugPrint("Error opening MIDI input: %s", buf);
			}
			return mmResult;
		}

		void Start()
		{
			if (handle)
			{
				// start midi input
				midiInStart(handle);
			}
		}

		void Stop()
		{
			if (handle)
			{
				// stop midi input
				midiInStop(handle);
			}
		}

		void Close()
		{
			if (handle)
			{
				midiInClose(handle);
			}
		}
	}

}