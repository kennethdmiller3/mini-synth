#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Spectrum Analyzer Display
*/

#define SPECTRUM_WIDTH 80
#define SPECTRUM_HEIGHT 10

// fast fourier transform properties
#define FFT_TYPE 5
static int const SCALE = 128;
static int const FFT_SIZE = 256 << FFT_TYPE;
static int const FREQUENCY_BINS = FFT_SIZE / 2;

class DisplaySpectrumAnalyzer
{
public:
	void Init(DWORD stream, BASS_INFO const &info);
	void Cleanup(DWORD stream);
	void Update(HANDLE hOut, DWORD stream, BASS_INFO const &info, float const freq_min);

private:

	static void CALLBACK AppendDataToSlidingBuffer(HDSP handle, DWORD channel, float *buffer, DWORD length, DisplaySpectrumAnalyzer *user);
	static DWORD CALLBACK GetDataFromSlidingBuffer(HSTREAM handle, void *buffer, DWORD length, DisplaySpectrumAnalyzer *user);

	// sliding sample buffer stream
	CRITICAL_SECTION cs;
	HSTREAM sliding_stream;
	HDSP collect_samples;

	// sliding sample buffer for use by the FFT
	float sliding_buffer[FFT_SIZE];
};
