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
	A.args(argc, argv);

	if (SDL_Init(SDL_INIT_VIDEO) > 0)
	{
		std::cout << "SDL_Init has failed. SDL_ERROR: " << SDL_GetError() << std::endl;
	}

	RenderWindow window("Metronome", width, height, highDPI); 

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

		window.clear();

		// your drawing code here

		window.display();
	}

	A.shutdown();
	window.~RenderWindow();

	SDL_Quit();

	return 0;
}
