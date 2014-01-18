#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

MIDI Support
*/

namespace Midi
{
	namespace Input
	{
		unsigned int GetNumDevices();
		MMRESULT GetDeviceCaps(unsigned int id, MIDIINCAPS &caps);

		MMRESULT Open(unsigned int deviceId);
		void Start();
		void Stop();
		void Close();
	}
}