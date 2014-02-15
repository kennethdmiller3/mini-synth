/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Spectrum Analyzer Display
*/
#include "StdAfx.h"

#include "DisplaySpectrumAnalyzer.h"
#include "Math.h"

static HSTREAM sliding_stream;
static HDSP collect_samples;

#define FFT_TYPE 5

static int const SCALE = 128;
static int const FFT_SIZE = 256 << FFT_TYPE;
static int const FREQUENCY_BINS = FFT_SIZE / 2;

// sliding sample buffer for use by the FFT
float sliding_buffer[FFT_SIZE];

// frequency constants
static float const semitone = 1.0594630943592952645618252949463f;	//powf(2.0f, 1.0f / 12.0f);
static float const quartertone = 0.97153194115360586874328941582127f;	//1/sqrtf(semitone)
static float const freq_scale = FREQUENCY_BINS * 2.0f * quartertone;

// position and size
static COORD const pos = { 0, 0 };
static COORD const size = { SPECTRUM_WIDTH, SPECTRUM_HEIGHT };

// plotting characters
static CHAR_INFO const bar_full = { 0, BACKGROUND_GREEN };
static CHAR_INFO const bar_top = { 223, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static CHAR_INFO const bar_bottom = { 220, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
static CHAR_INFO const bar_empty = { 0, BACKGROUND_BLUE };
static CHAR_INFO const bar_nyquist = { 0, BACKGROUND_RED };

// add data from the audio stream to the sliding sample buffer
static void CALLBACK AppendDataToSlidingBuffer(HDSP handle, DWORD channel, float *buffer, DWORD length, void *user)
{
	unsigned int count = length / (2 * sizeof(float));
	if (count < FFT_SIZE)
	{
		// shift sample data down and add new data to the end,
		memmove(sliding_buffer, sliding_buffer + count, (FFT_SIZE - count) * sizeof(float));
	}
	else if (count > FFT_SIZE)
	{
		// use the last FFT_SIZE sliding_buffer
		buffer += (count - FFT_SIZE) * 2;
		count = FFT_SIZE;
	}

	// combine stereo sliding_buffer into mono for analysis
	for (unsigned int i = 0; i < count; ++i)
	{
		sliding_buffer[FFT_SIZE - count + i] = 0.5f * (buffer[i + i] + buffer[i + i + 1]);
	}
}

// get data from the sliding sample buffer
static DWORD CALLBACK GetDataFromSlidingBuffer(HSTREAM handle, void *buffer, DWORD length, void *user)
{
	memcpy(buffer, (char *)sliding_buffer + FFT_SIZE * sizeof(float) - length, length);
	return length;
}

void InitSpectrumAnalyzer(DWORD stream, BASS_INFO const &info)
{
	// add a channel DSP to collect samples
	collect_samples = BASS_ChannelSetDSP(stream, (DSPPROC *)AppendDataToSlidingBuffer, NULL, -1);

	// create a data stream to return data from the sliding buffer
	sliding_stream = BASS_StreamCreate(info.freq, 1, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE, GetDataFromSlidingBuffer, NULL);
}

void CleanupSpectrumAnalyzer(DWORD stream)
{
	// remove the channel DSP
	BASS_ChannelRemoveDSP(stream, collect_samples);

	// free the data stream
	BASS_StreamFree(sliding_stream);
}

// SPECTRUM ANALYZER
// horizontal axis shows semitone frequency bands
// vertical axis shows logarithmic power
void UpdateSpectrumAnalyzer(HANDLE hOut, DWORD stream, BASS_INFO const &info, float const freq_min)
{
	// get complex FFT data from the sliding buffer
	float fft[FREQUENCY_BINS * 2][2];
	BASS_ChannelGetData(sliding_stream, &fft[0][0], (BASS_DATA_FFT256 + FFT_TYPE) | BASS_DATA_FFT_COMPLEX);

	// get the lower frequency bin for the zeroth semitone band
	// (half a semitone down from the center frequency)
	float freq = freq_scale * freq_min / info.freq;
	int b0 = Max(RoundInt(freq), 0);

	// get power in each semitone band
	float spectrum[SPECTRUM_WIDTH] = { 0 };
	int xlimit = SPECTRUM_WIDTH;
	float prev_value = SCALE * (fft[b0][0] * fft[b0][0] + fft[b0][1] * fft[b0][1]);

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
			spectrum[x] = prev_value;
			continue;
		}

		// sum power across the semitone band
		float scale = SCALE / float(b1 - b0);
		float value = 0.0f;
		for (; b0 < b1; ++b0)
			value += fft[b0][0] * fft[b0][0] + fft[b0][1] * fft[b0][1];
		spectrum[x] = prev_value = scale * value;
	}

	// inaudible band
	int xinaudible = RoundInt(log2f(20000 / freq_min) * 12);

	// plot log-log spectrum
	// each grid cell is one semitone wide and 6 dB high
	CHAR_INFO buf[SPECTRUM_HEIGHT][SPECTRUM_WIDTH];
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
