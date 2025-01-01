#ifndef NOSDL_H_INCLUDED
#define NOSDL_H_INCLUDED


#include "noSDL_keysym.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_INIT_VIDEO         1
#define SDL_INIT_NOPARACHUTE   2

#define SDL_SWSURFACE 1

#define SDL_QUIT            1
#define SDL_KEYDOWN         2
#define SDL_MOUSEBUTTONDOWN 3
#define SDL_MOUSEBUTTONUP   4
#define SDL_MOUSEMOTION     5
#define SDL_KEYUP           6
#define SDL_MOD_KEYDOWN     7
#define SDL_MOD_KEYUP       8

#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3

typedef struct SDL_button {
    int button;
} SDL_button;

typedef struct SDL_Event {
    int type;

    SDL_button button;

    int key_keysym_sym;
    int key_keysym_mod;

    int motion_xrel;
    int motion_yrel;
} SDL_Event;

typedef struct SDL_Surface {
    void *pixels;
    int w;
    int h;
} SDL_Surface;

typedef struct SDL_Rect {
    int x;
    int y;
    int w;
    int h;
} SDL_Rect;

typedef unsigned int Uint32;
typedef unsigned int SDL_GrabMode;

#define SDL_TRUE 1
#define SDL_FALSE 0

int SDL_PollEvent(SDL_Event *event);
int SDL_Init(Uint32 flags);
int SDL_ShowCursor(int toggle);
void SDL_Delay(Uint32 ms);
int SDL_Flip(SDL_Surface *screen);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
SDL_Surface * SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
SDL_Surface * SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

void SDL_WM_SetCaption(const char *title, const char *icon);
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode);

void SDL_Kernel_Log(const char *line);


void SDL_wrapStartTimer();
unsigned SDL_wrapCheckTimer();
unsigned SDL_wrapCheckTimerMs();

#ifdef __cplusplus
}
#endif
#endif // NOSDL_H_INCLUDED
