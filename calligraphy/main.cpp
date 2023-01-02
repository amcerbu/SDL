#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>

#include "RenderWindow.h"
#include "audio.h"

// const double PI = 245850922.0 / 78256779.0;
const int width = 1200;
const int height = 1200;
int screen_width = width;
int screen_height = height;
const bool highDPI = true;
const double correction = (highDPI ? 1 : 0.5);
double radius = correction * 0.75 * std::min(screen_width, screen_height);

const int count = 2048; // 512; // segments on path
const double smooth = 256; // 64;
const int skip = 128; // segments on path
const int resolution = 32; // segments on circular caps
const bool capped = false; // display line segment caps?
const bool circled = false; // display semicircular caps?
const bool core = false; // display core curve?
const bool pairs = false; // display many segments?

const bool tracked = false;
const bool undulating = false;
const int tracks = 1; // number of parallel curves (on each side)
const double spacing = radius / 4; // spacing of parallel curves

const bool tied = false;

const double ratio = 0.75; // what proportion of a full curve to render (may take values over 1)
const bool closed = ratio == 1;
const double alpha = 0.85; // alpha of core curve
const double circlealpha = sqrt(alpha);
const double fillalpha = 0.33; // alpha of core curve
const double backalpha = 1; // alpha of background
const double r = 1;
const double g = 1;
const double b = 1;

const bool geometry = true;
const bool disks = false;
bool fullscreen = false;
bool mouse = false;

double mod = 0;
double modfreq = 1; // 1.5 * 0.75; // 0.75;

double phase = 0;
double freq = 1.5 * 0.05; // rate at which oscillator completes revolution
double framerate = 120;

SDL_FPoint points[count];
SDL_FPoint normals[count];
SDL_FPoint offsets[count * 2 * tracks];
SDL_FPoint circle1[resolution * tracks];
SDL_FPoint circle2[resolution * tracks];
SDL_FPoint cap1[2];
SDL_FPoint cap2[2];

SDL_Vertex trackverts[(count - !closed) * 6];
SDL_Vertex diskverts1[(resolution - 1) * 3];
SDL_Vertex diskverts2[(resolution - 1) * 3];

const bool blend = true;
SDL_BlendMode polyblend;

