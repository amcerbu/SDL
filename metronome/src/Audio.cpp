// Audio.cpp
#include "Audio.h"
#include "metro.h"
#include "synth.h"
#include "filter.h"
#include "noise.h"

Metro<double> metronome(2); // 120 bpm
// Filter<double> filter({1,0,-1}, {0,-0.999});
Filter<double> filter({1},{0,-0.999});
Filter<double> filter2({0.1},{0,-0.9});
Noise<double> noise;
double gain = 1;
double the_sample;

int elapsed;
int lag;

int Audio::process(const float* in, float* out, unsigned long frames)
{
	for (int i = 0; i < frames; i++)
	{
		// if (metronome())
		// {
		// 	gain = 1;
		// 	lag = elapsed; // count samples between metro clicks
		// 	elapsed = 0;
		// }
		// // else
		// // {
		// // 	gain = 0;
		// // }

		// gain *= 0.995;
		// elapsed++;

		// the_sample = 0.75 * filter(0.5 * gain) * noise();
		// // the_sample = filter(metronome());

		the_sample = 0.75 * filter(metronome()) * filter2(noise());
		
		for (int j = 0; j < out_chans; j++)
		{
			out[j + i * out_chans] = (float)the_sample;
		}

		metronome.tick();
		filter.tick();
		filter2.tick();
		noise.tick();
	}

	return 0;
}
