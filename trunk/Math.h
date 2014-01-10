#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Math Functions
*/

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// square a value
// (useful when x is a long or compicated expression)
template<typename T> static inline T Squared(T x)
{
	return x * x;
}

// clamp value x between lower bound a and upper bound b
template<typename T> static inline T Clamp(T x, T a, T b)
{
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

// get minimum of a and b
template<typename T> static inline T Min(T a, T b)
{
	return a < b ? a : b;
}

// get maximum of a and b
template<typename T> static inline T Max(T a, T b)
{
	return a > b ? a : b;
}

// fast approximation of tanh()
static inline float FastTanh(float x)
{
	if (x < -3)
		return -1;
	if (x > 3)
		return 1;
	return x * (27 + x * x) / (27 + 9 * x * x);
}

// Fast float-to-integer conversions based on an article by Laurent de Soras
// http://ldesoras.free.fr/doc/articles/rounding_en.pdf
// The input value must be in the interval [-2**30, 2**30-1]

// fast integer round
static inline int RoundInt(float const x)
{
	float const round_nearest = 0.5f;
	int i;
	__asm
	{
		fld x;
		fadd st, st(0);
		fadd round_nearest;
		fistp i;
		sar i, 1;
	}
	return i;
}

// fast integer floor
static inline int FloorInt(float const x)
{
	float const round_minus_infinity = -0.5f;
	int i;
	__asm
	{
		fld x;
		fadd st, st(0);
		fadd round_minus_infinity;
		fistp i;
		sar i, 1;
	}
	return i;
}

// fast integer ceiling
static inline int CeilingInt(float const x)
{
	float const round_plus_infinity = -0.5f;
	int i;
	__asm
	{
		fld x;
		fadd st, st(0);
		fsubr round_plus_infinity;
		fistp i;
		sar i, 1;
	}
	return -i;
}

// fast integer truncate
static inline int TruncateInt(float const x)
{
	float const round_minus_infinity = -0.5f;
	int i;
	__asm
	{
		fld x;
		fadd st, st(0);
		fabs;
		fadd round_minus_infinity;
		fistp i;
		sar i, 1;
	}
	return x >= 0 ? i : -i;
}
