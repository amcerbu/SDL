#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <unistd.h>

#include "RenderWindow.h"
#include "Color.h"
#include "Disk.h"
#include "Score.h"
#include "audio.h"
#include "Performer.h"
#include "metro.h"
#include "argparse.h"

// using namespace soundmath;

const bool highDPI = true;
bool fullscreen = false;
const double correction = (highDPI ? 2 : 1);
const bool audio = false;

const int width = 800;
const int height = 800;

const double radius = 30;
const double dot = radius * 0.25;
const double bigd = radius * 0.5;

const int fingers = 12;
const double density = 1.25;
const int disks = 12; // density * width * height / (PI * radius * radius);
const double inflation = radius * 2;

const double rigidity = 0.1;
const double wallrigidity = 0.05;
const double bindanim = 0.95;
const double floatanim = 0.8;
const double grabanim = 0.9;

const double restingval = 1.0 / 6;
const double excitedval = 1.0 / 3;

const double restingsat = 0.0 / 3; // 2.0 / 3;
const double excitedsat = 2.0 / 3; // 0.0 / 3;

const double alpha = 1; // 2.0 / 3;

const double friction = 0.8;
const int oversample = 1;


Disk grabbers[fingers];
bool active[fingers];

SDL_FingerID ids[fingers];

Disk floaters[disks];
int grabbed[disks]; // floater #i is grabbed by finger grabbed[i]
int grabbing[fingers]; // finger #i is grabbing floater grabbing[i]
double bind[fingers];
double targbind[fingers];

double basehues[disks];

int num_fingers = 0;

using namespace soundmath;

const int bsize = 2048;
int in_chans;
int out_chans;
int in_channel;
int in_device;
int out_device;

void args(int argc, char *argv[]);
inline int process(const float* in, float* out);
Audio A = Audio(process, bsize);
const int n = 6;
Score score("score1.txt");
Performer perf(n, 1, 36, 0.999, 0.9999, 0.7);
Metro<double> metro(10);
double gain = 1;

double clip(double x)
{
	return x < 0 ? 0 : (x > 1 ? 1 : x);
}

