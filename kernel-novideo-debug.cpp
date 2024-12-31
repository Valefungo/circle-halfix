//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "noSDL.h"

#ifdef __cplusplus
extern "C" {
#endif

    // the real main
    int main_halfix_unix(int argc, char **argv);

#ifdef __cplusplus
}
#endif

static CKernel *this_kernel;


int SDL_Init(Uint32 flags)
{
    // unused, nothing real to initialize
    return SDL_FALSE;
}

int SDL_ShowCursor(int toggle)
{
    // unused
    return toggle;
}

void SDL_Delay(Uint32 ms)
{
    // unused
}

void SDL_WM_SetCaption(const char *title, const char *icon)
{
    // absolutely not
}

SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode)
{
    // it's always grabbed
    return mode;
}

int SDL_PollEvent(SDL_Event *event)
{
    return 0;
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    free(surface);
}

int SDL_Flip(SDL_Surface *screen)
{
    // this_kernel->screen2d->UpdateDisplay();
    return 0;
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    // printf( "SDL_BlitSurface SRC %d %d %d, %d   DST %d %d %d %d\n", srcrect->x, srcrect->y, srcrect->w, srcrect->h, dstrect->x, dstrect->y, dstrect->w, dstrect->h);
    // printf( "SDL_BlitSurface %d, %d\n", src->w, src->h);
    // this_kernel->screen2d->DrawImage(0, 0, src->w, src->h, (TScreenColor *)src->pixels);
    return 0;
}

SDL_Surface * SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{ 
    printf( "SDL_SetVideoMode %d, %d, %d, %d\n", width, height, bpp, DEPTH);
    this_kernel->Pause((char *)"SDL_SetVideoMode");

    // this_kernel->screen2d->Resize(width, height);
    

    SDL_Surface *su = (SDL_Surface *)malloc(sizeof(SDL_Surface));
    su->w = width;
    su->h = height;
    su->pixels = NULL;

    return su;
}

SDL_Surface * SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    printf( "SDL_CreateRGBSurfaceFrom %d, %d, %d, %d\n", width, height, depth, pitch);
    this_kernel->Pause((char *)"SDL_SetVideoMode");

    SDL_Surface *su = (SDL_Surface *)malloc(sizeof(SDL_Surface));
    su->w = width;
    su->h = height;
    su->pixels = pixels;

    return su;
}

using namespace std;

CKernel::CKernel (void) : CStdlibAppStdio ("11-halfix")
{
    this_kernel = this;
    mActLED.Blink (5);  // show we are alive
}

void CKernel::Pause(char *m)
{
    mLogger.Write (GetKernelName (), LogNotice, "Pause: %s... Type some characters and hit <RETURN>", m);
    // printf("Pause: %s...\n", m);
    // printf("Type some characters and hit <RETURN>\n");

    char line[200];
    if (fgets(line, sizeof(line), stdin) != nullptr)
    {
    	// printf("Read '%s' from stdin...\n", line);
    	mLogger.Write (GetKernelName (), LogNotice, "Read '%s' from stdin...", line);
    }
    else
    {
            perror("fgets returned NULL");
    }
}

bool CKernel::Initialize(void)
{
    Pause((char *)"ini-init");

    bool r = CStdlibAppStdio::Initialize();

    Pause((char *)"pre-init");

    screen2d = new C2DGraphics(640, 480);
    screen2d->Initialize();

    Pause((char *)"post-init");

    return r;
}

CStdlibApp::TShutdownMode CKernel::Run (void)
{
    mLogger.Write (GetKernelName (), LogNotice, "C Standard Library stdin/stdout/stderr Demo");
    mLogger.Write (GetKernelName (), LogNotice, "stdio test...");

    Pause((char *)"run");

    // mLogger.Initialize (&m_LogFile);

    mLogger.Write (GetKernelName (), LogNotice, "Compile time: " __DATE__ " " __TIME__);
    mLogger.Write (GetKernelName (), LogNotice, "A timer will stop the loop");

    Pause((char *)"do-run");

    char *argv[] = { (char *)"halfix" };
    int retval = main_halfix_unix(1, argv);
    // int retval = 0;

    mLogger.Write (GetKernelName (), LogNotice, "Shutting down because of %d...", retval);
    if (retval >= 0)
      Pause((char *)"maggiore di zero");
    else
      Pause((char *)"affanculo");

    Pause((char *)"shutdown-run");
    return ShutdownHalt;
}