// gives coordinates to point
void f(double angle, SDL_FPoint* point)
{
	const bool wiggly = false;
	point->x = (float)( screen_width * correction + (radius / 2 + wiggly * 0.1 * radius / 4 * cos(mod + sin(mod * angle))) * cos(angle) + sin(mod / 5) * 0.5 * radius / 2 * sin(3 * angle));
	point->y = (float)(screen_height * correction + (radius / 2 + wiggly * 0.1 * radius / 4 * sin(mod + cos(mod * angle))) * sin(angle) + cos(mod / 6) * 0.5 * radius / 2 * cos(5 * angle));
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

double norm(double x, double y, Smoothing normtype = L2, double smoothing = smooth / count)
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

using namespace soundmath;

// const int bsize = 64;
// inline int process(const float* in, float* out);
// Audio A = Audio(process, bsize);

int main(int argc, char* args[])
{
	if (blend)
	{
		polyblend = SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_SRC_ALPHA,
			SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,// SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			SDL_BLENDOPERATION_MAXIMUM,
			SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ZERO, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			SDL_BLENDFACTOR_SRC_ALPHA, // SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			SDL_BLENDOPERATION_MINIMUM);
	}
	else
	{
		polyblend = SDL_BLENDMODE_NONE;
	}


	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	for (int i = 0; i < count; i++)
	{
		// double angle = 2 * PI * (phase + ratio * (double)i / (count - 1)); // make sure this thing closes up!
		double angle = 2 * PI * (phase + ratio * (double)i / count); // make sure this thing closes up!
		f(angle, &points[i]);
	}

	gauss(points, normals, count);

	for (int i = 0; i < tracks; i++)
	{
		move(points, offsets + (2 * i) * count, normals, spacing * (1 + i), count);
		move(points, offsets + (2 * i + 1) * count, normals, -spacing * (1 + i), count);
	}

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	screen_width = fullscreen ? DM.w : width;
	screen_height = fullscreen ? DM.h : height;

	if (mouse)
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	else
		SDL_SetRelativeMouseMode(SDL_TRUE);


	RenderWindow window("Calligraphy", screen_width, screen_height, highDPI, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); 

	bool running = true;
	SDL_Event event;
	SDL_Rect fillRect = { 0, 0, (int)(width * 2 * correction), (int)(height * 2 * correction) };

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

				if (event.key.keysym.sym == SDLK_SPACE)
				{
					mod += (double)count / 32;
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
					radius = correction * 0.75 * std::min(screen_width, screen_height);
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

		if (true)
		{
			window.color(1 - r, 1 - g, 1 - b, backalpha);
			// window.color(0,0,0,1);

			if (backalpha < 1)
				window.rectangle(&fillRect);
			else
				window.clear();

			if (disks)
			{
				SDL_Color color;
				unsigned char R, G, B;

				int j = 0;

				for (int i = 0; i < resolution - 1; i++)
				{
					R = (unsigned char)(255 * (1 + sin(2 * PI * (0.0 / 3 * (cos(mod * freq / modfreq) / 2) + 0))) / 2);
					G = (unsigned char)(255 * (1 + sin(2 * PI * (1.0 / 3 * (cos(mod * freq / modfreq) / 2) + 0))) / 2);
					B = (unsigned char)(255 * (1 + sin(2 * PI * (2.0 / 3 * (cos(mod * freq / modfreq) / 2) + 0))) / 2);

					color = { R, G, B, (unsigned char)(fillalpha * 255) };

					diskverts1[j] = { SDL_FPoint{ (circle1 + resolution * (tracks - 1))[i].x, (circle1 + resolution * (tracks - 1))[i].y }, color, SDL_FPoint{ 0 } };
					diskverts1[j + 1] = { SDL_FPoint{ (circle1 + resolution * (tracks - 1))[i + 1].x, (circle1 + resolution * (tracks - 1))[i + 1].y }, color, SDL_FPoint{ 0 } };
					diskverts1[j + 2] = { SDL_FPoint{ (points)[0].x, (points)[0].y }, color, SDL_FPoint{ 0 } };

					R = (unsigned char)(255 * (1 + sin(2 * PI * (0.0 / 3 * (cos(mod * freq / modfreq) / 2) + 1))) / 2);
					G = (unsigned char)(255 * (1 + sin(2 * PI * (1.0 / 3 * (cos(mod * freq / modfreq) / 2) + 1))) / 2);
					B = (unsigned char)(255 * (1 + sin(2 * PI * (2.0 / 3 * (cos(mod * freq / modfreq) / 2) + 1))) / 2);

					color = { R, G, B, (unsigned char)(fillalpha * 255) };

					diskverts2[j] = { SDL_FPoint{ (circle2 + resolution * (tracks - 1))[i].x, (circle2 + resolution * (tracks - 1))[i].y }, color, SDL_FPoint{ 0 } };
					diskverts2[j + 1] = { SDL_FPoint{ (circle2 + resolution * (tracks - 1))[i + 1].x, (circle2 + resolution * (tracks - 1))[i + 1].y }, color, SDL_FPoint{ 0 } };
					diskverts2[j + 2] = { SDL_FPoint{ (points)[count - 1].x, (points)[count - 1].y }, color, SDL_FPoint{ 0 } };

					j += 3;
				}
				

				window.blend(polyblend);
				window.color(1 - r, 1 - g, 1 - b, 0);
				window.rectangle(&fillRect);

				window.geometry(diskverts1, resolution * 3);
				window.geometry(diskverts2, resolution * 3);
				window.blend(SDL_BLENDMODE_BLEND);
			}

			if (geometry)
			{
				int i = 1;
				int j = 0;

				int left = (2 * (tracks - 1)) * count;
				int right = (2 * (tracks - 1) + 1) * count;

				SDL_Color color;
				unsigned char R, G, B;

				for (int i = 0; i < count - 1; i++)
				{
					R = (unsigned char)(255 * (1 + sin(2 * PI * (0.0 / 3 * cos(mod * freq / modfreq) / 2 + (double)i / count))) / 2);
					G = (unsigned char)(255 * (1 + sin(2 * PI * (1.0 / 3 * cos(mod * freq / modfreq) / 2 + (double)i / count))) / 2);
					B = (unsigned char)(255 * (1 + sin(2 * PI * (2.0 / 3 * cos(mod * freq / modfreq) / 2 + (double)i / count))) / 2);

					color = { R, G, B, (unsigned char)(fillalpha * 255) };

					trackverts[j++] = { SDL_FPoint{ (offsets + left)[i].x, (offsets + left)[i].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + left)[i + 1].x, (offsets + left)[i + 1].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + right)[i].x, (offsets + right)[i].y }, color, SDL_FPoint{ 0 } };

					trackverts[j++] = { SDL_FPoint{ (offsets + right)[i].x, (offsets + right)[i].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + right)[i + 1].x, (offsets + right)[i + 1].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + left)[i + 1].x, (offsets + left)[i + 1].y }, color, SDL_FPoint{ 0 } };
				}

				if (closed)
				{
					R = (unsigned char)(255 * (1 + sin(2 * PI * (0.0 / 3 * cos(mod * freq / modfreq) / 2 + 1))) / 2);
					G = (unsigned char)(255 * (1 + sin(2 * PI * (1.0 / 3 * cos(mod * freq / modfreq) / 2 + 1))) / 2);
					B = (unsigned char)(255 * (1 + sin(2 * PI * (2.0 / 3 * cos(mod * freq / modfreq) / 2 + 1))) / 2);

					color = { R, G, B, (unsigned char)(fillalpha * 255) };

					trackverts[j++] = { SDL_FPoint{ (offsets + left)[count - 1].x, (offsets + left)[count - 1].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + left)[0].x, (offsets + left)[0].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + right)[count - 1].x, (offsets + right)[count - 1].y }, color, SDL_FPoint{ 0 } };

					trackverts[j++] = { SDL_FPoint{ (offsets + right)[count - 1].x, (offsets + right)[count - 1].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + right)[0].x, (offsets + right)[0].y }, color, SDL_FPoint{ 0 } };
					trackverts[j++] = { SDL_FPoint{ (offsets + left)[0].x, (offsets + left)[0].y }, color, SDL_FPoint{ 0 } };
				}

				window.color(1 - r, 1 - g, 1 - b, 0);
				window.rectangle(&fillRect);

				window.blend(polyblend);
				
				window.geometry(trackverts, (count - !closed) * 6);
				window.blend(SDL_BLENDMODE_BLEND);
			}

			if (core)
			{
				window.color(r, g, b, alpha);
				window.curve(points, count);
			}

			if (pairs)
			{
				for (int i = 0; i < count - skip; i += skip)
				{
					for (int j = i + skip; j < count; j += skip)
					{
						double x1, y1, x2, y2, dx, dy;
						x1 = points[i].x;
						y1 = points[i].y;
						x2 = points[j].x;
						y2 = points[j].y;

						dx = x2 - x1;
						dy = y2 - y1;

						double opacity = 0.2 / (norm(dx, dy) / radius) - 0.1;
						if (opacity >= 0.4)
						{
							window.color(r, g, b, opacity);
							window.line(x1, y1, x2, y2);
						}
					}
				}
			}

			if (tracked)
			{			
				for (int i = 0; i < tracks; i++)
				{
					window.color(r, g, b, alpha * (1 - (double)(i + 1) / (tracks + 1)));
					window.curve(offsets + (2 * i) * count, count);
					window.curve(offsets + (2 * i + 1) * count, count);
				}
			}

			if (capped) // draws line segment caps
			{
				cap1[0] = offsets[2 * (tracks - 1) * count];
				cap1[1] = offsets[(2 * (tracks - 1) + 1) * count];
				cap2[0] = offsets[2 * (tracks - 1) * count + count - 1];
				cap2[1] = offsets[(2 * (tracks - 1) + 1) * count + count - 1];

				window.curve(cap1, 2);
				window.curve(cap2, 2);
			}

			if (tied)
			{
				for (int i = skip; i < count; i += skip)
				{
					cap1[0] = offsets[2 * (tracks - 1) * count + i];
					cap1[1] = offsets[(2 * (tracks - 1) + 1) * count + i];

					window.curve(cap1, 2);
				}
			}

			if (circled) // draws semicircular caps
			{
				for (int i = 0; i < tracks; i++)
				{
					window.color(r, g, b, circlealpha * (1 - (double)(i + 1) / (tracks + 1)));
					window.curve(circle1 + resolution * i, resolution);
					window.curve(circle2 + resolution * i, resolution);
				}
			}

			phase += freq / framerate;
			phase -= int(phase);

			mod += modfreq / framerate;

			for (int i = 0; i < count; i++)
			{
				double angle = 2 * PI * (phase + ratio * (double)i / (count - 1));
				f(angle, &points[i]);
			}

			gauss(points, normals, count);

			for (int i = 0; i < tracks; i++)
			{
				double scale;
				if (undulating)
					scale =  (1 + sin((2 * PI * (double)i / tracks) + mod / 8)) / 2;
				else
					scale = 1 + i;

				move(points, offsets + (2 * i) * count, normals, scale * spacing, count);
				move(points, offsets + (2 * i + 1) * count, normals, scale * -spacing, count);
			}

			if (circled || disks) // draws semicircular caps
			{
				for (int i = 0; i < tracks; i++)
				{
					window.color(r, g, b, circlealpha * (1 - (double)(i + 1) / (tracks + 1)));
					double x1, y1, x2, y2;
						
					x1 = offsets[2 * i * count].x;
					y1 = offsets[2 * i * count].y;
					x2 = (x1 + offsets[(2 * i + 1) * count].x) / 2;
					y2 = (y1 + offsets[(2 * i + 1) * count].y) / 2;
					
					for (int j = 0; j < resolution; j++)
					{
						double angle = PI * (double)j / (resolution - 1);
						double sine = sin(angle);
						double cosine = cos(angle);
						circle1[j + resolution * i] = { (float)(x2 + cosine * (x2 - x1) - sine * (y2 - y1)),
														(float)(y2 + cosine * (y2 - y1) + sine * (x2 - x1)) };
					}

					x1 = offsets[2 * i * count + count - 1].x;
					y1 = offsets[2 * i * count + count - 1].y;
					x2 = (x1 + offsets[(2 * i + 1) * count + count - 1].x) / 2;
					y2 = (y1 + offsets[(2 * i + 1) * count + count - 1].y) / 2;
					
					for (int j = 0; j < resolution; j++)
					{
						double angle = PI * (double)j / (resolution - 1);
						double sine = sin(angle);
						double cosine = cos(angle);
						circle2[j + resolution * i] = { (float)(x2 + cosine * (x1 - x2) - sine * (y1 - y2)),
														(float)(y2 + cosine * (y1 - y2) + sine * (x1 - x2)) };
					}

				}
			}

			window.display();
		}
	}

	window.~RenderWindow();

	SDL_Quit();

	return 0;
}

// inline int process(const float* in, float* out)
// {
// 	for (int i = 0; i < bsize; i++)
// 	{
		
// 	}
// }