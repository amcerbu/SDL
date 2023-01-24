// Audio.cpp
#include "Audio.h"
#include "fourier.h"

// #define N 8192
// #define N 16384
// #define N 32768
#define N 65536
// #define N 262144
#define LAPS 4
// #define LAPS 32
#define THRESH 275 // Hz

const int thresh_index = (int)((double)THRESH * N / SR);
bool to_cut;

inline int f_process(const std::complex<double>* in, std::complex<double>* out)
{
	if (to_cut)
	{	
		for (int i = 0; i < thresh_index; i++)
		{
			out[i] = 0;
			out[N - i - 1] = 0;
		}
		for (int i = thresh_index; i < N / 2; i++)
		{
			out[i] = in[i];
			out[N - i - 1] = in[N - i - 1];
		}
	}
	else
	{
		for (int i = thresh_index; i < N / 2; i++)
		{
			out[i] = 0;
			out[N - i - 1] = 0;
		}
		for (int i = 0; i < thresh_index; i++)
		{
			out[i] = in[i];
			out[N - i - 1] = in[N - i - 1];
		}
	}

	return 0;
}

Fourier F1 = Fourier(f_process, N, LAPS);
Fourier F2 = Fourier(f_process, N, LAPS);
double sample1 = 0;
double sample2 = 0;

int Audio::process(const float* in, float* out, unsigned long frames)
{
	to_cut = cut;
	for (int i = 0; i < frames; i++)
	{
		F1.write(in[2 * i]);
		F2.write(in[2 * i + 1]);

		double real, imag;
		F1.read(&real, &imag);
		sample1 = out[2 * i] = real; // + delay1(in[2 * i]));

		F2.read(&real, &imag);
		sample2 = out[2 * i + 1] = real; // + delay2(in[2 * i + 1]));;
	}

	return 0;
}
