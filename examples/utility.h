#pragma once

#include <cmath>


static const float PI = 3.1415926535f;
static const float TWO_PI = 2.f * PI;


static float Wrap0to1(float x)
{
	return x - std::floor(x);
}

static float WhiteNoise()
{
	return (std::rand() / float(RAND_MAX)) * 2.0f - 1.0f;
}
