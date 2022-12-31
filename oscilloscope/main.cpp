#include <SDL2/SDL.h>
#include <iostream>
#include <unistd.h>

#include "RenderWindow.h"
#include "argparse.h"

#include "audio.h"
#include "delay.h"
#include "synth.h"
#include "filter.h"
#include "metro.h"

int screen_width;
int screen_height;
const int width = 800;
const int height = 800;
const bool highDPI = true;
const double correction = (highDPI ? 1 : 0.5);
bool fullscreen = false;
bool mouse = true;

#define DARKNESS 255
#define ALPHA 64
#define LINEALPHA 96
#define LIGHTNESS 0

#define BSIZE 64
#define FRAMERATE 120
#define INCREMENT 1

#define CHROMATIC 12
#define OCTAVES 3
#define A0 21

using namespace soundmath;

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

SDL_BlendMode polyblend = SDL_ComposeCustomBlendMode(
	SDL_BLENDFACTOR_SRC_ALPHA,
	SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,// SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	SDL_BLENDOPERATION_MAXIMUM,
	SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ZERO, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	SDL_BLENDOPERATION_MINIMUM);

const int waveSize = SR / FRAMERATE;

SDL_FPoint waveform[2 * waveSize];
SDL_FPoint Loffsets[2 * waveSize];
SDL_FPoint Roffsets[2 * waveSize];
SDL_FPoint normals[2 * waveSize];

SDL_Vertex trackverts[2 * waveSize * 6];

SDL_Rect fillRect = { 0, 0, (int)(width * 2 * correction), (int)(height * 2 * correction) };

const int pushoff = 192;

double phase = 0;
double mod = 0;
double modfreq = 1.5 * 0.75; // 0.75;
double freq = 1.5 * 0.05; // rate at which oscillator completes revolution

int waveOrigin = 0;
double gain = 5;
int dtime = SR / 20;

Delay<double> chandelay(1, SR);

bool ready = false;
bool flipped = false;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[i] = 0;
		double the_sample = in[in_chans * i + in_channel];
		waveform[waveOrigin + waveSize * flipped] = 
			SDL_FPoint{
				float((1 + highDPI) * (1 + gain * the_sample) / 2 * screen_width),
				float((1 + highDPI) * (1 + gain * chandelay(the_sample)) / 2 * screen_height)
			};

		waveOrigin--;
		if (waveOrigin < 0)
		{
			ready = true;
			flipped = !flipped;
			waveOrigin += waveSize;
		}

		chandelay.tick();
	}

	chandelay.coefficients({{dtime,1}},{});

	return 0;
}

enum Smoothing 
{
	constant,
	L2,
	L1,
	L4,
	LHalf,
	LInf
};

const double smooth = 256; // 64;

double norm(double x, double y, Smoothing normtype = L2, double smoothing = smooth / waveSize)
{
	switch (normtype)
	{
		case constant: return 1;
		case L2: return sqrt(smoothing * smoothing + x * x + y * y);
		case L1: return abs(smoothing) + abs(x) + abs(y);
		case L4: return pow(smoothing * smoothing * smoothing * smoothing + x * x * x * x + y * y * y * y, 0.25);
		case LHalf: return pow(sqrt(abs(smoothing)) + sqrt(abs(x)) + sqrt(abs(y)), 2);
		case LInf: return std::max(abs(smoothing), std::max(abs(x), abs(y)));
	}
}


