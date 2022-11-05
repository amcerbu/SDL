#include <SDL2/SDL.h>
#include <iostream>

#include "RenderWindow.h"
#include "Audio.h"

const int width = 800;
const int height = 800;
const bool highDPI = true;

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

	RenderWindow window("Hierarchy", width, height, highDPI); 
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
					running = false;
			}
		}

		window.color(0,0,0);
		window.clear();
		A.graphics(&window, width, height, 60);
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
