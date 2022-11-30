#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"
#include "Audio.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;
bool fullscreen = false;
bool mouse = true;

const int bsize = 64;

Audio A(bsize, SR);

int main(int argc, char* argv[])
{
	A.defaults();
	A.args(argc, argv);

	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	auto screen_width = fullscreen ? DM.w : width;
	auto screen_height = fullscreen ? DM.h : height;

	RenderWindow window("Huygens", screen_width, screen_height, highDPI, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); 

	if (mouse)
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	else
		SDL_SetRelativeMouseMode(SDL_TRUE);

	A.prepare();
	A.startup(); // startup audio engine

	bool running = true;
	SDL_Event event;

	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				running = false;

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

				if (event.key.keysym.sym == SDLK_UP)
				{
					A.tempo(3.0 / 2);
				}
				if (event.key.keysym.sym == SDLK_DOWN)
				{
					A.tempo(2.0 / 3);
				}

				if (event.key.keysym.sym == SDLK_RIGHT)
				{
					A.tempo(4.0 / 3);
				}
				if (event.key.keysym.sym == SDLK_LEFT)
				{
					A.tempo(3.0 / 4);
				}

				if (event.key.keysym.sym == SDLK_SPACE)
				{
					A.toggle();
				}
				
			}
		}

		window.color(0,0,0);
		window.clear();
		
		window.display();
	}

	A.shutdown();
	window.~RenderWindow();

	SDL_Quit();

	return 0;
}
