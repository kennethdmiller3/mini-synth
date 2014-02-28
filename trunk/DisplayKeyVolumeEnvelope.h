#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Per-Key Volume Envelope Display
*/

#include "Envelope.h"

class DisplayKeyVolumeEnvelope
{
public:
	void Init(HANDLE hOut);
	void Update(HANDLE hOut);
};
