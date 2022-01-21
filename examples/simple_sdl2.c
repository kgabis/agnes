#include <assert.h>
#include <string.h>

#include <SDL.h>

#include "../agnes.h"

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 480

static void get_input(const Uint8 *state, agnes_input_t *out_input);
static void* read_file(const char *filename, size_t *out_len);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s game.nes\n", argv[0]);
        return 1;
    }

    const char *ines_name = argv[1];

    size_t ines_data_size = 0;
    void* ines_data = read_file(ines_name, &ines_data_size);
    if (ines_data == NULL) {
        fprintf(stderr, "Reading %s failed.\n", ines_name);
        return 1;
    }
    
    agnes_t *agnes = agnes_make();
    if (agnes == NULL) {
        fprintf(stderr, "Making agnes failed.\n");
        return 1;
    }

    bool ok = agnes_load_ines_data(agnes, ines_data, ines_data_size);
    if (!ok) {
        fprintf(stderr, "Loading %s failed.\n", ines_name);
        return 1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Initializing SDL failed.\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("agnes", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Surface *surface = SDL_CreateRGBSurface(0, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, 32, RMASK, GMASK, BMASK, AMASK);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);

    SDL_Rect window_size = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

    agnes_input_t input;

    while (true) {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            break;
        }

        const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);

        if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
            break;
        }

        get_input(keyboard_state, &input);

        agnes_set_input(agnes, &input, NULL);

        ok = agnes_next_frame(agnes);
        if (!ok) {
            fprintf(stderr, "Getting next frame failed.\n");
            return 1;
        }

        uint32_t *pixels = (uint32_t*)surface->pixels;
        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                agnes_color_t c = agnes_get_screen_pixel(agnes, x, y);
                int ix = (y * AGNES_SCREEN_WIDTH) + x;
                uint32_t c_val = c.a << 24 | c.r << 16 | c.g << 8 | c.b;
                pixels[ix] = c_val;
            }
        }

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

static void get_input(const Uint8 *state, agnes_input_t *out_input) {
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

static void* read_file(const char *filename, size_t *out_len) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    long pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        return NULL;
    }
    size_t file_size = pos;
    rewind(fp);
    unsigned char *file_contents = (unsigned char *)malloc(file_size);
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1) {
        if (ferror(fp)) {
            fclose(fp);
            free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    *out_len = file_size;
    return file_contents;
}
