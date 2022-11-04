target = Audiovisual

# grab system info
uname_s := $(shell uname -s)
$(info uname_s=$(uname_s))
uname_p := $(shell uname -p)
$(info uname_p=$(uname_p))

# system specific variables, add more here
INCDIR.Darwin.i386 := -I /usr/local/include/eigen3 -I /usr/local/include/rtmidi
LINKDIR.Darwin.i386 := 

INCDIR.Darwin.arm64 := -I /opt/homebrew/include -I /opt/homebrew/include/eigen3 -I /opt/homebrew/include/rtmidi
LINKDIR.Darwin.arm64 := -L /opt/homebrew/lib

INCDIR += $(INCDIR.$(uname_s).$(uname_p))
LINKDIR += $(LINKDIR.$(uname_s).$(uname_p))

# $(info INCDIR=$(INCDIR))
# $(info LINKDIR=$(LINKDIR))

LIBS = $(LINKDIR) -lSDL2 -lSDL2_image -lm -lfftw3 -lportaudio -lncurses -lrtmidi
CFLAGS = -std=c++17 -O3
INC = -I ./include -I ../lib/include/graphics -I ../lib/include/audio \
	  $(INCDIR)

priv_objects = main.o \
			   $(patsubst %.cpp, %.o, $(wildcard ./src/*.cpp))

lib_objects  = $(patsubst %.cpp, %.o, $(wildcard ../lib/src/graphics/*.cpp)) \
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
