#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Voice Assignment
*/

// midi note frequencies
// (11 octaves + 1)
#define NOTES 133
extern float note_frequency[NOTES];

// number of voices
#define VOICES 16

// current note assignemnts
// (via keyboard or midi input)
extern unsigned char voice_note[VOICES];
extern unsigned char voice_vel[VOICES];

// most recent voice triggered
extern int voice_most_recent;

// most recent note key pressed
// (via keyboard or midi input)
extern int note_most_recent;

// note on
// (returns voice index)
extern int NoteOn(int note, int velocity = 64);

// note off
// (returns voice index)
extern int NoteOff(int note, int velocity = 64);
