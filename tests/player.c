#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef COMPILE_WITH_SDL
#ifdef WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#endif

#include "deps/parson.h"

#include "deps/kgflags.h"

#ifdef AGNES_XCODE
#include "agnes.h"
#else
#include "../agnes.h"
#endif

#include "tests_common.h"

#ifdef COMPILE_WITH_SDL

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 480

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static SDL_Surface *g_surface;
static SDL_Texture *g_texture;

#endif /* COMPILE_WITH_SDL */

typedef enum {
    PLAYER_MODE_VERIFY,
    PLAYER_MODE_UPDATE
} player_mode_t;

static bool init_sdl(void);
static void destroy_sdl(void);
static bool check_sdl_quit_event(void);
static void set_sdl_pixel(int x, int y, uint32_t val, unsigned frame);
static void present_sdl(unsigned frame);
static bool get_file_name(const char* recording_path, char *buf, int buf_len);

static bool play_game(const char *ines_path, const char *rec_path, int max_frames, bool *out_should_quit);

static bool g_vsync = false;
static bool g_render = true;
static int  g_frame_skip = 1;

player_mode_t g_mode = PLAYER_MODE_VERIFY;

#ifdef AGNES_PLAYER
int main(int argc, char **argv) {
#else
int player_main(int argc, char **argv) {
#endif
    kgflags_string_array_t recs;
    kgflags_string_array("recordings", "Array of recordings to play.", true, &recs);

    const char *roms_dir = NULL;
    kgflags_string("roms-dir", ".", "Directory with NES roms used for recordings.", true, &roms_dir);

    kgflags_bool("vsync", true, "Vsync when playing games.", false, &g_vsync);

    kgflags_bool("render", true, "Enable or disable frame rendering.", false, &g_render);

    int max_frames = 0;
    kgflags_int("max-frames", 0, "Maximum number of frames played per game", false, &max_frames);

    kgflags_int("frame-skip", 1, "Render every n-th frame.", false, &g_frame_skip);

    const char *mode_str = NULL;
    kgflags_string("mode", NULL, "Mode (verify or update)", true, &mode_str);

    bool print_time = false;
    kgflags_bool("print-time", false, "Primt how long it took to run", false, &print_time);

    if (!kgflags_parse(argc, argv)) {
        kgflags_print_errors();
        kgflags_print_usage();
        return 1;
    }

    if (strcmp(mode_str, "verify") == 0) {
        g_mode = PLAYER_MODE_VERIFY;
    } else if (strcmp(mode_str, "update") == 0) {
        g_mode = PLAYER_MODE_UPDATE;
    } else {
        assert(false);
    }

    bool ok = init_sdl();
    assert(ok);

    int tests_failed = 0;
    clock_t start = clock();
    for (int i = 0; i < kgflags_string_array_get_count(&recs); i++) {
        const char* rec_path = kgflags_string_array_get_item(&recs, i);
        assert(rec_path);
        char rec_name_buf[512];
        ok = get_file_name(rec_path, rec_name_buf, ARRAY_SIZE(rec_name_buf));
        assert(ok);
        char rom_path_buf[512];
        sprintf(rom_path_buf, "%s/%s.nes", roms_dir, rec_name_buf);
        bool should_quit = false;
        printf("Playing: %s\n", rom_path_buf);
        ok = play_game(rom_path_buf, rec_path, max_frames, &should_quit);
        if (ok) {
            printf("\tOK\n");
        } else {
            printf("\tFAIL\n");
            tests_failed++;
        }
        if (should_quit) {
            break;
        }
    }
    clock_t end = clock();
    float seconds = (float)(end - start) / CLOCKS_PER_SEC;

    if (print_time) {
        printf("Finished in %1.4g seconds\n", seconds);
    }

    destroy_sdl();

    return tests_failed;
}

static bool play_game(const char *game_path, const char *rec_path, int max_frames, bool *out_should_quit) {
    size_t ines_data_size = 0;
    void* ines_data = read_file(game_path, &ines_data_size);
    if (!ines_data) {
        printf("Reading failed: %s\n", game_path);
        return false;
    }

    agnes_t *agnes = agnes_make();
    assert(agnes);
    bool ok = agnes_load_ines_data(agnes, ines_data, ines_data_size);
    if (!ok) {
        printf("Loading ines data failed\n");
        return false;
    }

    JSON_Value *recording_val = json_parse_file(rec_path);
    if (!recording_val) {
        printf("Parsing recording failed: %s\n", rec_path);
        return false;
    }

    JSON_Object *recording_obj = json_object(recording_val);

    uint32_t current_ines_hash = djb2_hash(ines_data, ines_data_size);
    uint32_t loaded_ines_hash = (uint32_t)json_object_get_number(recording_obj, "ines_hash");
    assert(current_ines_hash == loaded_ines_hash);

    JSON_Array *frame_array = json_object_get_array(recording_obj, "frame_data");

    agnes_input_t input_1, input_2;

    unsigned frame_number = 0;

    bool update_recording = false;

    bool result_ok = true;
    while (true) {
        bool quit = check_sdl_quit_event();
        if (quit) {
            *out_should_quit = true;
            break;
        }

        if (frame_number >= json_array_get_count(frame_array) || (max_frames > 0 && frame_number >= (unsigned)max_frames)) {
            break;
        }

        JSON_Object* frame_object = json_array_get_object(frame_array, frame_number);

        unsigned in_1_num = json_object_get_number(frame_object, "in_1");
        unsigned in_2_num = json_object_get_number(frame_object, "in_2");

        number_to_input(in_1_num, &input_1);
        number_to_input(in_2_num, &input_2);

        agnes_set_input(agnes, &input_1, &input_2);

        while (true) {
            bool new_frame = false;
            ok = agnes_tick(agnes, &new_frame);
            assert(ok);
            if (new_frame) {
                break;
            }
        }

        uint32_t current_pixels_hash = DJB2_INITIAL_HASH;

        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                agnes_color_t c = agnes_get_screen_pixel(agnes, x, y);
                uint32_t c_val = c.a << 24 | c.r << 16 | c.g << 8 | c.b;
                set_sdl_pixel(x, y, c_val, frame_number);
                current_pixels_hash = djb2_hash_incremental(current_pixels_hash, c_val);
            }
        }

        uint32_t loaded_pixels_hash = json_object_get_number(frame_object, "hash");

        switch (g_mode) {
            case PLAYER_MODE_VERIFY: {
                if (loaded_pixels_hash != current_pixels_hash) {
                    printf("Invalid frame: %d\n", frame_number);
                    result_ok = false;
                }
//                assert(loaded_pixels_hash == current_pixels_hash);
                break;
            }
            case PLAYER_MODE_UPDATE: {
                if (loaded_pixels_hash != current_pixels_hash) {
                    printf("Different frame hash at frame %d\n", frame_number);
                    json_object_set_number(frame_object, "hash", current_pixels_hash);
                    update_recording = true;
                }
                break;
            }
        }

        present_sdl(frame_number);

        frame_number++;
    }
    
    agnes_destroy(agnes);

    if (update_recording) {
        printf("Updating recording %s\n", rec_path);
        json_serialize_to_file_pretty(recording_val, rec_path);
    }

    return result_ok;
}

