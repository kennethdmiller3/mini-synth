/*
	BASS simple synth
	Copyright (c) 2001-2012 Un4seen Developments Ltd.
	*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include "bass.h"

BASS_INFO info;
HSTREAM stream; // the stream

// display error messages
void Error(char const *text)
{
	fprintf(stderr, "Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

__inline int Clamp(int x, int a, int b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

char const titleText[] = ">>> BASS SIMPLE SYNTH";
COORD const titlePos = { 0, 0 };

WORD const keys[] = {
	'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M',
	'Q', '2', 'W', '3', 'E', 'R', '5', 'T', '6', 'Y', '7', 'U',
};
COORD const keypos[] =
{
	{ 1, 5 }, { 2, 4 }, { 3, 5 }, { 4, 4 }, { 5, 5 }, { 7, 5 }, { 8, 4 }, { 9, 5 }, { 10, 4 }, { 11, 5 }, { 12, 4 }, { 13, 5 },
	{ 1, 2 }, { 2, 1 }, { 3, 2 }, { 4, 1 }, { 5, 2 }, { 7, 2 }, { 8, 1 }, { 9, 2 }, { 10, 1 }, { 11, 2 }, { 12, 1 }, { 13, 2 },
};
#define KEYS ARRAY_SIZE(keys)

char const * const fxname[9] =
{
	"Chorus", "Compressor", "Distortion", "Echo", "Flanger", "Gargle", "I3DL2Reverb", "ParamEQ", "Reverb"
};

static char poly4[(1 << 4) - 1];
static char poly5[(1 << 5) - 1];
static char poly17[(1 << 17) - 1];

float omega[KEYS];
float timescale = 1.0f;
int octave = 4;

#define MAXVOL (0.22*32768)
#define DECAY (MAXVOL/4000)
float vol[KEYS], pos[KEYS]; // keys' volume and pos
int loopcount[KEYS];

// from Atari800 pokey.c
static void InitPoly(char aOut[], int aSize, int aTap, unsigned int aSeed, char aInvert)
{
	unsigned int x = aSeed;
	unsigned int i = 0;
	do
	{
		aOut[i] = (x & 1) ^ aInvert;
		x = ((((x >> aTap) ^ x) & 1) << (aSize - 1)) | (x >> 1);
		++i;
	}
	while (x != aSeed);
}

enum
{
	WAVE_SINE,
	WAVE_SQUARE,
	WAVE_SAWTOOTH,
	WAVE_TRIANGLE,
	WAVE_POLY4,
	WAVE_POLY5,
	WAVE_POLY17,

	WAVE_COUNT
};

static __inline int CommonUpdate(int k, short *buffer, int s, float t)
{
	// left and right channels are the same
	buffer[1] = buffer[0] = (short)Clamp(buffer[0] + s, -32768, 32767);

	pos[k] += omega[k] * t;
	while (pos[k] >= 1.0f)
	{
		pos[k] -= 1.0f;
		++loopcount[k];
	}

	if (vol[k] < MAXVOL)
	{
		vol[k] -= DECAY;
		if (vol[k] <= 0)
		{
			vol[k] = 0; // faded-out
			return 0;
		}
	}
	return 1;
}

inline float EvaluateSine(float a, float t)
{
	return a * sinf(M_PI * 2.0f * t);
}

void WriteSine(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = EvaluateSine(vol[k], pos[k]);

		if (!CommonUpdate(k, buffer + c, s, timescale))
			break;
	}
}

inline float EvaluateSquare(float a, float t)
{
	return t < 0.5f ? a : -a;
}

void WriteSquare(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = EvaluateSquare(vol[k], pos[k]);

		if (!CommonUpdate(k, buffer + c, s, timescale))
			break;
	}
}

inline float EvaluateSawtooth(float a, float t)
{
	if (t < 0.5f)
		return a * 2.0f * t;
	else
		return a * 2.0f * (t - 1.0f);
}

void WriteSawtooth(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = EvaluateSawtooth(vol[k], pos[k]);

		if (!CommonUpdate(k, buffer + c, s, timescale))
			break;
	}
}

inline float EvaluateTriangle(float a, float t)
{
	if (t < 0.25f)
		return a * 4.0f * t;
	else if (t < 0.75f)
		return a * 4.0f * (0.5f - t);
	else
		return a * 4.0f * (t - 1.0f);
}

void WriteTriangle(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = EvaluateTriangle(vol[k], pos[k]);

		if (!CommonUpdate(k, buffer + c, s, timescale))
			break;
	}
}

void WritePoly4(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = poly4[loopcount[k]] ? vol[k] : -vol[k];

		if (!CommonUpdate(k, buffer + c, s, timescale * 4.0f * 15 / 16))
			break;
		if (loopcount[k] >= ARRAY_SIZE(poly4))
			loopcount[k] = 0;
	}
}

void WritePoly5(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = poly5[loopcount[k]] ? vol[k] : -vol[k];

		if (!CommonUpdate(k, buffer + c, s, timescale * 4.0f * 31 / 32))
			break;
		if (loopcount[k] >= ARRAY_SIZE(poly5))
			loopcount[k] = 0;
	}
}

void WritePoly17(int k, short *buffer, DWORD length)
{
	int c, s;
	for (c = 0; c < length / sizeof(short); c += 2)
	{
		s = poly17[loopcount[k]] ? vol[k] : -vol[k];

		if (!CommonUpdate(k, buffer + c, s, timescale * 4.0f))
			break;
		if (loopcount[k] >= ARRAY_SIZE(poly17))
			loopcount[k] = 0;
	}
}

typedef void(*WriteFunc)(int k, short *buffer, DWORD length);
WriteFunc const wavewrite[WAVE_COUNT] =
{
	WriteSine, WriteSquare, WriteSawtooth, WriteTriangle, WritePoly4, WritePoly5, WritePoly17
};
char const * const wavename[WAVE_COUNT] =
{
	"sine", "square", "sawtooth", "triangle", "poly4", "poly5", "poly17"
};
int wavetype = WAVE_SINE;

DWORD CALLBACK WriteStream(HSTREAM handle, short *buffer, DWORD length, void *user)
{
	int k, c, s;
	memset(buffer, 0, length);
	for (k = 0; k < ARRAY_SIZE(keys); k++)
	{
		if (!vol[k]) continue;
		wavewrite[wavetype](k, buffer, length);
	}
	return length;
}

// debug output
int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
	OutputDebugStringA(buf);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

void PrintBufferSize(HANDLE hOut, DWORD buflen)
{
	char buf[64];
	COORD pos = { 1, 7 };
	DWORD written;
	sprintf(buf, "- + Buffer: %3dms", buflen);
	WriteConsoleOutputCharacter(hOut, buf, strlen(buf), pos, &written);
}

void PrintOctave(HANDLE hOut, int octave)
{
	char buf[64];
	COORD pos = { 1, 8 };
	DWORD written;
	sprintf(buf, "[ ] Octave: %d", octave);
	WriteConsoleOutputCharacter(hOut, buf, strlen(buf), pos, &written);
}

void PrintWaveType(HANDLE hOut, int wavetype)
{
	char buf[64];
	COORD pos = { 1, 9 };
	DWORD written;
	sprintf(buf, ", . Wave: %-10s", wavename[wavetype]);
	WriteConsoleOutputCharacter(hOut, buf, strlen(buf), pos, &written);
}

void PrintEffects(HANDLE hOut, HFX fx[], int count)
{
	char buf[64];
	COORD pos = { 32, 1 };
	DWORD written;
	int i;
	for (i = 0; i < count; ++i)
	{
		pos.Y = i + 1;
		sprintf(buf, "F%d %-11s %-3s", i + 1, fxname[i], fx[i] ? "ON" : "off");
		WriteConsoleOutputCharacter(hOut, buf, strlen(buf), pos, &written);
	}
}

void Clear(HANDLE hOut)
{
	CONSOLE_SCREEN_BUFFER_INFO bufInfo;
	COORD pos = { 0, 0 };
	DWORD written;
	DWORD size;
	GetConsoleScreenBufferInfo(hOut, &bufInfo);
	size = bufInfo.dwSize.X * bufInfo.dwSize.Y;
	FillConsoleOutputCharacter(hOut, (TCHAR)' ', size, pos, &written);
	FillConsoleOutputAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, size, pos, &written);
	SetConsoleCursorPosition(hOut, pos);
}

void main(int argc, char **argv)
{
	HFX fx[9] = { 0 }; // effect handles
	INPUT_RECORD keyin;
	DWORD r, buflen;
	int k;
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	int running = 1;

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
	{
		fprintf(stderr, "An incorrect version of BASS.DLL was loaded");
		return;
	}

	// set the window title
	SetConsoleTitle(TEXT(titleText));

#if 0
	// set the console buffer size
	static const COORD bufferSize = { 80, 25 };
	SetConsoleScreenBufferSize(hOut, bufferSize);

	// set the console window size
	static const SMALL_RECT windowSize = { 0, 0, 79, 24 };
	SetConsoleWindowInfo(hOut, TRUE, &windowSize);
#endif

	// clear the window
	Clear(hOut);

	// hide the cursor
	static const CONSOLE_CURSOR_INFO cursorInfo = { 100, FALSE };
	SetConsoleCursorInfo(hOut, &cursorInfo);

	// set input mode
	SetConsoleMode(hIn, 0);

	// 10ms update period
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);

	// setup output - get latency
	//if (!BASS_Init(-1, 44100, BASS_DEVICE_LATENCY, 0, NULL))
	if (!BASS_Init(-1, 48000, BASS_DEVICE_LATENCY, 0, NULL))
		Error("Can't initialize device");

	BASS_GetInfo(&info);
	// default buffer size = update period + 'minbuf' + 1ms extra margin
	BASS_SetConfig(BASS_CONFIG_BUFFER, 10 + info.minbuf + 1);
	buflen = BASS_GetConfig(BASS_CONFIG_BUFFER);
	// if the device's output rate is unknown default to 44100 Hz
	if (!info.freq) info.freq = 44100;
	// create a stream, stereo so that effects sound nice
	stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);

	// initialize polynomial noise tables
	InitPoly(poly4, 4, 1, 0xF, 0);
	InitPoly(poly5, 5, 2, 0x1F, 1);
	InitPoly(poly17, 17, 5, 0x1FFFF, 0);

	// compute phase advance per sample per key
	for (k = 0; k < KEYS; ++k)
	{
		//omega[k] = pow(2.0, (k + 3) / 12.0) * 220.0 / info.freq;	// middle C = 261.626Hz; A above middle C = 440Hz
		omega[k] = pow(2.0, k / 12.0) * 256.0f / info.freq;		// middle C = 256Hz
	}

	DebugPrint("device latency: %dms\n", info.latency);
	DebugPrint("device minbuf: %dms\n", info.minbuf);
	DebugPrint("ds version: %d (effects %s)\n", info.dsver, info.dsver < 8 ? "disabled" : "enabled");

	// show the note keys
	for (k = 0; k < KEYS; ++k)
	{
		char buf[] = { keys[k], '\0' };
		WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		DWORD written;
		WriteConsoleOutputAttribute(hOut, &attrib, 1, keypos[k], &written);
		WriteConsoleOutputCharacter(hOut, buf, 1, keypos[k], &written);
	}

	PrintBufferSize(hOut, buflen);
	PrintOctave(hOut, octave);
	PrintWaveType(hOut, wavetype);
	if (info.dsver >= 8)
		PrintEffects(hOut, fx, 9);

	/*
	printf(
		"press -/+ to de/increase the buffer\n"
		"press [/] to de/increase the octave\n"
		"press ,/. to switch the waveform\n"
		"press escape to quit\n\n");
	if (info.dsver >= 8) // DX8 effects available
		printf("press F1-F9 to toggle effects\n\n");
	printf("using a %dms buffer\r", buflen);
	*/

	BASS_ChannelPlay(stream, FALSE);

	while (running)
	{
		// if there are any pending input events
		DWORD numEvents;
		INPUT_RECORD keyin;
		ReadConsoleInput(hIn, &keyin, 1, &numEvents);
		if (keyin.EventType != KEY_EVENT)
			continue;

		if (keyin.Event.KeyEvent.bKeyDown)
		{
			WORD code = keyin.Event.KeyEvent.wVirtualKeyCode;
			if (code == VK_ESCAPE)
			{
				running = 0;
				break;
			}
			else if (code == VK_SUBTRACT || code == VK_ADD)
			{
				// recreate stream with smaller/larger buffer
				BASS_StreamFree(stream);
				if (code == VK_SUBTRACT)
					BASS_SetConfig(BASS_CONFIG_BUFFER, buflen - 1); // smaller buffer
				else
					BASS_SetConfig(BASS_CONFIG_BUFFER, buflen + 1); // larger buffer
				buflen = BASS_GetConfig(BASS_CONFIG_BUFFER);
				PrintBufferSize(hOut, buflen);
				stream = BASS_StreamCreate(info.freq, 2, 0, (STREAMPROC*)WriteStream, 0);
				// set effects on the new stream
				for (r = 0; r < 9; r++) if (fx[r]) fx[r] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + r, 0);
				BASS_ChannelPlay(stream, FALSE);
			}
			else if (info.dsver >= 8 && code >= VK_F1 && code <= VK_F9)
			{
				r = code - VK_F1;
				if (fx[r])
				{
					BASS_ChannelRemoveFX(stream, fx[r]);
					fx[r] = 0;
					PrintEffects(hOut, fx, 9);
				}
				else
				{
					// set the effect, not bothering with parameters (use defaults)
					if (fx[r] = BASS_ChannelSetFX(stream, BASS_FX_DX8_CHORUS + r, 0))
						PrintEffects(hOut, fx, 9);
				}
			}
			else if (code == VK_OEM_4)	// '['
			{
				if (octave > 0)
				{
					--octave;
					timescale *= 0.5f;
					PrintOctave(hOut, octave);
				}
			}
			else if (code == VK_OEM_6)	// ']'
			{
				if (octave < 9)
				{
					++octave;
					timescale *= 2.0f;
					PrintOctave(hOut, octave);
				}
			}
			else if (code == VK_OEM_COMMA)
			{
				if (wavetype > 0)
				{
					--wavetype;
					PrintWaveType(hOut, wavetype);
					memset(pos, 0, KEYS * sizeof(pos[0]));
					memset(loopcount, 0, KEYS * sizeof(loopcount[0]));
				}
			}
			else if (code == VK_OEM_PERIOD)
			{
				if (wavetype < WAVE_COUNT - 1)
				{
					++wavetype;
					PrintWaveType(hOut, wavetype);
					memset(pos, 0, KEYS * sizeof(pos[0]));
					memset(loopcount, 0, KEYS * sizeof(loopcount[0]));
				}
			}
		}
		for (k = 0; k < KEYS; k++)
		if (keyin.Event.KeyEvent.wVirtualKeyCode == keys[k])
		{
			if (keyin.Event.KeyEvent.bKeyDown && vol[k] < MAXVOL)
			{
				pos[k] = 0;
				loopcount[k] = 0;
				vol[k] = MAXVOL + DECAY / 2; // start k (setting "vol" slightly higher than MAXVOL to cover any rounding-down)

				WORD attrib = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
				DWORD written;
				WriteConsoleOutputAttribute(hOut, &attrib, 1, keypos[k], &written);
			}
			else if (!keyin.Event.KeyEvent.bKeyDown && vol[k])
			{
				vol[k] -= DECAY; // trigger k fadeout

				WORD attrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
				DWORD written;
				WriteConsoleOutputAttribute(hOut, &attrib, 1, keypos[k], &written);
			}
			break;
		}
	}

	// clear the window
	Clear(hOut);

	BASS_Free();
}
