// Multitouch.cpp
#include "Multitouch.h"

void Multitouch::setup()
{
	for (int i = 0; i < disks; i++)
	{
		basehues[i] = (double)(i) / disks;
		// basehues[i] = 0.125;
	}

	for (int i = 0; i < disks; i++)
	{
		floaters[i].set_interp(floatanim);
		floaters[i].set_friction(friction);
		floaters[i].position((1 + 0.5 * sin(2 * M_PI * i / disks)) / 2, (1 - 0.5 * cos(2 * M_PI * i / disks)) / 2);
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
}

void Multitouch::process_event(SDL_Event event)
{
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
				ids[j] = event.tfinger.fingerId;

				grabbers[j].position(event.tfinger.x, event.tfinger.y);
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
			}

			j += 1;
		}
	}
}

void Multitouch::tick(int screen_width, int screen_height, int width, int height)
{
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
			floaters[i].constrain(0.5 + 0.25 * sin(2 * M_PI * i / disks), 0.5 - 0.25 * cos(2 * M_PI * i / disks), -cos(2 * M_PI * i / disks), sin(2 * M_PI * i / disks), 0.01, 0, 1.0 / oversample);
			floaters[i].constrain(0.5, 0.5, sin(2 * M_PI * i / disks), -cos(2 * M_PI * i / disks), 0.01, 0, 1.0 / oversample);
			// floaters[i].radiate(0.5, 0.5, 0.25, 0.01, 1.0 / oversample);
		}

		for (int i = 0; i < disks; i++)
		{
			for (int j = i + 1; j < disks; j++)
			{
				// Disk::interact(floaters[i], floaters[j], (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2, rigidity, 1.0 / oversample);
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
}


void Multitouch::draw(RenderWindow* window, int screen_width, int screen_height, int width, int height)
{
	for (int i = 0; i < disks; i++)
	{
		floaters[i].draw(window, (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2);
	}

	for (int i = 0; i < fingers; i++)
	{
		if (active[i])
		{
			grabbers[i].draw(window, (screen_width - width) / 2, width + (screen_width - width) / 2, (screen_height - height) / 2, height + (screen_height - height) / 2);
		}
	}
}