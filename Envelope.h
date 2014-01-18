#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Envelope Generator
*/

// envelope generator configuration
class EnvelopeConfig
{
public:
	bool enable;
	float attack_time;
	float attack_rate;
	float decay_time;
	float decay_rate;
	float sustain_level;
	float release_time;
	float release_rate;

	EnvelopeConfig(bool const enable, float const attack_time, float const decay_time, float const sustain_level, float const release_time);
};

// envelope generator state
class EnvelopeState
{
public:
	bool gate;
	enum State
	{
		OFF,

		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE,

		COUNT
	};
	State state;
	float amplitude;

	EnvelopeState();

	void Gate(EnvelopeConfig const &config, bool on);

	float Update(EnvelopeConfig const &config, float const step);
};
