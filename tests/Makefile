CC = gcc
CFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter
SDLCONFIG = $(shell sdl2-config --cflags --libs)
all: player player_sdl recorder

.PHONY: player player_sdl recorder

player: player.c tests_common.c deps/parson.c ../agnes.c
	$(CC) $(CFLAGS) -DAGNES_PLAYER -o $@ $?

player_sdl: player.c tests_common.c deps/parson.c ../agnes.c
	$(CC) $(CFLAGS) $(SDLCONFIG) -DAGNES_PLAYER -DCOMPILE_WITH_SDL -o $@ $^

recorder: recorder.c tests_common.c deps/parson.c ../agnes.c
	$(CC) $(CFLAGS) -DAGNES_RECORDER $(SDLCONFIG) -o $@ $^

clean:
	rm -rf player recorder player_sdl *.dSYM *.o