//
// kernel.h
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
#ifndef _kernel_h
#define _kernel_h

#include <string>
#include <circle_stdlib_app.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/2dgraphics.h>
#include <circle/types.h>
#include <circle/input/mouse.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/multicore.h>
#include <circle/memory.h>
#include "oscillator.h"

/**
 * Multicore App
 */
class CStdlibAppMultiCore: public CMultiCoreSupport
{
public:
        CStdlibAppMultiCore (CMemorySystem *pMemorySystem): CMultiCoreSupport (pMemorySystem)
        {
        }

        void Run (unsigned nCore);
};

class CKernel : public CStdlibAppStdio
{
public:
    enum TTextAlign
    {
            AlignLeft,
            AlignRight,
            AlignCenter
    };

    CKernel (void);
    virtual bool Initialize(void);
    void *HighMem_Alloc(long size);

    int64_t fileGetSize(char *fname);

    void MsPause(int ms);
    void StartTimer();
    unsigned CheckTimer();
    unsigned CheckTimerMs();

    unsigned short kmap_usb_to_ps2[256];

    void DrawColorRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color,
                        unsigned nTargetWidth, unsigned nTargetHeight, TScreenColor *targetPixelBuffer);
    void DrawColorRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color);
    void DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight,
                        unsigned nSourceX, unsigned nSourceY, TScreenColor *sourcePixelBuffer,
                        unsigned nTargetWidth, unsigned nTargetHeight, TScreenColor *targetPixelBuffer);
    void DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight,
                        unsigned nSourceX, unsigned nSourceY, TScreenColor *sourcePixelBuffer);
    void DrawText (unsigned nX, unsigned nY, TScreenColor Color, const char *pText, TTextAlign Align,
                   unsigned nTargetWidth, unsigned nTargetHeight, TScreenColor *targetPixelBuffer);
    void DrawText (unsigned nX, unsigned nY, TScreenColor Color, const char *pText, TTextAlign Align);

    void wrapClearScreen(TScreenColor color);
    void wrapDrawImage(unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor *sourcePixelBuffer);
    void wrapResize(unsigned nWidth, unsigned nHeight);

    void ConfigureMouse(boolean init, unsigned nScreenWidth, unsigned nScreenHeight);

    CUSBKeyboardDevice * volatile m_pre_pKeyboard;
    CUSBKeyboardDevice * volatile m_pKeyboard;

    CMouseDevice * volatile m_pre_pMouse;
    CMouseDevice * volatile m_pMouse;

    CLogger * p_mLogger;
    CCharGenerator m_Font;

    CSoundBaseDevice        *m_pSound;

    COscillator m_LFO;
    COscillator m_VFO;

    boolean bRunningMulticore;
    CStdlibAppMultiCore mStdlibAppMultiCore;

    void UpdateKeyboardAndMouse();
    static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);
    static void KeyboardRemovedHandler (CDevice *pDevice, void *pContext);

    void MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove);
    static void MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove);
    static void MouseRemovedHandler (CDevice *pDevice, void *pContext);

    TShutdownMode Run (void);
};

#endif