static bool init_sdl(void) {
    if (!g_render) {
        return true;
    }
#ifdef COMPILE_WITH_SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        perror("Could not initialize SDL");
        puts("Error!");
        return false;
    }

    g_window = SDL_CreateWindow("agnes", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | (g_vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    g_surface = SDL_CreateRGBSurface(0, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, 32, RMASK, GMASK, BMASK, AMASK);
    g_texture = SDL_CreateTextureFromSurface(g_renderer, g_surface);
    return true;
#else /* COMPILE_WITH_SDL */
    return true;
#endif
}

static void destroy_sdl() {
    if (!g_render) {
        return;
    }
#ifdef COMPILE_WITH_SDL
    SDL_DestroyTexture(g_texture);
    SDL_FreeSurface(g_surface);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
#endif /* COMPILE_WITH_SDL */
}

static bool check_sdl_quit_event() {
    if (!g_render) {
        return false;
    }
#ifdef COMPILE_WITH_SDL
    SDL_Event e;
    SDL_PollEvent(&e);

    if (e.type == SDL_QUIT) {
        return true;
    }

    const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);

    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        return true;
    }

    return false;
#else /* COMPILE_WITH_SDL */
    return false;
#endif
}

static void set_sdl_pixel(int x, int y, uint32_t val, unsigned frame) {
    if (!g_render) {
        return;
    }
#ifdef COMPILE_WITH_SDL
    int ix = (y * AGNES_SCREEN_WIDTH) + x;
    if (frame % g_frame_skip == 0) {
        ((uint32_t*)g_surface->pixels)[ix] = val;
    }
#endif /* COMPILE_WITH_SDL */
}

static void present_sdl(unsigned frame) {
    if (!g_render) {
        return;
    }
#ifdef COMPILE_WITH_SDL
    if (frame % g_frame_skip == 0) {
        SDL_Rect window_size = {.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};
        SDL_UpdateTexture(g_texture, NULL, g_surface->pixels, g_surface->pitch);
        SDL_RenderCopy(g_renderer, g_texture, NULL, &window_size);
        SDL_RenderPresent(g_renderer);
    }
#endif /* COMPILE_WITH_SDL */
}

static bool get_file_name(const char *path, char *buf, int buf_len) {
    memset(buf, 0, buf_len);
    const char *path_end = strrchr(path, '/');
    const char *ext_end = strrchr(path, '.');

    const char *to_copy_start = path_end != NULL ? (path_end  + 1) : path;
    long to_copy_len = ext_end != NULL ? (ext_end - to_copy_start) : strlen(to_copy_start);
    if (buf_len < (to_copy_len + 1)) {
        return false;
    }
    strncpy(buf, to_copy_start, to_copy_len);
    return true;
}
