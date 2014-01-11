#pragma once
/*
MINI VIRTUAL ANALOG SYNTHESIZER
Copyright 2014 Kenneth D. Miller III

Polynomial Bandlimited Step
*/

// width of bandlimited step relative to the sample period
const float POLYBLEP_WIDTH = 1.5f;

// Valimaki/Huovilainen PolyBLEP
inline float PolyBLEP(float t, float w)
{
	if (t >= w)
		return 0.0f;
	if (t <= -w)
		return 0.0f;
	t /= w;
	if (t > 0.0f)
		return t + t - t * t - 1;
	else
		return t * t + t + t + 1;
}

// width of integrated bandlimited step relative to the sample period
const float INTEGRATED_POLYBLEP_WIDTH = 1.5f;

// symbolically-integrated PolyBLEP
inline float IntegratedPolyBLEP(float t, float w)
{
	if (t >= w)
		return 0.0f;
	if (t <= -w)
		return 0.0f;
	t /= w;
	float const t2 = t * t;
	float const t3 = t2 * t;
	if (t > 0.0f)
		return (0.33333333f - t + t2 - 0.33333333f * t3) * 4 * w;
	else
		return (0.33333333f + t + t2 + 0.33333333f * t3) * 4 * w;
}
