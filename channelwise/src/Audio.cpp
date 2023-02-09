// Audio.cpp
#include "Audio.h"

int Audio::process(const float* in, float* out, unsigned long frames)
{
	for (int i = 0; i < frames; i++)
	{
		for (int j = 0; j < out_chans; j++)
		{
			// your audio code here
			out[j + i * out_chans] = 0;
		}
	}

	return 0;
}
