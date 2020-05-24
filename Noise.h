#ifndef H_NOISE
#define H_NOISE
#include <stdlib.h>

void generateNoise(float **noise, int noiseWidth, int noiseHeight);
float smoothNoise(float **noise, int noiseWidth, int noiseHeight, float x, float y);
float turbulence(float **noise, int noiseWidth, int noiseHeight, float x, float y, float size);

#endif