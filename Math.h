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

// linear interpolation
template<typename T> inline T Lerp(const T v0, const T v1, const float s)
{
	return v0 + (v1 - v0) * s;
}

// fast approximation of tanh()
static inline float FastTanh(float x)
{
#if 0
	if (fabsf(x) < 1.5f)
		return x - 0.14814814814814814814814814814815f * x * x * x;
	return x > 0 ? 1 : -1;
#else
	if (x < -3)
		return -1;
	if (x > 3)
		return 1;
	return x * (27 + x * x) / (27 + 9 * x * x);
#endif
}

// Fast float-to-integer conversions based on an article by Laurent de Soras
// http://ldesoras.free.fr/doc/articles/rounding_en.pdf
// The input value must be in the interval [-2**30, 2**30-1]

#if _M_IX86_FP > 0
#include <xmmintrin.h>
#endif

// fast integer round
static inline int RoundInt(float const x)
{
#if 0//_M_IX86_FP > 0
	return int(roundf(x));
#elif _M_IX86_FP > 0
	return _mm_cvt_ss2si(_mm_set_ss(x + x + 0.5f)) >> 1;
#else
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
#endif
}

// fast integer floor
static inline int FloorInt(float const x)
{
#if 0//_M_IX86_FP > 0
	return int(floorf(x));
#elif _M_IX86_FP > 0
	return _mm_cvt_ss2si(_mm_set_ss(x + x - 0.5f)) >> 1;
#else
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
#endif
}

// fast integer ceiling
static inline int CeilingInt(float const x)
{
#if 0//_M_IX86_FP > 0
	return int(ceilf(x));
#elif _M_IX86_FP > 0
	return -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (x + x))) >> 1);
#else
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
#endif
}

// fast integer truncate
static inline int TruncateInt(float const x)
{
#if _M_IX86_FP > 0
	return _mm_cvtt_ss2si(_mm_set_ss(x));
#else
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
#endif
}
