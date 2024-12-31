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


class CKernel : public CStdlibAppStdio
{
public:
    CKernel (void);
    virtual bool Initialize(void);

    void Pause(char *m);
    void MsPause(int ms);

    C2DGraphics *screen2d;
    unsigned short kmap_usb_to_ps2[256];

    CUSBKeyboardDevice * volatile m_pre_pKeyboard;
    CUSBKeyboardDevice * volatile m_pKeyboard;

    CMouseDevice * volatile m_pre_pMouse;
    CMouseDevice * volatile m_pMouse;

    CLogger * p_mLogger;

    void UpdateKeyboardAndMouse();
    static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);
    static void KeyboardRemovedHandler (CDevice *pDevice, void *pContext);

    void MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove);
    static void MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove);
    static void MouseRemovedHandler (CDevice *pDevice, void *pContext);

    TShutdownMode Run (void);

private:

};

#endif
