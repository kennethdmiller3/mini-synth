#pragma once

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