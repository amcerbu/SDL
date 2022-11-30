SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/hellomake
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
CFLAGS   := -Wall
LDFLAGS  := -Llib
LDLIBS   := -lm

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
    $(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
    $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
    mkdir -p $@

clean:
    @$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)

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
