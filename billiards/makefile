target = Billiards

LIBS = -lSDL2 -lSDL2_image -framework OpenGL
CFLAGS = -std=c++17 -Ofast
INC = -I ./include -I ../lib/include/graphics

objects = main.o \
	$(patsubst %.cpp, %.o, $(wildcard ../lib/src/graphics/*.cpp)) \
	$(patsubst %.cpp, %.o, $(wildcard ./src/*.cpp))

rebuildables = $(objects) $(target)

$(target): $(objects)
	g++ -o $(target) $(objects) $(LIBS) $(CFLAGS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS) $(INC)

.PHONEY:
clean:
	rm $(rebuildables)
