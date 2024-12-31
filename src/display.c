// Simple display driver

#include "display.h"
#include "devices.h"
#include "util.h"
// #include <SDL/SDL.h>
#include "noSDL.h"
#include <stdlib.h>

#define DISPLAY_LOG(x, ...) LOG("DISPLAY", x, ##__VA_ARGS__)
#define DISPLAY_FATAL(x, ...)          \
    do {                               \
        DISPLAY_LOG(x, ##__VA_ARGS__); \
        ABORT();                       \
    } while (0)

static SDL_Surface* surface = NULL;
static SDL_Surface* screen = NULL;
static void* surface_pixels;

void* display_get_pixels(void)
{
    return surface_pixels;
}

static int h, w, mouse_enabled = 0, mhz_rating = -1;

static void display_set_title(void)
{
    char buffer[1000];
    UNUSED(mhz_rating);
    sprintf(buffer, "Halfix x86 Emulator - [%d x %d] - %s", w, h,
        mouse_enabled ? "Press ESC to release mouse" : "Right-click to capture mouse");
    SDL_WM_SetCaption(buffer, "Halfix");
}

void display_update_cycles(int cycles_elapsed, int us)
{
    mhz_rating = (int)((double)cycles_elapsed / (double)us);
    display_set_title();
}

// Nasty hack: don't update until screen has been resized (screen is resized during VGABIOS init)
static int resized = 0;
void display_set_resolution(int width, int height)
{
    resized = 1;
    if ((!width && !height)) {
        display_set_resolution(640, 480);
        return;
    }
    DISPLAY_LOG("Changed resolution to w=%d h=%d\n", width, height);

    if (surface_pixels)
        free(surface_pixels);
    surface_pixels = malloc(width * height * 4);

    if (surface)
        SDL_FreeSurface(surface);
    if (screen)
        SDL_FreeSurface(screen);

    screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
    surface = SDL_CreateRGBSurfaceFrom(surface_pixels, width, height, 32,
        width * 4, // pitch -- number of bytes per row
        0x00ff0000, // red
        0x0000ff00, // green
        0x000000ff, // blue
        0xff000000); // alpha
    w = width;
    h = height;
    display_set_title();

}

void display_update(int scanline_start, int scanlines)
{
    if (!resized)
        return;
    if ((w == 0) || (h == 0))
        return;
    if ((scanline_start + scanlines) > h) {
        DISPLAY_LOG("%d x %d [%d %d]\n", w, h, scanline_start, scanlines);
        ABORT();
    } else {
        // DISPLAY_LOG("Updating %d scanlines starting from %d\n", scanlines, scanline_start);
        SDL_Rect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = w;
        rect.h = h;

        SDL_BlitSurface(surface, &rect, screen, &rect);
        SDL_Flip(screen);
    }
}

static int input_captured = 0;
static void display_mouse_capture_update(int y)
{
    input_captured = y;
    SDL_WM_GrabInput(y);
    SDL_ShowCursor(SDL_TRUE ^ y);
    mouse_enabled = y;
    display_set_title();
}

void display_release_mouse(void)
{
    display_mouse_capture_update(0);
}

#define KEYMOD_INVALID -1

static int sdl_keysym_to_scancode(int sym)
{
    return sym;
}
static inline void display_kbd_send_key(int k)
{
    if (k == KEYMOD_INVALID)
        return;
    if (k & 0xFF00)
        kbd_add_key(k >> 8);
    kbd_add_key(k & 0xFF);
}
// XXX: work around a SDL bug?
static inline void send_keymod_scancode(int k, int or)
{
    if (k & KMOD_ALT) {
        display_kbd_send_key(0xE038 | or);
    }
}

void display_handle_events(void)
{
    SDL_Event event;
    int k;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN: {
            //printf("KeyDown\n");
            display_set_title();
            send_keymod_scancode(event.key_keysym_mod, 0);
            display_kbd_send_key(sdl_keysym_to_scancode(event.key_keysym_sym));
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            k = event.type == SDL_MOUSEBUTTONDOWN ? MOUSE_STATUS_PRESSED : MOUSE_STATUS_RELEASED;
            //printf("Mouse button %s\n", k == MOUSE_STATUS_PRESSED ? "down" : "up");
            switch (event.button.button) {
            case SDL_BUTTON_LEFT:
                kbd_mouse_down(k, MOUSE_STATUS_NOCHANGE, MOUSE_STATUS_NOCHANGE);
                break;
            case SDL_BUTTON_MIDDLE:
                kbd_mouse_down(MOUSE_STATUS_NOCHANGE, k, MOUSE_STATUS_NOCHANGE);
                break;
            case SDL_BUTTON_RIGHT:
                if (k == MOUSE_STATUS_PRESSED && !mouse_enabled) // Don't send anything
                    display_mouse_capture_update(1);
                else
                    kbd_mouse_down(MOUSE_STATUS_NOCHANGE, MOUSE_STATUS_NOCHANGE, k);
                break;
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            if (input_captured)
                kbd_send_mouse_move(event.motion_xrel, event.motion_yrel);
            break;
        }
        case SDL_KEYUP: {
            //printf("KeyUp\n");
            int c = sdl_keysym_to_scancode(event.key_keysym_sym);
            send_keymod_scancode(event.key_keysym_mod, 0x80);
            display_kbd_send_key(c | 0x80);
            break;
        }
        }
    }
}

// Send the CTRL+ALT+DEL sequence to the emulator
void display_send_ctrl_alt_del(int down)
{
    down = down ? 0 : 0x80;
    display_kbd_send_key(0x1D | down); // CTRL
    display_kbd_send_key(0xE038 | down); // ALT
    display_kbd_send_key(0xE053 | down); // DEL
}

void display_send_scancode(int key)
{
    display_kbd_send_key(key);
}

void display_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE))
        DISPLAY_FATAL("Unable to initialize SDL");

    display_set_title();
    display_set_resolution(640, 480);

    screen = SDL_SetVideoMode(640, 400, 32, SDL_SWSURFACE);

    resized = 0;
}
void display_sleep(int ms)
{
    SDL_Delay(ms);
}

