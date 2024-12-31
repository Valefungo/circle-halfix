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
#include "noSDL_keysym.h"
#include "png.h"

#define TRUE_RASPI_4

#ifdef __cplusplus
extern "C" {
#endif

    // the real main
    int main_halfix_unix(int argc, char **argv);

    void screenshot(const char *fn, TScreenColor *pixels, int width, int height);

#ifdef __cplusplus
}
#endif

static CKernel *this_kernel;
static int do_screenshot=0;
static int screenshot_count=0;
static int virtual_screen_width = 640;
static int virtual_screen_height = 480;

void screenshot(const char *fn, TScreenColor *pixels, int width, int height)
{
    png_bytep         *b_rgb         = NULL;
    FILE              *fp            = NULL;
    uint32_t           temp          = 0x00000000;

    /* create file */
    fp = fopen(fn, (const char *) "wb");
    if (!fp) {
        return;
    }

    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    b_rgb = (png_bytep *) malloc(sizeof(png_bytep) * height);
    if (b_rgb == NULL) {
        fclose(fp);
        return;
    }

    int start_y = 0;
    int row_len = width;
    int start_x = 0;
    for (int y = 0; y < height; ++y) {
        b_rgb[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
        for (int x = 0; x < width; ++x) {
            if (pixels == NULL)
                memset(&(b_rgb[y][x * 3]), 0x00, 3);
            else {
                temp                  = pixels[((start_y + y) * row_len) + start_x + x];
                b_rgb[y][x * 3]       = (temp >> 16) & 0xff;
                b_rgb[y][(x * 3) + 1] = (temp >> 8) & 0xff;
                b_rgb[y][(x * 3) + 2] = temp & 0xff;
            }
        }
    }

    png_write_info(png_ptr, info_ptr);

    png_write_image(png_ptr, b_rgb);

    png_write_end(png_ptr, NULL);

    /* cleanup heap allocation */
    for (int i = 0; i < height; i++)
        if (b_rgb[i])
            free(b_rgb[i]);

    free(b_rgb);
    fclose(fp);
}

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
    // delay is actually unused but let's take the opportunity to update the USB status
    this_kernel->MsPause(ms);

    this_kernel->UpdateKeyboardAndMouse();
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

SDL_Event static_event;
SDL_Event static_mouse_event;
int lastmousex=0;
int lastmousey=0;

int SDL_PollEvent(SDL_Event *event)
{
    if (static_event.type != 0)
    {
        this_kernel->p_mLogger->Write (this_kernel->GetKernelName(), LogNotice, "KPOLL %d %04X %02X\n", static_event.type, static_event.key_keysym_sym, static_event.key_keysym_mod);

        memcpy(event, &static_event, sizeof(SDL_Event));
        static_event.type = 0;
        return SDL_TRUE;
    }

    if (static_mouse_event.type != 0)
    {
        this_kernel->p_mLogger->Write (this_kernel->GetKernelName(), LogNotice, "MPOLL %d x%d y%d\n", static_mouse_event.type, static_mouse_event.motion_xrel, static_mouse_event.motion_yrel);

        memcpy(event, &static_mouse_event, sizeof(SDL_Event));
        static_mouse_event.type = 0;
        return SDL_TRUE;
    }

    return SDL_FALSE;
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    free(surface);
}

int SDL_Flip(SDL_Surface *screen)
{
    this_kernel->screen2d->UpdateDisplay();

    this_kernel->UpdateKeyboardAndMouse();
    return 0;
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    // this_kernel->p_mLogger.Write (GetKernelName(), LogNotice, "SDL_BlitSurface SRC %d %d %d, %d   DST %d %d %d %d\n", srcrect->x, srcrect->y, srcrect->w, srcrect->h, dstrect->x, dstrect->y, dstrect->w, dstrect->h);
    // this_kernel->p_mLogger.Write (GetKernelName(), LogNotice, "SDL_BlitSurface %d, %d\n", src->w, src->h);

#ifdef TRUE_RASPI_4
    // no, let it stay dirty to be able to see any log
    // this_kernel->screen2d->ClearScreen(BLACK_COLOR);

    this_kernel->screen2d->DrawImage(0, 0, src->w, src->h, (TScreenColor *)src->pixels);
#else
    TScreenColor *scr = (TScreenColor *)src->pixels;

    // emulated raspi 3
    this_kernel->screen2d->ClearScreen(BLACK_COLOR);

    TScreenColor pix[src->w * src->h];
    unsigned int temp;

    // C2DGraphics needs the final uint as ARGB, we have xBGR
    for (int y = 0; y < src->w * src->h; y++) {
        temp   = scr[y];
        unsigned char b = (temp >> 16) & 0xFF;
        unsigned char g = (temp >> 8) & 0xFF;
        unsigned char r = (temp >> 0) & 0xFF;
        pix[y] = COLOR32(r, g, b, 255);
    }

    this_kernel->screen2d->DrawImage(0, 0, src->w, src->h, pix);

#endif

    if (do_screenshot)
    {
        screenshot_count++;

        char fn[200] = "";
        sprintf(fn, "screenblit%d.png", screenshot_count);
        screenshot(fn, (TScreenColor *)src->pixels, src->w, src->h);

        do_screenshot = 0;
    }

    return 0;
}

SDL_Surface * SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{ 
    virtual_screen_width = width;
    virtual_screen_height = height;
    // this_kernel->p_mLogger.Write (GetKernelName(), LogNotice, "SDL_SetVideoMode %d, %d, %d, %d\n", width, height, bpp, DEPTH);

//    if (height != 0)
//        this_kernel->screen2d->Resize(width, height);

    SDL_Surface *su = (SDL_Surface *)malloc(sizeof(SDL_Surface));
    su->w = width;
    su->h = height;
    su->pixels = NULL;

    return su;
}

SDL_Surface * SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    // this_kernel->p_mLogger.Write (GetKernelName(), LogNotice, "SDL_CreateRGBSurfaceFrom %d, %d, %d, %d\n", width, height, depth, pitch);

    SDL_Surface *su = (SDL_Surface *)malloc(sizeof(SDL_Surface));
    su->w = width;
    su->h = height;
    su->pixels = pixels;

    return su;
}

