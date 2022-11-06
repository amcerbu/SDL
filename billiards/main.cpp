#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"
#include "Color.h"

#define number long double

const int width = 720;
const int height = 720;
const bool highDPI = true;
const double correction = (highDPI ? 1 : 0.5);
const double PI = 245850922.0 / 78256779.0;
const double fillalpha = 0.875;
const double colormod = 32;

SDL_Rect fill = { 0, 0, (int)(width * 2 * correction), (int)(height * 2 * correction) };

SDL_BlendMode polyblend = SDL_BLENDMODE_ADD;
// SDL_ComposeCustomBlendMode(
// 	SDL_BLENDFACTOR_SRC_ALPHA,
// 	SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
// 	SDL_BLENDOPERATION_MAXIMUM,
// 	SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ZERO, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
// 	SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
// 	SDL_BLENDOPERATION_MINIMUM);

const int oversample = 1024;
const number stiffness = 4;
const bool rigid = false;

const number radius = width / 6;
const number power = 1.0 / 4;
const number overlap = 1 - power; // spacing of initialized balls
const int x_balls = overlap * width / radius;
const int y_balls = overlap * height / radius;
const int balls = x_balls * y_balls;
const number buffer = 1;

number tiles = 2; // how many fundamental domains to draw
const bool centered = true;
const int resolution = (int)(2 * PI * radius / 16); // segments per circle

const bool checker = false;

SDL_Vertex points[resolution * 3];

const number nudge = 1;
number x_centers[balls];
number y_centers[balls];
number x_vels[balls];
number y_vels[balls];
number x_accs[balls];
number y_accs[balls];

bool prepared[balls * (balls - 1) / 2];

bool left[balls];
bool right[balls];
bool up[balls];
bool down[balls];

double hues[balls];
double saturations[balls];
double values[balls];

number clip(number x)
{
	return (x < 0 ? 0 : (x > 1 ? 1 : x));
}

void init()
{
	number x_avg = 0;
	number y_avg = 0;
	for (int i = 0; i < x_balls; i++)
	{
		for (int j = 0; j < y_balls; j++)
		{
			int index = i + x_balls * j;
			left[index] = false;
			right[index] = false;
			up[index] = false;
			down[index] = false;

			x_centers[index] = radius / (2 * overlap) + radius * i / overlap;
			y_centers[index] = radius / (2 * overlap) + radius * j / overlap;
			x_vels[index] = (y_balls - 1 - j) * radius / 64;
			y_vels[index] = i * radius / 64;

			x_avg += x_vels[index];
			y_avg += y_vels[index];

			hues[index] = 0;
			saturations[index] = 2.0 / 3;
			values[index] = 1.0 / 3;

			// hues[index] = 0.5;
			// saturations[index] = 0.5;
			// values[index] = 0.0625;
		}
	}

	hues[0] = 0.5;
	saturations[0] = 0;
	values[0] = 1;

	if (centered)
		for (int i = 0; i < balls; i++)
		{
			x_vels[i] -= (number)x_avg / balls;
			y_vels[i] -= (number)y_avg / balls;
		}
}

void collisions(number step = 1)
{
	memset(x_accs, 0, balls * sizeof(number));
	memset(y_accs, 0, balls * sizeof(number));

	number x1, x2, y1, y2, vx1, vx2, vy1, vy2;
	number dx, dy;
	int count = 0;

	for (int i = 0; i < balls; i++)
	{
		for (int j = i + 1; j < balls; j++)
		{
			x1 = x_centers[i];
			x2 = x_centers[j];
			y1 = y_centers[i];
			y2 = y_centers[j];

			vx1 = x_vels[i];
			vx2 = x_vels[j];
			vy1 = y_vels[i];
			vy2 = y_vels[j];

			dx = x2 - x1;
			dy = y2 - y1;
			number dist = dx * dx + dy * dy;
			number tx, ty;

			for (int l = -1; l <= 1; l++)
			{
				for (int k = -1; k <= 1; k++)
				{
					tx = x2 - x1 + l * width;
					ty = y2 - y1 + k * height;
					if (tx * tx + ty * ty < dist)
					{
						dist = tx * tx + ty * ty;
						dx = tx;
						dy = ty;
					}
				}
			}

			number repulsion = (radius * buffer) * (radius * buffer) - dist;
			repulsion /= radius * radius;
			repulsion *= step * (repulsion > 0);

			if (repulsion > 0)
			{
				if (prepared[count] && rigid)
				{
					double d1 = (vx1 * dx + vy1 * dy) / (dx * dx + dy * dy); // dot products
					double d2 = (vx2 * dx + vy2 * dy) / (dx * dx + dy * dy);

					x_accs[i] += (1 * d2 - d1) * dx; // + -(1 - t) * repulsion * dx / sqrt(dist);
					y_accs[i] += (1 * d2 - d1) * dy; // + -(1 - t) * repulsion * dy / sqrt(dist);
					x_accs[j] += (1 * d1 - d2) * dx; // + (1 - t) * repulsion * dx / sqrt(dist);
					y_accs[j] += (1 * d1 - d2) * dy; // + (1 - t) * repulsion * dy / sqrt(dist);
				}
				else
				{
					x_accs[i] += -stiffness * repulsion * dx / sqrt(dist);
					y_accs[i] += -stiffness * repulsion * dy / sqrt(dist);
					x_accs[j] += stiffness * repulsion * dx / sqrt(dist);
					y_accs[j] += stiffness * repulsion * dy / sqrt(dist);	
				}

				prepared[count] = false;
			}
			else
			{
				prepared[count] = true;
			}

			count += 1;
		}
	}

	for (int i = 0; i < balls; i++)
	{
		x_vels[i] += x_accs[i];
		y_vels[i] += y_accs[i];
	}

}

