#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"
#include "audio.h"
#include "argparse.h"
#include "synth.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;

const int bsize = 64;
int in_chans;
int out_chans;
int in_channel;
int in_device;
int out_device;

using namespace soundmath;

void args(int argc, char *argv[]);
inline int process(const float* in, float* out);
Audio A = Audio(process, bsize);

int main(int argc, char* argv[])
{
	args(argc, argv);
	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	RenderWindow window("Fm", width, height, highDPI); 

	A.startup(in_chans, out_chans, true, in_device, out_device); // startup audio engine

	bool running = true;
	SDL_Event event;

	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = false;
			}

			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					running = false;
				}
			}
		}

		window.clear();

		window.display();
	}

	A.shutdown();
	window.~RenderWindow();

	SDL_Quit();

	return 0;
}

void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("Fm");

	int def_in = 0;
	int def_out = 0;
	Audio::initialize(false, &def_in, &def_out);

	program.add_argument("-i", "--input")
		.default_value<int>((int)def_in)
		.required()
		.scan<'i', int>()
		.help("device id for audio in");

	program.add_argument("-if", "--in_framesize")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per frame of input");

	program.add_argument("-c", "--channel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel selector (in [0, if - 1])");

	program.add_argument("-o", "--output")
		.default_value<int>((int)def_out)
		.required()
		.scan<'i', int>()
		.help("device id for audio out");

	program.add_argument("-of", "--out_framesize")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per frame of output");

	program.add_argument("-d", "--devices")
		.help("list audio device names and exits")
		.default_value(false)
		.implicit_value(true);

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	in_device = program.get<int>("-i");
	in_chans = program.get<int>("-if");
	in_channel = program.get<int>("-c");
	out_device = program.get<int>("-o");
	out_chans = program.get<int>("-of");

	Audio::initialize(program.is_used("-d"));

	if (program.is_used("-d"))
	{
		std::exit(1);
	}
}

double freqs[] = {110, 220, 330, 440, 550};

Synth<double> osc1(&cycle, freqs[0]);
Synth<double> osc2(&cycle, freqs[1]);
Synth<double> osc3(&cycle, freqs[2]);
Synth<double> osc4(&cycle, freqs[3]);
Synth<double> osc5(&cycle, freqs[4]);
Synth<double>* oscs[] = {&osc1, &osc2, &osc3, &osc4, &osc5};

Synth<double> lfo(&cycle, 0.05);
double depth;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < bsize; i++)
	{
		double the_sample = (osc1() + osc2() + osc3() + osc4() + osc5()) / 10;
		depth = 1100 * (1 + lfo()) / 2;
		// depth = freqs[4];
		// depth = 0;

		for (int j = 0; j < 5; j++)
		{
			for (int k = j + 1; k < 5; k++)
			{
				oscs[j]->freqmod(freqs[j] + depth * (*oscs[k])());
			}
		}

		for (int j = 0; j < out_chans; j++)
		{
			out[j + out_chans * i] = float(the_sample);
		}

		for (int j = 0; j < 5; j++)
		{
			oscs[j]->tick();
		}

		lfo.tick();
	}

	return 0;
}
