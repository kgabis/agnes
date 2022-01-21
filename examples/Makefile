CC = gcc
CFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter

CPPC = g++
CPPFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter

SDLCONFIG = $(shell sdl2-config --cflags --libs)

all: simple_sdl2 simple_sdl2_cpp

.PHONY: simple_sdl2
simple_sdl2: simple_sdl2.c ../agnes.c
	$(CC) $(CFLAGS) $(SDLCONFIG) -o $@ $^

.PHONY: simple_sdl2_cpp
simple_sdl2_cpp: simple_sdl2.c ../agnes.c
	$(CPPC) $(CPPFLAGS) $(SDLCONFIG) -o $@ $^

clean:
	rm -rf simple_sdl2 simple_sdl2_cpp *.dSYM *.o
