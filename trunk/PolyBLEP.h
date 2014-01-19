#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Polynomial Bandlimited Step
*/

// width of bandlimited step relative to the sample period
const float POLYBLEP_WIDTH = 1.5f;

// approximation of 0.5 * s * (sin((t * M_PI) / (w + w)) - sign(t))

// cubic version:
// +: (3t - t^3 - 2) * s * 0.25f
// -: (3t - t^3 + 2) * s * 0.25f

// Valimaki/Huovilainen PolyBLEP
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

// width of integrated bandlimited step relative to the sample period
const float INTEGRATED_POLYBLEP_WIDTH = 1.5f;

// symbolically-integrated PolyBLEP
inline float IntegratedPolyBLEP(float t, float w, float s)
{
	if (fabsf(t) >= w)
		return 0.0f;
	t /= w;
	float const t2 = t * t;
	float const t3 = t2 * t;
	float t3t = t3 * 0.33333333f + t;
	if (t >= 0)
		t3t = -t3t;
	return (t3t + t2 + 0.33333333f) * 0.5f * s * w;
}

// special case for a slope step of 8 units (triangle -4 to +4)
#if 0
#define IntegratedPolyBLEP(t, w) IntegratedPolyBLEP(t, w, 8)
#else
inline float IntegratedPolyBLEP(float t, float w)
{
	if (fabsf(t) >= w)
		return 0.0f;
	t /= w;
	float const t2 = t * t;
	float const t3 = t2 * t;
	float t3t = t3 * 0.33333333f + t;
	if (t >= 0)
		t3t = -t3t;
	return (t3t + t2 + 0.33333333f) * 4 * w;
}
#endif