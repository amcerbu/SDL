// Audio.cpp
#include "Audio.h"
#include "Performer.h"
#include "Score.h"
#include "metro.h"

using namespace soundmath;

const int n = 12;
const int overtones = 7;
const int transp = 36;
const double decay = 6000;
const double darkness = 0.3;
const int filter_order = 1;
Performer p(n, overtones, transp, decay, darkness, filter_order);

Metro<double> metro(10); 
Score score("score2.txt");
ArrayCT chord(n);

const int width = 2; // setting this to 12 gives mono
MatrixT kernel(n, n);

void Audio::prepare()
{
	kernel.setZero();
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < n; j++)
		{
			kernel(j, (i + j) % n) = 1;
			kernel(j, (i + j + 1) % n) = 1;
			kernel(j, (n + i + j - 1) % n) = 1;
		}
	}
	kernel.rowwise().normalize();

	for (int i = 0; i < width; i++)
	{
		kernel = kernel * kernel;
		kernel.rowwise().normalize();
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			std::cout << kernel(i,j) << " ";
		}
		std::cout << std::endl;
	}
}

int Audio::process(const float* in, float* out, unsigned long frames)
{
	for (int i = 0; i < frames; i++)
	{
		if (metro())
		{
			std::vector<int> notes = score.read();
			std::vector<T> loader(n, 0);
			chord.setZero();
			for (int a : notes)
			{
				loader[a % n] = a;
				chord(a % n) = 1;
			}

			p.full_mod(loader);
		}
		ArrayCT input = metro() * chord;
		ArrayCT output = *(p(&input));

		// distribute voices across channels
		ArrayCT distributed = (kernel * output.matrix()).array();
		for (int j = 0; j < out_chans; j++)
		{
			out[j + i * out_chans] = distributed(int(n * (T)j / out_chans)).real();
		}

		p.tick();
		metro.tick();
	}

	return 0;
}
