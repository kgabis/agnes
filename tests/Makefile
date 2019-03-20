CC = gcc
CFLAGS = -O3 -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter

all: player player_sdl recorder

.PHONY: player player_sdl recorder

player: player.c tests_common.c deps/parson.c
	$(CC) $(CFLAGS) -DAGNES_PLAYER -o $@ $?

player_sdl: player.c tests_common.c deps/parson.c
	$(CC) $(CFLAGS) -lsdl2 -DAGNES_PLAYER -DCOMPILE_WITH_SDL -o $@ $?

recorder: recorder.c tests_common.c deps/parson.c
	$(CC) $(CFLAGS) -DAGNES_RECORDER -lsdl2 -o $@ $?

clean:
	rm -rf player recorder player_sdl *.dSYM *.o