using namespace std;

CKernel::CKernel (void) : CStdlibAppStdio ("circle-halfix")
{
    this_kernel = this;
    p_mLogger = &mLogger;   // make this available to C code
    mActLED.Blink (5);  // show we are alive
}

void CKernel::MsPause(int ms)
{
    mTimer.MsDelay (ms);
}

void CKernel::Pause(char *m)
{
    mLogger.Write (GetKernelName(), LogNotice, "Pause: %s... Type some characters and hit <RETURN>", m);

    /* * /
    char line[200];
    if (fgets(line, sizeof(line), stdin) != nullptr)
    {
    	mLogger.Write (GetKernelName(), LogNotice, "Read '%s' from stdin...", line);
    }
    else
    {
        perror("fgets returned NULL");
    }
    / **/
}

bool CKernel::Initialize(void)
{
    Pause((char *)"ini-init");

    bool r = CStdlibAppStdio::Initialize();

    Pause((char *)"pre-init");

#ifdef TRUE_RASPI_4
    mScreen.Resize(1920, 1080);
    // no vsync
    screen2d = new C2DGraphics(1920, 1080, FALSE, 0);
    screen2d->Initialize();
#else
    // little screen on qemu
    screen2d = new C2DGraphics(1024, 768, FALSE, 0);
    screen2d->Initialize();
#endif

    // https://stackoverflow.com/questions/69558563/how-to-convert-between-keyboard-scan-code-and-usb-keyboard-usage-index
    // https://pastebin.com/5J41WCQP
    //
    kmap_usb_to_ps2[0x01]=0x00FF; // Overrun Error
    kmap_usb_to_ps2[0x02]=0x00FC; // POST Fail
    kmap_usb_to_ps2[0x04]=0x001E; // a A
    kmap_usb_to_ps2[0x05]=0x0030; // b B
    kmap_usb_to_ps2[0x06]=0x002E; // c C
    kmap_usb_to_ps2[0x07]=0x0020; // d D
    kmap_usb_to_ps2[0x08]=0x0012; // e E
    kmap_usb_to_ps2[0x09]=0x0021; // f F
    kmap_usb_to_ps2[0x0A]=0x0022; // g G
    kmap_usb_to_ps2[0x0B]=0x0023; // h H
    kmap_usb_to_ps2[0x0C]=0x0017; // i I
    kmap_usb_to_ps2[0x0D]=0x0024; // j J
    kmap_usb_to_ps2[0x0E]=0x0025; // k K
    kmap_usb_to_ps2[0x0F]=0x0026; // l L
    kmap_usb_to_ps2[0x10]=0x0032; // m M
    kmap_usb_to_ps2[0x11]=0x0031; // n N
    kmap_usb_to_ps2[0x12]=0x0018; // o O
    kmap_usb_to_ps2[0x13]=0x0019; // p P
    kmap_usb_to_ps2[0x14]=0x0010; // q Q
    kmap_usb_to_ps2[0x15]=0x0013; // r R
    kmap_usb_to_ps2[0x16]=0x001F; // s S
    kmap_usb_to_ps2[0x17]=0x0014; // t T
    kmap_usb_to_ps2[0x18]=0x0016; // u U
    kmap_usb_to_ps2[0x19]=0x002F; // v V
    kmap_usb_to_ps2[0x1A]=0x0011; // w W
    kmap_usb_to_ps2[0x1B]=0x002D; // x X
    kmap_usb_to_ps2[0x1C]=0x0015; // y Y
    kmap_usb_to_ps2[0x1D]=0x002C; // z Z
    kmap_usb_to_ps2[0x1E]=0x0002; // 1 !
    kmap_usb_to_ps2[0x1F]=0x0003; // 2 @
    kmap_usb_to_ps2[0x20]=0x0004; // 3 #
    kmap_usb_to_ps2[0x21]=0x0005; // 4 $
    kmap_usb_to_ps2[0x22]=0x0006; // 5 %
    kmap_usb_to_ps2[0x23]=0x0007; // 6 ^
    kmap_usb_to_ps2[0x24]=0x0008; // 7 &
    kmap_usb_to_ps2[0x25]=0x0009; // 8 *
    kmap_usb_to_ps2[0x26]=0x000A; // 9 (
    kmap_usb_to_ps2[0x27]=0x000B; // 0 )
    kmap_usb_to_ps2[0x28]=0x001C; // Return
    kmap_usb_to_ps2[0x29]=0x0001; // Escape
    kmap_usb_to_ps2[0x2A]=0x000E; // Backspace
    kmap_usb_to_ps2[0x2B]=0x000F; // Tab
    kmap_usb_to_ps2[0x2C]=0x0039; // Space
    kmap_usb_to_ps2[0x2D]=0x000C; // - _
    kmap_usb_to_ps2[0x2E]=0x000D; // = +
    kmap_usb_to_ps2[0x2F]=0x001A; // [ {
    kmap_usb_to_ps2[0x30]=0x001B; // ] }
    kmap_usb_to_ps2[0x31]=0x002B; // \ |
    kmap_usb_to_ps2[0x32]=0x002B; // Europe 1 (Note 2)
    kmap_usb_to_ps2[0x33]=0x0027; // ; :
    kmap_usb_to_ps2[0x34]=0x0028; // ' "
    kmap_usb_to_ps2[0x35]=0x0029; // ` ~
    kmap_usb_to_ps2[0x36]=0x0033; // , <
    kmap_usb_to_ps2[0x37]=0x0034; // . >
    kmap_usb_to_ps2[0x38]=0x0035; // / ?
    kmap_usb_to_ps2[0x39]=0x003A; // Caps Lock
    kmap_usb_to_ps2[0x3A]=0x003B; // F1
    kmap_usb_to_ps2[0x3B]=0x003C; // F2
    kmap_usb_to_ps2[0x3C]=0x003D; // F3
    kmap_usb_to_ps2[0x3D]=0x003E; // F4
    kmap_usb_to_ps2[0x3E]=0x003F; // F5
    kmap_usb_to_ps2[0x3F]=0x0040; // F6
    kmap_usb_to_ps2[0x40]=0x0041; // F7
    kmap_usb_to_ps2[0x41]=0x0042; // F8
    kmap_usb_to_ps2[0x42]=0x0043; // F9
    kmap_usb_to_ps2[0x43]=0x0044; // F10
    kmap_usb_to_ps2[0x44]=0x0057; // F11
    kmap_usb_to_ps2[0x45]=0x0058; // F12
    kmap_usb_to_ps2[0x46]=0xE037; // Print Screen (Note 1)
    kmap_usb_to_ps2[0x47]=0x0046; // Scroll Lock
    kmap_usb_to_ps2[0x49]=0xE052; // Insert (Note 1)
    kmap_usb_to_ps2[0x4A]=0xE047; // Home (Note 1)
    kmap_usb_to_ps2[0x4B]=0xE049; // Page Up (Note 1)
    kmap_usb_to_ps2[0x4C]=0xE053; // Delete (Note 1)
    kmap_usb_to_ps2[0x4D]=0xE04F; // End (Note 1)
    kmap_usb_to_ps2[0x4E]=0xE051; // Page Down (Note 1)
    kmap_usb_to_ps2[0x4F]=0xE04D; // Right Arrow (Note 1)
    kmap_usb_to_ps2[0x50]=0xE04B; // Left Arrow (Note 1)
    kmap_usb_to_ps2[0x51]=0xE050; // Down Arrow (Note 1)
    kmap_usb_to_ps2[0x52]=0xE048; // Up Arrow (Note 1)
    kmap_usb_to_ps2[0x53]=0x0045; // Num Lock
    kmap_usb_to_ps2[0x54]=0xE035; // Keypad / (Note 1)
    kmap_usb_to_ps2[0x55]=0x0037; // Keypad *
    kmap_usb_to_ps2[0x56]=0x004A; // Keypad -
    kmap_usb_to_ps2[0x57]=0x004E; // Keypad +
    kmap_usb_to_ps2[0x58]=0xE01C; // Keypad Enter
    kmap_usb_to_ps2[0x59]=0x004F; // Keypad 1 End
    kmap_usb_to_ps2[0x5A]=0x0050; // Keypad 2 Down
    kmap_usb_to_ps2[0x5B]=0x0051; // Keypad 3 PageDn
    kmap_usb_to_ps2[0x5C]=0x004B; // Keypad 4 Left
    kmap_usb_to_ps2[0x5D]=0x004C; // Keypad 5
    kmap_usb_to_ps2[0x5E]=0x004D; // Keypad 6 Right
    kmap_usb_to_ps2[0x5F]=0x0047; // Keypad 7 Home
    kmap_usb_to_ps2[0x60]=0x0048; // Keypad 8 Up
    kmap_usb_to_ps2[0x61]=0x0049; // Keypad 9 PageUp
    kmap_usb_to_ps2[0x62]=0x0052; // Keypad 0 Insert
    kmap_usb_to_ps2[0x63]=0x0053; // Keypad . Delete
    kmap_usb_to_ps2[0x64]=0x0056; // Europe 2 (Note 2)
    kmap_usb_to_ps2[0x65]=0xE05D; // App
    kmap_usb_to_ps2[0x67]=0x0059; // Keypad =
    kmap_usb_to_ps2[0x68]=0x005D; // F13
    kmap_usb_to_ps2[0x69]=0x005E; // F14
    kmap_usb_to_ps2[0x6A]=0x005F; // F15
    kmap_usb_to_ps2[0x85]=0x007E; // Keypad , (Brazilian Keypad .)
    kmap_usb_to_ps2[0x87]=0x0073; // Keyboard Int'l 1 ろ (Ro)
    kmap_usb_to_ps2[0x88]=0x0070; // Keyboard Int'l 2 かたかな ひらがな ローマ字 (Katakana/Hiragana)
    kmap_usb_to_ps2[0x89]=0x007D; // Keyboard Int'l 3 ￥ (Yen)
    kmap_usb_to_ps2[0x8A]=0x0079; // Keyboard Int'l 4 前候補 変換 (次候補) 全候補 (Henkan)
    kmap_usb_to_ps2[0x8B]=0x007B; // Keyboard Int'l 5 無変換 (Muhenkan)
    kmap_usb_to_ps2[0x8C]=0x005C; // Keyboard Int'l 6 (PC9800 Keypad , )
    kmap_usb_to_ps2[0x90]=0x00F2; // Keyboard Lang 1 한/영 (Hanguel/English)
    kmap_usb_to_ps2[0x91]=0x00F1; // Keyboard Lang 2 한자 (Hanja)
    kmap_usb_to_ps2[0x92]=0x0078; // Keyboard Lang 3 かたかな (Katakana)
    kmap_usb_to_ps2[0x93]=0x0077; // Keyboard Lang 4 ひらがな (Hiragana)
    kmap_usb_to_ps2[0x94]=0x0076; // Keyboard Lang 5 半角/全角 (Zenkaku/Hankaku)
    kmap_usb_to_ps2[0xE0]=0x001D; // Left Control
    kmap_usb_to_ps2[0xE1]=0x002A; // Left Shift
    kmap_usb_to_ps2[0xE2]=0x0038; // Left Alt
    kmap_usb_to_ps2[0xE3]=0xE05B; // Left GUI
    kmap_usb_to_ps2[0xE4]=0xE01D; // Right Control
    kmap_usb_to_ps2[0xE5]=0x0036; // Right Shift
    kmap_usb_to_ps2[0xE6]=0xE038; // Right Alt
    kmap_usb_to_ps2[0xE7]=0xE05C; // Right GUI

    Pause((char *)"post-init");

    return r;
}

