#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Random Number Generator
*/

namespace Random
{
	// random seed
	extern unsigned int gSeed;

	// set seed
	inline void Seed(unsigned int aSeed)
	{
		gSeed = aSeed;
	}

	// random unsigned integer
	inline unsigned int Int()
	{
#if 1
		// 32-bit xor-shift generator
		// based on an implementation presented in a paper by George Marsaglia:
		// http://www.jstatsoft.org/v08/i14/paper
		gSeed ^= (gSeed << 13);
		gSeed ^= (gSeed >> 17);
		gSeed ^= (gSeed << 5);
#else
		gSeed = 1664525L * gSeed + 1013904223L;
#endif
		return gSeed;
	}

	// random uniform float
	inline float Float()
	{
		union { float f; unsigned u; } floatint;
		floatint.u = 0x3f800000 | (Int() >> 9);
		return floatint.f - 1.0f;
	}
}
