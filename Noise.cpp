#include "Noise.h"

using namespace std;

// Using this is a reference
// https://lodev.org/cgtutor/randomnoise.html

void generateNoise(float **noise, int noiseWidth, int noiseHeight)
{
	for (int y = 0; y < noiseHeight; y++)
	{
		for (int x = 0; x <  noiseWidth; x++)
		{
			noise[y][x] = (rand() % 32768) / 32768.0;
		}
	}
}

float smoothNoise(float **noise, int noiseWidth, int noiseHeight, float x, float y)
{
    float fractX = x - int(x);
    float fractY = y - int(y);

    int x1 = (int(x) + noiseWidth) % noiseWidth;
    int y1 = (int(y) + noiseHeight) % noiseHeight;

    int x2 = (x1 + noiseWidth - 1) % noiseWidth;
    int y2 = (y1 + noiseHeight - 1) % noiseHeight;

    float value = 0.0;
    value += fractX * fractY * noise[y1][x1];
    value += (1 - fractX) * fractY * noise[y1][x2];
    value += fractX * (1 - fractY) * noise[y2][x1];
    value += (1 - fractX) * (1 - fractY) * noise[y2][x2];

    return value;
}

float turbulence(float **noise, int noiseWidth, int noiseHeight, float x, float y, float size)
{
    float value = 0.0;
    float initialSize = size;

    while (size >= 1)
    {
        value += smoothNoise(noise, noiseWidth, noiseHeight, x / size, y / size) * size;
        size /= 2.0;
    }

    return (128.0 * value / initialSize);
}