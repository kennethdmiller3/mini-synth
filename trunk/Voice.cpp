#include "StdAfx.h"

#include "Voice.h"
#include "OscillatorNote.h"
#include "Filter.h"
#include "Amplifier.h"
#include "Control.h"

// current note assignemnts
// (via keyboard or midi input)
unsigned char voice_note[VOICES];
unsigned char voice_vel[VOICES];
unsigned char note_voice[NOTES];

// most recent voice triggered
int voice_most_recent;

// most recent note key pressed
// (via keyboard or midi input)
int note_most_recent;

// choose a voice
int ChooseVoice(int note)
{
	int voice = -1;

	// quietest voice
	int quietest_voice = -1;
	float quietest_amplitude = FLT_MAX;

	// find a voice to use
	for (int v = 0; v < VOICES; ++v)
	{
		// if retriggering the voice or the voice is currently off...
		if (voice_note[v] == note || amp_env_state[v].state == EnvelopeState::OFF)
		{
			// use this voice
			voice = v;
			break;
		}

		// if the voice is quieter than the current quietest...
		if (amp_env_state[v].amplitude < quietest_amplitude)
		{
			// use that
			quietest_voice = v;
			quietest_amplitude = amp_env_state[v].amplitude;
		}
	}

	// use the quietest voice if not already assigned a voice
	if (voice < 0)
	{
		voice = quietest_voice;
	}

	return voice;
}

// note frequency
float NoteFrequency(int note, float follow)
{
	float const base = (note - 60) / 12.0f + Control::pitch_offset;
	return powf(2, follow * base) * middle_c_frequency;
}

// note on
int NoteOn(int note, int velocity)
{
	// choose a voice
	int voice = ChooseVoice(note);
	if (voice < 0)
		return voice;

	// set most recent voice and note
	voice_most_recent = voice;
	note_most_recent = note;

	// set voice note
	voice_note[voice] = unsigned char(note);
	note_voice[note] = unsigned char(voice);

	// set voice velocity
	voice_vel[voice] = unsigned char(velocity);

	// start the oscillator
	// (assume restart on key)
	for (int o = 0; o < NUM_OSCILLATORS; ++o)
		osc_state[voice][o].Start();

	// start the filter
	flt_state[voice].Reset();

	// gate the volume envelope
	amp_env_state[voice].Gate(amp_env_config, true);

	// gate the filter envelope
	flt_env_state[voice].Gate(flt_env_config, true);

	return voice;
}

// note off
int NoteOff(int note, int velocity)
{
	// find the voice for the note
	int voice = note_voice[note];
	if (voice < 0)
		return voice;

	// TO DO: use note-off velocity

	// gate the volume envelope
	amp_env_state[voice].Gate(amp_env_config, false);

	// gate the filter envelope
	flt_env_state[voice].Gate(flt_env_config, false);

	return voice;
}
