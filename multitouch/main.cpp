#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

#include "RenderWindow.h"
#include "Color.h"
#include "Disk.h"

const bool highDPI = true;
const double correction = (highDPI ? 2 : 1);

const int width = 768;
const int height = 768;


const double PI = 245850922.0 / 78256779.0;
const double radius = 24;
const double dot = radius * 0.125;
const double bigd = radius * 0.5;

const double factor = 5;
const int fingers = 12;
const double density = 1.5; // 1.0 / 16;
const int disks = density * width * height / (PI * radius * radius);
const double inflation = radius * factor;

const double rigidity = 0.1;
const double wallrigidity = 0.05;
const double bindanim = 0.95;
const double floatanim = 0.8;
const double grabanim = 0.9;

const double restingval = 1.0 / 3;
const double excitedval = 1.0 / 3;

const double restingsat = 0.0 / 3; // 2.0 / 3;
const double excitedsat = 2.0 / 3; // 0.0 / 3;

const double alpha = 1; // 2.0 / 3;

const double friction = 0.9;
const int oversample = 8;


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

const int fineness = 6;
const int x_zones = width / (radius * fineness);
const int y_zones = height / (radius * fineness);

int main(int argc, char* args[])
{
	std::cout << "Starting up: " << disks << " balls." << std::endl;

	// bool registered[disks][disks];
	// int zones[disks][x_zones][y_zones];
	// int s_zone[disks];
	// int zone_inds[x_zones][y_zones];
	// int s_ind;

	// memset(registered, false, disks * disks * sizeof(bool));
	// memset(zones, -1, disks * x_zones * y_zones * sizeof(int));
	// memset(s_zone, -1, disks * sizeof(int));
	// memset(zone_inds, 0, x_zones * y_zones * sizeof(int));
	// s_ind = 0;

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

	RenderWindow window("Multitouch", width, height, highDPI, SDL_WINDOW_MOUSE_CAPTURE); 

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
						grabbers[j].position(event.tfinger.x, event.tfinger.y);
						grabbers[j].mod_radius(event.tfinger.pressure * (1 + dot));
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
						grabbers[j].position(event.tfinger.x, event.tfinger.y);
						grabbers[j].mod_radius(event.tfinger.pressure * (1 + dot));
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

			// memset(zone_inds, 0, x_zones * y_zones * sizeof(int));
			// double x, y, r;
			// for (int i = 0; i < disks; i++)
			// {
			// 	floaters[i].get_position(&x, &y);
			// 	floaters[i].get_radius(&r);

			// 	bool zoned[3][3];
			// 	bool sporadic = false;
			// 	memset(zoned, false, 3 * 3 * sizeof(bool));

			// 	int base_x = (int)(x * x_zones); // "home base" zones
			// 	int base_y = (int)(y * y_zones);

			// 	for (int l = -1; l <= 1; l++)
			// 	{
			// 		for (int p = -1; p <= 1; p++)
			// 		{
			// 			int pert_x = (int)((x + l * r / width) * x_zones);
			// 			int pert_y = (int)((y + p * r / height) * y_zones);

			// 			zoned[1 + pert_x - base_x][1 + pert_y - base_y] = true;

			// 			if (!(pert_x >= 0 && pert_x < x_zones && pert_y >= 0 && pert_y < y_zones))
			// 				sporadic = true;
			// 		}
			// 	}

			// 	for (int l = -1; l <= 1; l++)
			// 	{
			// 		for (int p = -1; p <= 1; p++)
			// 		{
			// 			if (zoned[1 + l][1 + p])
			// 			{
			// 				if (0 <= base_x + l && base_x + l < x_zones && 0 <= base_y + p && base_y + p < y_zones)
			// 				{
			// 					zones[zone_inds[base_x + l][base_y + p]][base_x + p][base_y + l] = i;
			// 					zone_inds[base_x + l][base_y + p]++;
			// 				}
			// 			}
			// 		}
			// 	}

			// 	if (sporadic)
			// 	{
			// 		s_zone[s_ind] = i;
			// 		s_ind++;
			// 	}
			// }

			// memset(registered, false, disks * disks * sizeof(bool));
			// for (int x = 0; x < x_zones; x++)
			// {
			// 	for (int y = 0; y < y_zones; y++)
			// 	{
			// 		// std::cout << "Checking zone x: " << x << ", y: " << y << " with " << zone_inds[x][y] << " members." << std::endl;
			// 		for (int i = 0; i < zone_inds[x][y]; i++)
			// 		{
			// 			for (int j = i + 1; j < zone_inds[x][y]; j++)
			// 			{
			// 				int ball1 = zones[i][x][y];
			// 				int ball2 = zones[j][x][y];
			// 				if (!registered[ball1][ball2])
			// 				{
			// 					// std::cout << ball1 << " " << ball2 << std::endl;
			// 					Disk::interact(floaters[ball1], floaters[ball2], 0, width, 0, height, rigidity, 1.0 / oversample);
			// 					registered[ball1][ball2] = true;
			// 					registered[ball2][ball1] = true;
			// 				}
			// 			}
			// 		}
			// 	}
			// }

			for (int i = 0; i < disks; i++)
			{
				for (int j = i + 1; j < disks; j++)
				{
					Disk::interact(floaters[i], floaters[j], 0, width, 0, height, rigidity, 1.0 / oversample);
				}
			}

			for (int i = 0; i < disks; i++)
			{
				floaters[i].walls(0, width, 0, height, wallrigidity, 1.0 / oversample);
			}

			for (int i = 0; i < fingers; i++)
			{
				if (active[i])
				{
					for (int j = 0; j < disks; j++)
					{
						if (grabbed[j] == i || (grabbing[i] == -1 && grabbed[j] == -1 && Disk::close(grabbers[i], floaters[j], 0, width, 0, height)))
						{
							grabbed[j] = i;
							grabbing[i] = j;
							Disk::pull(grabbers[i], floaters[j], 0, width, 0, height, bind[i], 1.0 / oversample);

							targbind[i] = 1;
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
			floaters[i].draw(&window, 0, width, 0, height);
		}

		for (int i = 0; i < fingers; i++)
		{
			if (active[i])
			{
				grabbers[i].draw(&window, 0, width, 0, height);
			}
		}

		window.display();
	}

	window.~RenderWindow();

	SDL_Quit();

	return 0;
}
