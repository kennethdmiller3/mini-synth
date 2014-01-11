/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Spectrum Analyzer Display
*/
#include "StdAfx.h"

#include "DisplaySpectrumAnalyzer.h"
#include "Math.h"

#define FREQUENCY_BINS 4096

static CHAR_INFO const bar_full = { 0, BACKGROUND_GREEN };
static CHAR_INFO const bar_top = { 223, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static CHAR_INFO const bar_bottom = { 220, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static CHAR_INFO const bar_empty = { 0, BACKGROUND_BLUE };
static CHAR_INFO const bar_nyquist = { 0, BACKGROUND_RED };

// SPECTRUM ANALYZER
// horizontal axis shows semitone frequency bands
// vertical axis shows logarithmic power
void UpdateSpectrumAnalyzer(HANDLE hOut, DWORD stream, BASS_INFO const &info, float const freq_min)
{
	// get complex FFT data
	float fft[FREQUENCY_BINS * 2][2];
	BASS_ChannelGetData(stream, &fft[0][0], BASS_DATA_FFT8192 | BASS_DATA_FFT_COMPLEX);

	// get the lower frequency bin for the zeroth semitone band
	// (half a semitone down from the center frequency)
	float const semitone = powf(2.0f, 1.0f / 12.0f);
	float freq = freq_min * FREQUENCY_BINS * 2.0f / info.freq / sqrtf(semitone);
	int b0 = Max(RoundInt(freq), 0);

	// get power in each semitone band
	float spectrum[SPECTRUM_WIDTH] = { 0 };
	int xlimit = SPECTRUM_WIDTH;
	for (int x = 0; x < SPECTRUM_WIDTH; ++x)
	{
		// get upper frequency bin for the current semitone
		freq *= semitone;
		int const b1 = Min(RoundInt(freq), FREQUENCY_BINS);

		// ensure there's at least one bin
		// (or quit upon reaching the last bin)
		if (b0 == b1)
		{
			if (b1 == FREQUENCY_BINS)
			{
				xlimit = x;
				break;
			}
			--b0;
		}

		// sum power across the semitone band
		float scale = float(FREQUENCY_BINS) / float(b1 - b0);
		float value = 0.0f;
		for (; b0 < b1; ++b0)
			value += fft[b0][0] * fft[b0][0] + fft[b0][1] * fft[b0][1];
		spectrum[x] = scale * value;
	}

	// inaudible band
	int xinaudible = RoundInt(logf(20000 / freq_min) * 12 / logf(2));

	// plot log-log spectrum
	// each grid cell is one semitone wide and 6 dB high
	CHAR_INFO buf[SPECTRUM_HEIGHT][SPECTRUM_WIDTH];
	COORD const pos = { 0, 0 };
	COORD const size = { SPECTRUM_WIDTH, SPECTRUM_HEIGHT };
	SMALL_RECT region = { 0, 0, 79, 49 };
	float threshold = 1.0f;
	for (int y = 0; y < SPECTRUM_HEIGHT; ++y)
	{
		int x;
		for (x = 0; x < xlimit; ++x)
		{
			if (spectrum[x] < threshold)
				buf[y][x] = bar_empty;
			else if (spectrum[x] < threshold * 2.0f)
				buf[y][x] = bar_bottom;
			else if (spectrum[x] < threshold * 4.0f)
				buf[y][x] = bar_top;
			else
				buf[y][x] = bar_full;
			if (x >= xinaudible)
				buf[y][x].Attributes |= BACKGROUND_RED;
		}
		for (; x < SPECTRUM_WIDTH; ++x)
		{
			buf[y][x] = bar_nyquist;
		}
		threshold *= 0.25f;
	}
	WriteConsoleOutput(hOut, &buf[0][0], size, pos, &region);
}