CStdlibApp::TShutdownMode CKernel::Run (void)
{
    mLogger.Write (GetKernelName(), LogNotice, "Circle-Halfix");
    mLogger.Write (GetKernelName(), LogNotice, "Compile time: " __DATE__ " " __TIME__);

    Pause((char *)"do-run");

    char *argv[] = { (char *)"halfix" };
    int retval = main_halfix_unix(1, argv);
    // int retval = 0;

    // if everything goes well, we'll never reach this point
    mLogger.Write (GetKernelName(), LogNotice, "Shutting down because of %d...", retval);
    return ShutdownHalt;
}

void CKernel::UpdateKeyboardAndMouse()
{
    boolean bUpdated = mUSBHCI.UpdatePlugAndPlay ();
    m_pKeyboard = (CUSBKeyboardDevice *) mDeviceNameService.GetDevice ("ukbd1", FALSE);
    m_pMouse = (CMouseDevice *) mDeviceNameService.GetDevice ("mouse1", FALSE);

    this_kernel->mLogger.Write (this_kernel->GetKernelName(), LogNotice, "Update K&M: %d k %08X %08X m %08X %08X", bUpdated, m_pKeyboard, m_pre_pKeyboard, m_pMouse, m_pre_pMouse);

    if (m_pKeyboard != m_pre_pKeyboard)
    {
        if (m_pKeyboard != 0)
        {
            m_pKeyboard->RegisterRemovedHandler (KeyboardRemovedHandler);

            m_pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
        }

        if (m_pKeyboard != 0)
        {
            // CUSBKeyboardDevice::UpdateLEDs() must not be called in interrupt context,
            // that's why this must be done here. This does nothing in raw mode.
            m_pKeyboard->UpdateLEDs ();
        }
    }

    if (m_pMouse != m_pre_pMouse)
    {
        if (m_pMouse != 0)
        {
            m_pMouse->RegisterRemovedHandler (MouseRemovedHandler);

            mLogger.Write (GetKernelName(), LogNotice, "USB mouse has %d buttons and %s wheel", m_pMouse->GetButtonCount(), m_pMouse->HasWheel() ? "a" : "no");

            m_pMouse->Release();
            if (!m_pMouse->Setup (virtual_screen_width, virtual_screen_height))
            {
                mLogger.Write (GetKernelName(), LogPanic, "Cannot setup mouse");
            }

            m_pMouse->SetCursor (0, 0);
            m_pMouse->ShowCursor (TRUE);

            m_pMouse->RegisterEventHandler (MouseEventStub);
        }

        if (m_pMouse != 0)
        {
            m_pMouse->UpdateCursor ();
        }
    }

    m_pre_pMouse = m_pMouse;
    m_pre_pKeyboard = m_pKeyboard;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
        assert (this_kernel != 0);

        unsigned int sdlmods = 0;
        if (ucModifiers & LCTRL) sdlmods += KMOD_LCTRL;
        if (ucModifiers & LSHIFT) sdlmods += KMOD_LSHIFT;
        if (ucModifiers & ALT) sdlmods += KMOD_LALT;
        if (ucModifiers & LWIN) sdlmods += KMOD_LMETA;
        if (ucModifiers & RCTRL) sdlmods += KMOD_RCTRL;
        if (ucModifiers & RSHIFT) sdlmods += KMOD_RSHIFT;
        if (ucModifiers & ALTGR) sdlmods += KMOD_RALT;
        if (ucModifiers & RWIN) sdlmods += KMOD_RMETA;

        static_event.key_keysym_mod = sdlmods;

        u16 k = 0;

        // only one of the six overlapping keys
        for (unsigned i = 0; i < 1; i++)
        {
            if (RawKeys[i] != 0)
            {
                // RawKeys are USB scancodes, we need to forward PS2 scancodes
                // static_event.key_keysym_sym = RawKeys[i];

                k = this_kernel->kmap_usb_to_ps2[RawKeys[i]];
                static_event.key_keysym_sym = k;

                static_event.type = SDL_KEYDOWN;
            }
            else
            {
                static_event.type = SDL_KEYUP;
            }
        }
/*
        CString Message;
        Message.Format ("Key status (modifiers %02X, k %04X)", (unsigned) ucModifiers, k);

        for (unsigned i = 0; i < 6; i++)
        {
            if (RawKeys[i] != 0)
            {
                CString KeyCode;
                KeyCode.Format (" %02X", (unsigned) RawKeys[i]);
                Message.Append (KeyCode);
            }
        }

        this_kernel->mLogger.Write (this_kernel->GetKernelName(), LogNotice, Message);
*/
}