int main(int argc, char* argv[])
{
	args(argc, argv);

	std::cout << "Starting up: " << disks << " balls." << std::endl;

	for (int i = 0; i < disks; i++)
	{
		basehues[i] = (double)(i) / disks;
		// basehues[i] = 0.125;
	}

	for (int i = 0; i < disks; i++)
	{
		floaters[i].set_interp(floatanim);
		floaters[i].set_friction(friction);
		floaters[i].position((1 + 0.5 * sin(2 * PI * i / disks)) / 2, (1 - 0.5 * cos(2 * PI * i / disks)) / 2);
		floaters[i].set_color(basehues[i], restingsat, restingval, alpha);
		floaters[i].set_radius(radius);
		grabbed[i] = -1;
	}

	for (int i = 0; i < fingers; i++)
	{
		grabbers[i].set_interp(grabanim);
		grabbers[i].set_color(0,0,1);
		grabbers[i].set_radius(dot);
		grabbing[i] = -1;
		bind[i] = 0;
		targbind[i] = 0;
		active[i] = false;
	}


	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	auto screen_width = fullscreen ? DM.w : width;
	auto screen_height = fullscreen ? DM.h : height;

	RenderWindow window("Audiovisual", screen_width, screen_height, highDPI, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	if (audio)
		A.startup(in_chans, out_chans, true, in_device, out_device); // startup audio engine


	bool running = true;
	SDL_Event event;

	// SDL_ShowCursor(SDL_DISABLE);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	window.blend(SDL_BLENDMODE_ADD);

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
			}

			if (event.type == SDL_FINGERDOWN)
			{
				bool found = false;
				int j = 0;

				while (!found && j < fingers)
				{
					if (!active[j])
					{
						found = true;
						active[j] = true;
						num_fingers += 1;
						ids[j] = event.tfinger.fingerId;
						// double theta = event.tfinger.x * 2 * PI;
						// double r = 1 - event.tfinger.y;

						grabbers[j].position(event.tfinger.x, event.tfinger.y);
						// grabbers[j].position((1 + r * cos(theta)) / 2, (1 + r * sin(theta)) / 2);
					}

					j += 1;
				}
			}

			if (event.type == SDL_FINGERMOTION)
			{
				bool found = false;
				int j = 0;

				while (!found && j < fingers)
				{
					if (ids[j] == event.tfinger.fingerId)
					{
						found = true;
						// double theta = event.tfinger.x * 2 * PI;
						// double r = 1 - event.tfinger.y;

						grabbers[j].position(event.tfinger.x, event.tfinger.y);
						// grabbers[j].position((1 + r * cos(theta)) / 2, (1 + r * sin(theta)) / 2);
					}

					j += 1;
				}
			}

			if (event.type == SDL_FINGERUP)
			{
				bool found = false;
				int j = 0;

				while (!found && j < fingers)
				{
					if (ids[j] == event.tfinger.fingerId)
					{
						found = true;
						active[j] = false;
						num_fingers -= 1;
					}

					j += 1;
				}
			}
		}

		for (int k = 0; k < oversample; k++)
		{
			for (int i = 0; i < disks; i++)
			{
				floaters[i].tick(1.0 / oversample);
			}

			for (int i = 0; i < fingers; i++)
			{
				grabbers[i].tick(1.0 / oversample);
			}

			for (int i = 0; i < disks; i++)
			{
				floaters[i].constrain(0.5 + 0.25 * sin(2 * PI * i / disks), 0.5 - 0.25 * cos(2 * PI * i / disks), -cos(2 * PI * i / disks), sin(2 * PI * i / disks), 0.01, 0, 1.0 / oversample);
				floaters[i].constrain(0.5, 0.5, sin(2 * PI * i / disks), -cos(2 * PI * i / disks), 0.01, 0, 1.0 / oversample);
				// floaters[i].radiate(0.5, 0.5, 0.25, 0.01, 1.0 / oversample);
			}

			for (int i = 0; i < disks; i++)
			{
				for (int j = i + 1; j < disks; j++)
				{
					Disk::interact(floaters[i], floaters[j], (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2, rigidity, 1.0 / oversample);
				}
			}

			for (int i = 0; i < disks; i++)
			{
				// floaters[i].walls((screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2, wallrigidity, 1.0 / oversample);
			}

			for (int i = 0; i < fingers; i++)
			{
				if (active[i])
				{
					for (int j = 0; j < disks; j++)
					{
						if (grabbed[j] == i || (grabbing[i] == -1 && grabbed[j] == -1 && Disk::close(grabbers[i], floaters[j], (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2)))
						{
							grabbed[j] = i;
							grabbing[i] = j;
							Disk::pull(grabbers[i], floaters[j], bind[i], 1.0 / oversample);

							targbind[i] = 0.1;
							grabbers[i].set_hue(basehues[j]);
							grabbers[i].mod_color(basehues[j], excitedsat, excitedval);
							// grabbers[i].mod_radius(bigd);

							floaters[j].mod_color(basehues[j], excitedsat, excitedval);
							floaters[j].mod_radius(inflation);
						}
					}
				}
				else
				{
					for (int j = 0; j < disks; j++)
						if (grabbed[j] == i)
						{
							grabbed[j] = -1;
							grabbing[i] = -1;

							targbind[i] = 0;
							bind[i] = 0;
							grabbers[i].set_color(0, 0, 1, 1);
							// grabbers[i].set_radius(dot);

							floaters[j].mod_color(basehues[j], restingsat, restingval);
							floaters[j].mod_radius(radius);
						}
				}
			}
		}

		for (int i = 0; i < disks; i++)
		{
			floaters[i].animate();
		}

		for (int i = 0; i < fingers; i++)
		{
			grabbers[i].animate();
			bind[i] = bindanim * bind[i] + (1 - bindanim) * targbind[i];
		}

		window.color(0, 0, 0, 1);
		window.clear();

		for (int i = 0; i < disks; i++)
		{
			floaters[i].draw(&window, (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2);
		}

		for (int i = 0; i < fingers; i++)
		{
			if (active[i])
			{
				grabbers[i].draw(&window, (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2);
			}
		}

		window.display();

		// usleep(100);
	}

	if (audio)
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


inline int process(const float* in, float* out)
{
	for (int i = 0; i < bsize; i++)
	{
		double the_sample = 0;

		if (metro())
		{
			// bool active[n];
			// memset(active, false, n * sizeof(bool));
			std::vector<int> chord = score.read();
			for (auto a : chord)
			{
				// std::cout << a << " ";
				double x, y;
				floaters[a % n].get_position(&x, &y);
				x -= 0.5;
				y -= 0.5;
				double theta = atan2(y, x);
				double modulation = fmod(theta - 2 * PI * a / n, 2 * PI);

				// perf.open(a);
				perf.modulate(a, clip(3 * sqrt(x * x + y * y)));
				// perf.freq_mod(a, modulation);
				// active[a % n] = true;
			}
			// std::cout << std::endl;

			// for (int j = 0; j < n; j++)
			// {
			// 	if (!active[j])
			// 		perf.close(j);
			// }
		}

		// for (int j = 0; j < n; j++)
		// {
		// 	the_sample += perf(metro(), j);
		// }

		if (out_chans == 8)
		{
			for (int j = 0; j < 4; j++)
			{
				double a, b, c;
				a = perf(metro(), 0 + 3 * j) / 2;
				b = perf(metro(), 1 + 3 * j) / 2;
				c = perf(metro(), 2 + 3 * j) / 2;

				out[2 * j + out_chans * i] = gain * (a + b / 2);
				out[2 * j + 1 + out_chans * i] = gain * (b / 2 + c);
			}
		}
		else
		{
			for (int j = 0; j < n; j++)
				the_sample += perf(metro(), j);
			for (int j = 0; j < out_chans; j++)
				out[j + out_chans * i] = gain * the_sample;
		}

		metro.tick();
		perf.tick();
	}

	return 0;
}
