#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"
#include "Audio.h"
#include "Multitouch.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;
bool fullscreen = false;

const int bsize = 64;

Audio A(bsize, SR);
Multitouch M;

int main(int argc, char* argv[])
{
	A.defaults();
	A.args(argc, argv);

	M.setup();

	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	auto screen_width = fullscreen ? DM.w : width;
	auto screen_height = fullscreen ? DM.h : height;

	RenderWindow window("Hierarchy", screen_width, screen_height, highDPI, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); 
	// RenderWindow window2("Scope", width, height, highDPI); 

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
			}

			M.process_event(event);
		}

		window.color(0,0,0);
		window.clear();

		M.tick(screen_width, screen_height, width, height);

		M.draw(&window, screen_width, screen_height, width, height);
		A.graphics(&window, screen_width, screen_height, width, height, 60);
		
		window.display();

		// window2.clear();
		// A.scope(&window2);
		// window2.display();
	}

	A.shutdown();
	window.~RenderWindow();
	// window2.~RenderWindow();

	SDL_Quit();

	return 0;
}