void CKernel::KeyboardRemovedHandler (CDevice *pDevice, void *pContext)
{
    assert (this_kernel != 0);
    this_kernel->mLogger.Write (this_kernel->GetKernelName(), LogNotice, "Keyboard removed");

    this_kernel->m_pKeyboard = 0;
}

void CKernel::MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove)
{
    assert (this_kernel != 0);
    this_kernel->MouseEventHandler (Event, nButtons, nPosX, nPosY, nWheelMove);
}

void CKernel::MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove)
{
    switch (Event)
    {
        case MouseEventMouseMove:
                static_mouse_event.type = SDL_MOUSEMOTION;

                // nPos are absolute positions, we need them relative.
#ifdef TRUE_RASPI_4
                static_mouse_event.motion_xrel = nPosX;
                static_mouse_event.motion_yrel = nPosY;
#else
                static_mouse_event.motion_xrel = lastmousex - nPosX;
                static_mouse_event.motion_yrel = lastmousey - nPosY;
#endif

                lastmousex = nPosX;
                lastmousey = nPosY;

                break;

        case MouseEventMouseDown:
                static_mouse_event.type = SDL_MOUSEBUTTONDOWN;
                static_mouse_event.button.button = 0;
                if (nButtons & MOUSE_BUTTON_LEFT)   static_mouse_event.button.button = SDL_BUTTON_LEFT;
                if (nButtons & MOUSE_BUTTON_MIDDLE) static_mouse_event.button.button = SDL_BUTTON_MIDDLE;
                if (nButtons & MOUSE_BUTTON_RIGHT)  static_mouse_event.button.button = SDL_BUTTON_RIGHT;
                break;

        case MouseEventMouseUp:
                static_mouse_event.type = SDL_MOUSEBUTTONUP;
                static_mouse_event.button.button = 0;
                if (nButtons & MOUSE_BUTTON_LEFT)   static_mouse_event.button.button = SDL_BUTTON_LEFT;
                if (nButtons & MOUSE_BUTTON_MIDDLE) static_mouse_event.button.button = SDL_BUTTON_MIDDLE;
                if (nButtons & MOUSE_BUTTON_RIGHT)  static_mouse_event.button.button = SDL_BUTTON_RIGHT;
                break;

        case MouseEventMouseWheel:
                // Wheel didn't exist
                break;

        default:
                break;
    }
}

void CKernel::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
    assert (this_kernel != 0);
    this_kernel->mLogger.Write (this_kernel->GetKernelName(), LogNotice, "Mouse removed");

    this_kernel->m_pMouse = 0;
}

