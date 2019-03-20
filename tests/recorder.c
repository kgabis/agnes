#include "tests_common.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "deps/kgflags.h"
#include "deps/parson.h"

#ifdef AGNES_XCODE
#include "agnes.h"
#else
#include "../agnes.h"
#endif

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 480

static void get_input_1(const Uint8 *state, agnes_input_t *out_input);
static void get_input_2(const Uint8 *state, agnes_input_t *out_input);

#ifdef AGNES_RECORDER
int main(int argc, char **argv) {
#else
int recorder_main(int argc, char **argv) {
#endif
    const char *game = NULL;
    kgflags_string("game", NULL, "Path of a game to record.", true, &game);

    const char *output = NULL;
    kgflags_string("output", NULL, "Output path.", true, &output);

    if (!kgflags_parse(argc, argv)) {
        kgflags_print_errors();
        kgflags_print_usage();
        return 1;
    }


    size_t ines_data_size = 0;
    void* ines_data = read_file(game, &ines_data_size);
    assert(ines_data);

    agnes_t *agnes = agnes_make();
    assert(agnes);
    bool ok = agnes_load_ines_data(agnes, ines_data, ines_data_size);
    assert(ok);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        perror("Could not initialize SDL");
        puts("Error!");
        return 1;
    }

    JSON_Value *json_val = json_value_init_object();
    JSON_Object *res_obj = json_object(json_val);

    uint32_t ines_file_hash = djb2_hash(ines_data, ines_data_size);
    json_object_set_number(res_obj, "ines_hash", ines_file_hash);

    JSON_Value *arr_val = json_value_init_array();
    JSON_Array *arr = json_array(arr_val);
    json_object_set_value(res_obj, "frame_data", arr_val);

    SDL_Window *window = SDL_CreateWindow("agnes", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Surface *surface = SDL_CreateRGBSurface(0, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, 32, RMASK, GMASK, BMASK, AMASK);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);

    SDL_Rect window_size = {.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};

    agnes_input_t input_1, input_2;

    agnes_state_t *state_dump = alloca(agnes_state_size());

    while (true) {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            break;
        }

        const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);

        if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
            JSON_Status status = json_serialize_to_file_pretty(json_val, output);
            assert(status == JSONSuccess);
            break;
        }

        if (keyboard_state[SDL_SCANCODE_T]) {
            agnes_dump_state(agnes, state_dump);
        } else if (keyboard_state[SDL_SCANCODE_Y]) {
            agnes_restore_state(agnes, state_dump);
        }

        JSON_Value* frame_value = json_value_init_object();
        JSON_Object* frame_object = json_object(frame_value);
        json_array_append_value(arr, frame_value);

        get_input_1(keyboard_state, &input_1);
        get_input_2(keyboard_state, &input_2);

        unsigned in_1_num = input_to_number(&input_1);
        if (in_1_num) {
            json_object_set_number(frame_object, "in_1", in_1_num);
        }

        unsigned in_2_num = input_to_number(&input_2);
        if (in_2_num) {
            json_object_set_number(frame_object, "in_2", in_2_num);
        }

        agnes_set_input(agnes, &input_1, &input_2);

        ok = agnes_next_frame(agnes);
        assert(ok);

        uint32_t pixels_hash = DJB2_INITIAL_HASH;
        uint32_t *pixels = (uint32_t*)surface->pixels;
        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                agnes_color_t c = agnes_get_screen_pixel(agnes, x, y);
                int ix = (y * AGNES_SCREEN_WIDTH) + x;
                uint32_t c_val = c.a << 24 | c.r << 16 | c.g << 8 | c.b;
                pixels[ix] = c_val;
                pixels_hash = djb2_hash_incremental(pixels_hash, c_val);
            }
        }

        json_object_set_number(frame_object, "hash", pixels_hash);

        SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
        SDL_RenderCopy(sdl_renderer, texture, NULL, &window_size);
        SDL_RenderPresent(sdl_renderer);
    }

    agnes_destroy(agnes);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

static void get_input_1(const Uint8 *state, agnes_input_t *out_input) {
    memset(out_input, 0, sizeof(agnes_input_t));

    if (state[SDL_SCANCODE_Z])      out_input->a = true;
    if (state[SDL_SCANCODE_X])      out_input->b = true;
    if (state[SDL_SCANCODE_LEFT])   out_input->left = true;
    if (state[SDL_SCANCODE_RIGHT])  out_input->right = true;
    if (state[SDL_SCANCODE_UP])     out_input->up = true;
    if (state[SDL_SCANCODE_DOWN])   out_input->down = true;
    if (state[SDL_SCANCODE_RSHIFT]) out_input->select = true;
    if (state[SDL_SCANCODE_RETURN]) out_input->start = true;
}

static void get_input_2(const Uint8 *state, agnes_input_t *out_input) {
    memset(out_input, 0, sizeof(agnes_input_t));

    if (state[SDL_SCANCODE_C])      out_input->a = true;
    if (state[SDL_SCANCODE_V])      out_input->b = true;
    if (state[SDL_SCANCODE_A])      out_input->left = true;
    if (state[SDL_SCANCODE_D])      out_input->right = true;
    if (state[SDL_SCANCODE_W])      out_input->up = true;
    if (state[SDL_SCANCODE_S])      out_input->down = true;
    if (state[SDL_SCANCODE_LSHIFT]) out_input->select = true;
    if (state[SDL_SCANCODE_TAB])    out_input->start = true;
}
