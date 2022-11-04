target = Fm

LIBS = -lSDL2 -lSDL2_image -lm -lfftw3 -lportaudio -lncurses -lrtmidi
CFLAGS = -std=c++17 -Ofast
INC = -I ./include -I ../lib/include/graphics -I ../lib/include/audio \
	  -I /usr/local/include/eigen3 -I /usr/local/include/rtmidi

objects = main.o \
		  $(patsubst %.cpp, %.o, $(wildcard ../lib/src/graphics/*.cpp)) \
		  $(patsubst %.cpp, %.o, $(wildcard ../lib/src/audio/*.cpp)) \
		  $(patsubst %.cpp, %.o, $(wildcard ./src/*.cpp))

rebuildables = $(objects) $(target)

$(target): $(objects)
	g++ -o $(target) $(objects) $(LIBS) $(CFLAGS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS) $(INC)

.PHONEY:
clean:
	rm $(rebuildables)