void tick(number step = 1)
{
	for (int i = 0; i < balls; i++)
	{
		x_centers[i] += step * x_vels[i];
		y_centers[i] += step * y_vels[i];

		number x_new = fmod(fmod(x_centers[i], width) + width,  width);
		number y_new = fmod(fmod(y_centers[i], height) + height,  height);

		if (i)
		{
			hues[i] += (x_new - x_centers[i]) / (colormod * width) + (y_new - y_centers[i]) / (colormod * height);
			hues[i] = fmod(hues[i] + 1, 1);
		}

		x_centers[i] = x_new;
		y_centers[i] = y_new;

		left[i] = x_centers[i] <= radius;
		right[i] = x_centers[i] > width - radius;
		up[i] = y_centers[i] <= radius;
		down[i] = y_centers[i] > height - radius;
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


void draw(RenderWindow* window)
{
	Color color;
	for (int i = 0; i < balls; i++)
	{
		double x, y;
		x = x_centers[i] / tiles;
		y = y_centers[i] / tiles;

		for (int l = -1; l <= tiles; l++)
		{
			for (int k = -1; k <= tiles; k++)
			{
				double hue = hues[i] + (i ? l / (colormod) + k / (colormod) : 0);
				hue = fmod(hue + 1, 1);

				double R, G, B;
				R = (values[i] + 1 + sin(2 * PI * (0.0 / 3 * saturations[i] + hue))) / (values[i] + 2);
				G = (values[i] + 1 + sin(2 * PI * (1.0 / 3 * saturations[i] + hue))) / (values[i] + 2);
				B = (values[i] + 1 + sin(2 * PI * (2.0 / 3 * saturations[i] + hue))) / (values[i] + 2);

				color.hsva(hue, saturations[i], values[i], fillalpha);
				window->color(color);
				// window->color(R, G, B, fillalpha);
				// window->color(1, 1, 1, 1);
				window->circle((x + l * width / tiles), (y + k * height / tiles), 0.5 * (radius / tiles));
				// circle(window, 
				// 	x + l * width / tiles, 
				// 	y + k * height / tiles, 
				// 	radius / tiles, 
				// 	hue, saturations[i], values[i]);
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

	RenderWindow window("Billiards", width, height, highDPI); 
	// window.blend(polyblend);

	init();

	bool running = true;
	SDL_Event event;

	bool spaced = false;

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

				if (!spaced && event.key.keysym.sym == SDLK_SPACE)
				{
					for (int i = 0; i < balls; i++)
					{
						x_vels[i] *= -1;
						y_vels[i] *= -1;
					}
					spaced = true;
				}

				if (event.key.keysym.sym == SDLK_LEFT)
				{
					
				}

				if (event.key.keysym.sym == SDLK_RIGHT)
				{
					
				}

				if (event.key.keysym.sym == SDLK_UP)
				{
					
				}

				if (event.key.keysym.sym == SDLK_DOWN)
				{
					
				}
			}
			if (event.type == SDL_KEYUP)
			{
				if (event.key.keysym.sym == SDLK_SPACE)
				{
					spaced = false;
				}
			}
		}

		const Uint8* keystates = SDL_GetKeyboardState(NULL);
		if (keystates[SDL_SCANCODE_LEFT])
		{
			x_vels[0] -= nudge;
		}
		if (keystates[SDL_SCANCODE_RIGHT])
		{
			x_vels[0] += nudge;
		}
		if (keystates[SDL_SCANCODE_UP])
		{
			y_vels[0] -= nudge;
		}
		if (keystates[SDL_SCANCODE_DOWN])
		{
			y_vels[0] += nudge;
		}

		window.color(0, 0, 0, 0);
		window.clear();

		draw(&window);

		if (checker)
		{
			// window.blend(SDL_BLENDMODE_BLEND);
			for (int l = 0; l < tiles; l++)
			{
				for (int k = 0; k < tiles; k++)
				{
					fill = { (int)(l * width / tiles), (int)(k * height / tiles), 
							 (int)(width / tiles), (int)(height / tiles) };

					double check = ((k + l) % 2) / 12.0;
					window.color(0, 0, 0, check);
					window.rectangle(&fill);
				}
			}
			// window.blend(polyblend);
		}


		for (int i = 0; i < oversample; i++)
		{
			tick(1.0 / oversample);
			collisions(1.0 / oversample);
		}

		window.display();
	}

	window.~RenderWindow();

	SDL_Quit();

	return 0;
}
