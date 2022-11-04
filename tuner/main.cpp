#include <SDL2/SDL.h>
#include <iostream>
#include <unistd.h>

#include "RenderWindow.h"
#include "argparse.h"

#include "audio.h"
#include "oscbank.h"
#include "slidebank.h"
#include "modbank.h"
#include "mixer.h"
#include "argparse.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;

#define DARKNESS 255
#define ALPHA 64
#define LINEALPHA 96
#define LIGHTNESS 0

#define BSIZE 64
#define FRAMERATE 60
#define INCREMENT 1

#define CHROMATIC 12
#define OCTAVES 5
#define A0 21

using namespace soundmath;

int CHANS;
int INPUT;
int DEVICE;

void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("filterer");

	int def_in = 0;
	int def_out = 0;
	Audio::initialize(false, &def_in, &def_out);

	program.add_argument("-i", "--input")
		.default_value<int>((int)def_in)
		.required()
		.scan<'i', int>()
		.help("device id for audio in");

	program.add_argument("-f", "--framesize")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per frame");

	program.add_argument("-c", "--channel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel");

	program.add_argument("-d", "--devices")
		.help("list audio device names")
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

	DEVICE = program.get<int>("-i");
	CHANS = program.get<int>("-f");
	INPUT = program.get<int>("-c");
	Audio::initialize(!(program.is_used("-i") && program.is_used("-f") && program.is_used("-c")) || program.is_used("-d"));
}


const int waveSize = (SR / INCREMENT) / FRAMERATE;
SDL_FPoint waveforms[2 * waveSize * CHROMATIC * OCTAVES];
int waveOrigin = 0;
double gain = 5.0;
double radius = 0.9; // 
double ratio = 0.75; // 
double offset = 0.0;

bool ready = false;
bool flipped = false;

int pitch = 24;

// double filter_r = 0.9999;
// double filter_r = 0.996;
// int multiplicity = 3;
double filter_r = 0.996;
int multiplicity = 3;
Oscbank<double, CHROMATIC * OCTAVES> oscbank;
Slidebank<double, CHROMATIC * OCTAVES> slidebank;
Modbank<double, CHROMATIC * OCTAVES> modulators; // modulation

double xs[CHROMATIC * OCTAVES];
double ys[CHROMATIC * OCTAVES];
double rs[CHROMATIC * OCTAVES];

void init_graphics()
{
	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		// rs[j] = (radius * pow(ratio, (double)j / CHROMATIC)); // logarithmic spiral
		// rs[j] = (radius * pow(ratio, j / CHROMATIC)); // logarithmic spiral
		rs[j] = radius * (1 - 1.0 / (OCTAVES) * ((double)j / CHROMATIC));
		xs[j] = rs[j] * sin((2 * PI * j * 1) / CHROMATIC);
		ys[j] = rs[j] * -cos((2 * PI * j * 1) / CHROMATIC);
	}

	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		double x = ((xs[j] + 1) / 2) * width * (1 + highDPI); 
		double y = ((ys[j] + 1) / 2) * height * (1 + highDPI);
		waveforms[2 * waveSize * j + waveSize * (true) + waveOrigin] = 
		waveforms[2 * waveSize * j + waveSize * (false) + waveOrigin] =
			SDL_FPoint{ float(x), float(y) };
	}
}

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i += INCREMENT)
	{
		out[i] = 0;
		double the_sample = in[CHANS * i + INPUT];
		// double sign = (the_sample > 0 ? 1 : (the_sample < 0 ? -1 : 0));
		// double normalized = the_sample * sign;

		// slidebank(modulators(offset + sign * sqrt(normalized), oscbank())); // for shapes!
		slidebank(modulators(offset + the_sample, oscbank()));
		for (int j = 0; j < CHROMATIC * OCTAVES; j++)
		{
			// std::complex<double> point = (*slidebank())(j) * (*oscbank())(j); // gives circles
			std::complex<double> point = (*slidebank())(j) * (1.0 + (*oscbank())(j)); // gives spinning circles
			// std::complex<double> point = (*slidebank())(j); // gives stationary points
			// std::complex<double> point = (*slidebank())(j) * (*oscbank())(j) * sqrt(normalized); // gives stationary points
			// double x = ((xs[j] + 1 + rs[j] * gain * point.real()) / 2) * width; 
			double x = ((xs[j] + 1 + gain * point.real()) / 2) * width * (1 + highDPI);
			// double y = ((ys[j] + 1 - rs[j] * gain * point.imag()) / 2) * height;
			double y = ((ys[j] + 1 - gain * point.imag()) / 2) * height * (1 + highDPI);

			waveforms[2 * waveSize * j + waveSize * flipped + waveOrigin] = 
				SDL_FPoint{ float(x), float(y) };
		}

		waveOrigin--;
		if (waveOrigin < 0)
		{
			ready = true;
			flipped = !flipped;
			waveOrigin += waveSize;
		}

		oscbank.tick();
		slidebank.tick();
	}

	return 0;
}

void coefficients(int base)
{
	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		oscbank.freqmod(j, 55 * pow(2, ((double)(base + j - A0)) / CHROMATIC));
	}
}

Audio A = Audio(process, BSIZE);


int main(int argc, char* argv[])
{
	args(argc, argv);
	init_graphics();

	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	RenderWindow window("Tuner", width, height, highDPI); 

	slidebank.setup(multiplicity, std::vector<std::complex<double>>(CHROMATIC * OCTAVES, filter_r));
	oscbank.open();
	slidebank.open();

	coefficients(pitch);
	A.startup(CHANS, 1, true, DEVICE); // startup audio engine

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
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					pitch -= 12;
					coefficients(pitch);
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					pitch += 12;
					coefficients(pitch);
					// osc.freqmod(mtof(pitch));
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					gain -= 0.05;
				}
				else if (event.key.keysym.sym == SDLK_UP) 
				{
					gain += 0.05;
				}
			}
		}

		// static SDL_Rect my_rect { 0, 0, width * (1 + highDPI), height * (1 + highDPI) };
		window.color(1, 1, 1);
		// window.rectangle(&my_rect);

		window.clear();

		while (!ready)
		{
			usleep(100);
		}

		window.color(0, 0, 0, 0.5);
		for (int j = 0; j < CHROMATIC * OCTAVES; j++)
		{
			window.curve(waveforms + 2 * waveSize * j + waveSize * (!flipped), waveSize);
		}
		ready = false;


		window.display();
	}

	A.shutdown(); // shutdown audio engine
	window.~RenderWindow();

	SDL_Quit();

	return 0;
}

