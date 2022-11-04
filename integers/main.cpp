#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;
const double correction = (highDPI ? 1 : 0.5);
const double PI = 245850922.0 / 78256779.0;
const double fillalpha = 0.875;
const double colormod = 32;

SDL_Rect fill = { 0, 0, (int)(width * 2 * correction), (int)(height * 2 * correction) };

SDL_BlendMode polyblend = SDL_BLENDMODE_ADD;

const double radius = width / 3;
const double overlap = 1; // spacing of initialized balls
const int x_balls = overlap * width / radius;
const int y_balls = overlap * height / radius;
const int balls = x_balls * y_balls;
const double buffer = 1; // how much balls can overlap before collision

double tiles = 3; // how many fundamental domains to draw per row
const int resolution = (int)(2 * PI * radius / 4); // segments per circle

const bool checker = false;

SDL_Vertex points[resolution * 3];

const long long scale = 1;
long long x_centers[balls];
long long y_centers[balls];
long long x_vels[balls];
long long y_vels[balls];
long long x_accs[balls];
long long y_accs[balls];

double hues[balls];
double saturations[balls];
double values[balls];


double clip(double x)
{
	return (x < 0 ? 0 : (x > 1 ? 1 : x));
}

void init()
{
	for (int i = 0; i < x_balls; i++)
	{
		for (int j = 0; j < y_balls; j++)
		{
			int index = i + x_balls * j;

			x_centers[index] = (int)(scale * (radius + 2 * radius * i / overlap));
			y_centers[index] = (int)(scale * (radius + 2 * radius * j / overlap));
			x_vels[index] = (int)(scale * -j * radius / 32);
			y_vels[index] = (int)(scale * i * radius / 32);

			hues[index] = 0.5;
			saturations[index] = 0.5;
			values[index] = 0.0625;
		}
	}
}

void tick()
{
	for (int i = 0; i < balls; i++)
	{
		x_centers[i] += x_vels[i];
		y_centers[i] += y_vels[i];

		int x_new = (x_centers[i] % (scale * 2 * width) + scale * 2 * width) % (scale * 2 * width);
		int y_new = (y_centers[i] % (scale * 2 * height) + scale * 2 * height) % (scale * 2 * height);

		hues[i] += ((double)(x_new - x_centers[i])) / (colormod * scale * 2 * width) + ((double)(y_new - y_centers[i])) / (colormod * scale * 2 * height);
		hues[i] = fmod(hues[i] + 1, 1);

		x_centers[i] = x_new;
		y_centers[i] = y_new;

		// x_vels[i] *= 0.999;
		// y_vels[i] *= 0.999;
	}
}


void circle(RenderWindow* window, double x, double y, double radius, double hue, double saturation, double value)
{
	hue = clip(hue);
	saturation = clip(saturation);
	unsigned char R, G, B;
	SDL_Color color;
	
	R = (unsigned char)(255 * (value + 1 + sin(2 * PI * (0.0 / 3 * saturation + hue))) / (value + 2));
	G = (unsigned char)(255 * (value + 1 + sin(2 * PI * (1.0 / 3 * saturation + hue))) / (value + 2));
	B = (unsigned char)(255 * (value + 1 + sin(2 * PI * (2.0 / 3 * saturation + hue))) / (value + 2));

	color = { R, G, B, (unsigned char)(fillalpha * 255) };

	double x_coords[resolution];
	double y_coords[resolution];
	for (int i = 0; i < resolution; i++)
	{
		x_coords[i] = x + radius * cos(2 * PI * (double)i / resolution);
		y_coords[i] = y + radius * sin(2 * PI * (double)i / resolution);
	}

	for (int j = 0; j < resolution; j++)
	{
		points[3 * j] = { SDL_FPoint{ float(correction * x_coords[j]), float(correction * y_coords[j]) }, color, SDL_FPoint{ 0 } };
		points[3 * j + 1] = { SDL_FPoint{ float(correction * x_coords[(j + 1) % resolution]), float(correction * y_coords[(j + 1) % resolution]) }, color, SDL_FPoint{ 0 } };
		points[3 * j + 2] = { SDL_FPoint{ float(correction * x), float(correction * y) }, color, SDL_FPoint{ 0 } };
	}

	window->geometry(points, resolution * 3);
}


void draw(RenderWindow* window, double r_scale = 1)
{
	for (int i = 0; i < balls; i++)
	{
		double x, y;
		x = (double)(x_centers[i]) / (scale * tiles);
		y = (double)(y_centers[i]) / (scale * tiles);

		for (int l = -1; l <= tiles; l++)
		{
			for (int k = -1; k <= tiles; k++)
			{
				double hue = hues[i] + l / (colormod) + k / (colormod);
				hue = fmod(hue + 1, 1);
				circle(window, 
					x + l * 2 * width / tiles, 
					y + k * 2 * height / tiles, 
					r_scale * radius / tiles, 
					hue, saturations[i], values[i]);
			}
		}
	}
}


int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	RenderWindow window("Integers", width, height, highDPI); 
	window.blend(polyblend);

	init();

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

		window.color(0, 0, 0, 0);
		window.clear();

		// draw(&window, 0.75);
		// draw(&window, 0.5);
		// draw(&window, 0.25);

		draw(&window);



		if (checker)
		{
			window.blend(SDL_BLENDMODE_BLEND);
			for (int l = 0; l < tiles; l++)
			{
				for (int k = 0; k < tiles; k++)
				{
					fill = { (int)(l * 2 * width / tiles), (int)(k * 2 * height / tiles), 
							 (int)(2 * width / tiles), (int)(2 * height / tiles) };

					double check = ((k + l) % 2) / 12.0;
					window.color(0, 0, 0, check);
					window.rectangle(&fill);
				}
			}
			window.blend(polyblend);
		}


		tick();

		window.display();
	}

	window.~RenderWindow();

	SDL_Quit();

	return 0;
}
