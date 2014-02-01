#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Polynomial Bandlimited Step
(based on Valimaki/Huovilainen PolyBLEP)
*/

// PolyBLEP
// Polynomial correction for C0 (value) discontinuity

// width of bandlimited step relative to the sample period
// this value was determined through experimentation
// 1 is not enough, allowing high harmonics to alias
// 2 is too much, suppressing high harmonics below Nyquist
const float POLYBLEP_WIDTH = 1.5f;

// the ideal bandlimited step is based on the sine integral
// (which is the integral of the sinc function)
// http://mathworld.wolfram.com/SineIntegral.html
// http://en.wikipedia.org/wiki/Trigonometric_integral
// 0.5 s (2 Si(pi t) / pi - sign(s)), where s is the step height
// Si(pi t) = sum k=1..infinity -(-1)^k (pi t)^(2k-1) / ((2k-1)(2k-1)!)

// many PolyBLEP use this piecewise quadratic curve:
//     2x - sign(x)(x^2)

// it's simple, fast, and surprisingly effective...
// (it suppress higher harmonics a bit too much, though)

// general case for value step of s units
inline float PolyBLEP(float t, float w, float s)
{
	if (fabsf(t) >= w)
		return 0.0f;
	t /= w;
	float tt1 = (t * t + 1) * 0.5f;
	if (t >= 0)
		tt1 = -tt1;
	return (tt1 + t) * s;
}

#if 0
// Hermite cubic spline approximation to the integral of a gaussian-windowed sinc
//     exp(-k^2/16) sin(k) / k where k = pi * t
//
// This preserves high-frequency harmonics better than the quadratic "smooth step"
// version but needs a wider sample window than the oscillators currently support
inline float PolyBLEP(float t, float w)
{
	if (fabsf(t) >= w + w)
		return 0.0f;
	t /= w;
	float const t2 = t * t;
	float const t3 = t2 * t;
	//float const t4 = t3 * t;
	if (t < -w)
		return 0.3333333f + t + 0.75f * t2 + 0.1666667f * t3;	//1.3333333f + 4 * t + 4 * t2 + 1.6666667f * t3 + 0.25f * t4;
	else if (t < 0)
		return 1 + 2 * t + 0.75f * t2 - 0.1666667f * t3;	//1 + t - 1.6666667f * t3 - 0.75f * t4;
	else if (t < w)
		return -1 + 2 * t - 0.75f * t2 - 0.1666667f * t3;	//-1 + t - 1.6666667f * t3 + 0.75f * t4;
	else
		return -0.3333333f + t - 0.75f * t2 + 0.1666667f * t3;	// -1.3333333f + 4 * t - 4 * t2 + 1.66666667f * t3 - 0.25f * t4;
}
#endif

// special case for a value step of 2 units (pulse/sawtooth -1 to +1)
inline float PolyBLEP(float t, float w)
{
	if (fabsf(t) >= w)
		return 0.0f;
	t /= w;
	float tt1 = t * t + 1;
	if (t >= 0)
		tt1 = -tt1;
	return tt1 + t + t;
}

// IntegratedPolyBLEP
// Polynomial correction for C1 (slope) discontinuity

// width of integrated bandlimited step relative to the sample period
const float INTEGRATED_POLYBLEP_WIDTH = 1.0f;

// approximation of s * (w * (1 - 2 / pi * cos(t * pi / (2 * w)) - fabsf(t))

// general case for a slope step of s units
inline float IntegratedPolyBLEP(float const t, float const w, float const s)
{
	float at = fabsf(t);
	if (at >= w)
		return 0.0f;
	at /= w;
	float const t2 = at * at;
#if 1
	float const t4 = t2 * t2;
	return (0.375f - at + 0.75f * t2 - 0.125f * t4) * w * s * 0.5f;
#else
	float const t3 = t2 * at;
	return (0.33333333f - at + t2 - 0.33333333f * t3) * w * s * 0.5f;
#endif
}

// special case for a slope step of 8 units (triangle -4 to +4)
inline float IntegratedPolyBLEP(float const t, float const w)
{
	float at = fabsf(t);
	if (at >= w)
		return 0.0f;
	at /= w;
	float const t2 = at * at;
#if 1
	float const t4 = t2 * t2;
	return (0.375f - at + 0.75f * t2 - 0.125f * t4) * w * 4;
#else
	float const t3 = t2 * at;
	return (0.33333333f - at + t2 - 0.33333333f * t3) * w * 4;
#endif
}