// fill an array of (hopefully unit) normals
void gauss(SDL_FPoint* in, SDL_FPoint* normals, int count)
{
	const Smoothing normtype = L2;
	double dx, dy, scale; 

	dx = (double)in[1].x - (double)in[0].x;
	dy = (double)in[1].y - (double)in[0].y;

	scale = norm(dx, dy, normtype); // sqrt(dx * dx + dy * dy);
	assert(scale != 0);

	normals[0] = { (float)(dy / scale), (float)(-dx / scale) };

	dx = in[count - 1].x - in[count - 2].x;
	dy = in[count - 1].y - in[count - 2].y;

	scale = norm(dx, dy, normtype); 
	assert(scale != 0);

	normals[count - 1] = { (float)(dy / scale), (float)(-dx / scale) };

	for (int i = 1; i < count - 1; i++)
	{
		double dx1, dy1, dx2, dy2, scale1, scale2;

		dx1 = (double)in[i].x - (double)in[i - 1].x;
		dy1 = (double)in[i].y - (double)in[i - 1].y;
		dx2 = (double)in[i + 1].x - (double)in[i].x;
		dy2 = (double)in[i + 1].y - (double)in[i].y;

		scale1 = norm(dx1, dy1, normtype); 
		scale2 = norm(dx2, dy2, normtype); 

		assert(scale1 != 0);
		assert(scale2 != 0);

		dx = (dx1 / scale1 + dx2 / scale2) / 2;
		dy = (dy1 / scale1 + dy2 / scale2) / 2;

		// scale = norm(dx, dy);
		scale = 1; // some flexibility -- nicer corners!
		normals[i] = { (float)(dy / scale), (float)(-dx / scale) };
	}
}

void move(SDL_FPoint* in, SDL_FPoint* out, SDL_FPoint* normals, double push, int count)
{
	for (int i = 0; i < count; i++)
	{
		out[i] = { (float)(in[i].x + push * normals[i].x), (float)(in[i].y + push * normals[i].y) };
	}
}


