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
const float POLYBLEP_WIDTH = 1.5f;

// approximation of 0.5 * s * (sin((t * M_PI) / (w + w)) - sign(t))

// cubic version:
// (this doesn't seem to work as well as the quadratic version
// +: (3t - t^3 - 2) * s * 0.25f
// -: (3t - t^3 + 2) * s * 0.25f
//	s *= 0.5f;
//	float const p = (3 - t * t) * t * 0.5f * s;
//	return t >= 0 ? p - s : p + s;

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
