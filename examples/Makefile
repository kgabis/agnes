CC = gcc
CFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter

CPPC = g++
CPPFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter

all: simple_sdl2 simple_sdl2_cpp

.PHONY: simple_sdl2
simple_sdl2: simple_sdl2.c
	$(CC) $(CFLAGS) -lsdl2 -o $@ $<

.PHONY: simple_sdl2_cpp
simple_sdl2: simple_sdl2.c
	$(CPPC) $(CPPFLAGS) -lsdl2 -o $@ $<

clean:
	rm -rf simple_sdl2 simple_sdl2_cpp *.dSYM *.o