int main(int argc, char* argv[])
{
	args(argc, argv);

	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	screen_width = fullscreen ? DM.w : width;
	screen_height = fullscreen ? DM.h : height;

	RenderWindow window("Oscilloscope", screen_width, screen_height, highDPI, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); 
	// window.blend(SDL_BLENDMODE_ADD);

	if (mouse)
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	else
		SDL_SetRelativeMouseMode(SDL_TRUE);
	
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
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					dtime += 1;
					dtime = std::min(dtime, SR - 1);
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					dtime -= 1;
					dtime = std::max(100, dtime);
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					gain -= 0.05;
				}
				else if (event.key.keysym.sym == SDLK_UP) 
				{
					gain += 0.05;
				}

				if (event.key.keysym.sym == SDLK_f)
				{
					fullscreen = !fullscreen;
					if (fullscreen)
						SDL_SetWindowFullscreen(window.sdl_window(), SDL_WINDOW_FULLSCREEN_DESKTOP);
					else
						SDL_SetWindowFullscreen(window.sdl_window(), 0);

					SDL_GetCurrentDisplayMode(0, &DM);

					screen_width = fullscreen ? DM.w : width;
					screen_height = fullscreen ? DM.h : height;
				}

				if (event.key.keysym.sym == SDLK_m)
				{
					mouse = !mouse;
					if (mouse)
					{
						SDL_SetRelativeMouseMode(SDL_FALSE);
					}
					else
						SDL_SetRelativeMouseMode(SDL_TRUE);
				}

			}
		}

		gauss(waveform + waveSize * (!flipped), normals + waveSize * (!flipped), waveSize);
		move(waveform + waveSize * (!flipped), Loffsets + waveSize * (!flipped), normals + waveSize * (!flipped), pushoff, waveSize);
		move(waveform + waveSize * (!flipped), Roffsets + waveSize * (!flipped), normals + waveSize * (!flipped), -pushoff, waveSize);

		int j = waveSize * (!flipped) * 6;
		SDL_Color color;
		for (int i = 0; i < waveSize - 1; i++)
		{
			unsigned char R = (unsigned char)(255 * (1 + sin(2 * PI * (0.0 / 3 * cos(mod * freq / modfreq) / 2 + 1 * (double)i / waveSize ))) / 2);
			unsigned char G = (unsigned char)(255 * (1 + sin(2 * PI * (1.0 / 3 * cos(mod * freq / modfreq) / 2 + 1 * (double)i / waveSize ))) / 2);
			unsigned char B = (unsigned char)(255 * (1 + sin(2 * PI * (2.0 / 3 * cos(mod * freq / modfreq) / 2 + 1 * (double)i / waveSize ))) / 2);
			// unsigned char B = 0;

			color = SDL_Color { R, G, B, (unsigned char)(12) };
			// color = SDL_Color{ 255, 255, 255, 12 };

			trackverts[j++] = { SDL_FPoint{ (Roffsets + waveSize * (!flipped))[i].x, (Roffsets + waveSize * (!flipped))[i].y }, color, SDL_FPoint{ 0 } };
			trackverts[j++] = { SDL_FPoint{ (Roffsets + waveSize * (!flipped))[i + 1].x, (Roffsets + waveSize * (!flipped))[i + 1].y }, color, SDL_FPoint{ 0 } };
			trackverts[j++] = { SDL_FPoint{ (Loffsets + waveSize * (!flipped))[i].x, (Loffsets + waveSize * (!flipped))[i].y }, color, SDL_FPoint{ 0 } };

			trackverts[j++] = { SDL_FPoint{ (Loffsets + waveSize * (!flipped))[i].x, (Loffsets + waveSize * (!flipped))[i].y }, color, SDL_FPoint{ 0 } };
			trackverts[j++] = { SDL_FPoint{ (Loffsets + waveSize * (!flipped))[i + 1].x, (Loffsets + waveSize * (!flipped))[i + 1].y }, color, SDL_FPoint{ 0 } };
			trackverts[j++] = { SDL_FPoint{ (Roffsets + waveSize * (!flipped))[i + 1].x, (Roffsets + waveSize * (!flipped))[i + 1].y }, color, SDL_FPoint{ 0 } };
		}

		// trackverts[waveSize * (!flipped) * 6 - 6] = { SDL_FPoint{ (Roffsets + waveSize * (flipped))[0].x, (Roffsets + waveSize * (flipped))[0].y }, color, SDL_FPoint{ 0 } };
		// trackverts[waveSize * (!flipped) * 6 - 5] = { SDL_FPoint{ (Roffsets + waveSize * (flipped))[0 + 1].x, (Roffsets + waveSize * (flipped))[0 + 1].y }, color, SDL_FPoint{ 0 } };
		// trackverts[waveSize * (!flipped) * 6 - 4] = { SDL_FPoint{ (Loffsets + waveSize * (flipped))[0].x, (Loffsets + waveSize * (flipped))[0].y }, color, SDL_FPoint{ 0 } };
		// trackverts[waveSize * (!flipped) * 6 - 3] = { SDL_FPoint{ (Loffsets + waveSize * (flipped))[0].x, (Loffsets + waveSize * (flipped))[0].y }, color, SDL_FPoint{ 0 } };
		// trackverts[waveSize * (!flipped) * 6 - 2] = { SDL_FPoint{ (Loffsets + waveSize * (flipped))[0 + 1].x, (Loffsets + waveSize * (flipped))[0 + 1].y }, color, SDL_FPoint{ 0 } };
		// trackverts[waveSize * (!flipped) * 6 - 1] = { SDL_FPoint{ (Roffsets + waveSize * (flipped))[0 + 1].x, (Roffsets + waveSize * (flipped))[0 + 1].y }, color, SDL_FPoint{ 0 } };

		window.color(0, 0, 0);
		window.clear();

		while (!ready)
		{
			usleep(100);
		}

		window.color(0, 0, 0, 0.5);
		// window.blend(polyblend);
		window.color(0, 0, 0, 0);
		window.rectangle(&fillRect);
		window.geometry(trackverts + waveSize * (!flipped) * 6, waveSize * 6);
		// window.blend(SDL_BLENDMODE_BLEND);

		window.color(1, 1, 1, 0.5);
		
		// window.color(0, 0, 0, 0.5);
		window.curve(waveform + waveSize * (!flipped), waveSize);
		// window.curve(Loffsets + waveSize * (!flipped), waveSize);
		// window.curve(Roffsets + waveSize * (!flipped), waveSize);
		ready = false;

		mod += modfreq / FRAMERATE;
		phase += freq / FRAMERATE;
		phase -= int(phase);


		window.display();
	}

	A.shutdown(); // shutdown audio engine
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
