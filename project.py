import sys
import os

def main():
	prefix = os.path.dirname(__file__)

	if len(sys.argv) < 2:
		print('Usage: python3 project.py [project-name]')
		return 1

	name = sys.argv[1]
	name = name.lower()

	try:
		os.mkdir(prefix + '/' + name)
	except:
		print('Directory exists already; exiting.')
		return 1
	
	print(f'Creating project \'{name}\'')
	os.mkdir(prefix + f'/{name}/include')
	os.mkdir(prefix + f'/{name}/src')

	target = name.capitalize()

	makefile = f'target = {target}' + '''

# grab system info
uname_s := $(shell uname -s)
uname_m := $(shell uname -m)
# $(info uname_s=$(uname_s))
# $(info uname_m=$(uname_m))

# system specific variables, add more here
INCDIR.Darwin.x86_64 := -I /usr/local/include/eigen3 -I /usr/local/include/rtmidi
LINKDIR.Darwin.x86_64 := 

INCDIR.Darwin.arm64 := -I /opt/homebrew/include -I /opt/homebrew/include/eigen3 -I /opt/homebrew/include/rtmidi
LINKDIR.Darwin.arm64 := -L /opt/homebrew/lib

INCDIR += $(INCDIR.$(uname_s).$(uname_m))
LINKDIR += $(LINKDIR.$(uname_s).$(uname_m))

# $(info INCDIR=$(INCDIR))
# $(info LINKDIR=$(LINKDIR))

LIBS = $(LINKDIR) -lSDL2 -lSDL2_image -lm -lfftw3 -lportaudio -lrtmidi
CFLAGS = -std=c++17 -O3
INC = -I ./include -I ../lib/include/graphics -I ../lib/include/audio $(INCDIR)

priv_objects = main.o $(patsubst %.cpp, %.o, $(wildcard ./src/*.cpp))

lib_objects  = $(patsubst %.cpp, %.o, $(wildcard ../lib/src/graphics/*.cpp)) \\
			   $(patsubst %.cpp, %.o, $(wildcard ../lib/src/audio/*.cpp))

rebuildables = $(priv_objects) $(target)

$(target): $(priv_objects) $(lib_objects)
	g++ -o $(target) $(priv_objects) $(lib_objects) $(LIBS) $(CFLAGS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS) $(INC)

.PHONEY:
clean:
	rm $(rebuildables)

cleanall:
	rm $(rebuildables) $(lib_objects)
'''

	project = '''{
	"folders":
	[
		{
			"path": "bin/..",
			"file_exclude_patterns": ["*.sublime-project"]
		},
		{
			"path": "../lib/",
		}
	],

	"build_systems":
	[
		{
			"name": "Run",
			"working_dir": "${project_path}",
			"cmd":''' + f'" ./{target}",' + '''
			"selector": "source.c++",
			"shell": true
		},
		{
			"name": "Make & run",
			"working_dir": "${project_path}",
			"cmd": "make;''' + f' ./{target}",' + '''
			"selector": "source.c++",
			"shell": true
		}
	]
}
'''

	entry = '''#include <SDL2/SDL.h>
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

	RenderWindow window("''' + f"{target}" + '''", width, height, highDPI); 

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
'''
	audio_h = '''// Audio.h
#pragma once
#include "AbstractAudio.h"

inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

class Audio : public AbstractAudio
{
public:
	using AbstractAudio::AbstractAudio;
	int process(const float* in, float* out, unsigned long frames);
};
'''
	audio_cpp = '''// Audio.cpp
#include "Audio.h"

int Audio::process(const float* in, float* out, unsigned long frames)
{
	for (int i = 0; i < frames; i++)
	{
		for (int j = 0; j < out_chans; j++)
		{
			// your audio code here
			out[j + i * out_chans] = 0;
		}
	}

	return 0;
}
'''

	f = open(prefix + f'/{name}/makefile', 'w')
	f.write(makefile)
	f.close()

	f = open(prefix + f'/{name}/{name}.sublime-project', 'w')
	f.write(project)
	f.close()

	f = open(prefix + f'/{name}/main.cpp', 'w')
	f.write(entry)
	f.close()

	f = open(prefix + f'/{name}/src/Audio.cpp', 'w')
	f.write(audio_cpp)
	f.close()

	f = open(prefix + f'/{name}/include/Audio.h', 'w')
	f.write(audio_h)
	f.close()

	return 0

if __name__ == '__main__':
	main()