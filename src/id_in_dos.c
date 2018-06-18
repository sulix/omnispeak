/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2018 Omnispeak Authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "id_in.h"

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <string.h>

static void INL_KeyService()
{
	static bool isSpecial = false;
	bool interruptsWereEnabled = disable();
	uint8_t scanCode = inportb(0x60);

	// Clear the key in the XT keyboard controller
	uint8_t temp = inportb(0x61);
	outportb(0x61, temp | 0x80);
	outportb(0x61, temp);

	if (scanCode == 0xE0)
	{
		// We'll get another interrupt with the actual scancode, so
		// remember that it's a special scancode for later.
		isSpecial = true;
	}
	else if (scanCode & 0x80)
	{
		// Key is released.
		IN_HandleKeyUp(scanCode & 0x7F, isSpecial);
		isSpecial = false;
	}
	else
	{
		// Key is pressed.
		IN_HandleKeyDown(scanCode, isSpecial);
		isSpecial = false;
	}
	// TODO: Support falling through to the system interrupt handler.

	// Reset the interrupt.
	outportb(0x20, 0x20);
	if (interruptsWereEnabled)
		enable();
}

void IN_DOS_PumpEvents()
{
	// Nothing, as this is all done in the background with interrupts (yay!)
}

void IN_DOS_WaitKey()
{
}

_go32_dpmi_seginfo in_dos_oldISR, in_dos_newISR;

void IN_DOS_Shutdown(void)
{
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &in_dos_oldISR);
	_go32_dpmi_free_iret_wrapper(&in_dos_newISR);
}

void IN_DOS_Startup(bool disableJoysticks)
{
	in_dos_newISR.pm_offset = (intptr_t)&INL_KeyService;
	in_dos_newISR.pm_selector = _go32_my_cs();

	_go32_dpmi_get_protected_mode_interrupt_vector(9, &in_dos_oldISR);
	_go32_dpmi_allocate_iret_wrapper(&in_dos_newISR);
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &in_dos_newISR);
}

bool IN_DOS_StartJoy(int joystick)
{
	// No Joysticks present
	return false;
}

void IN_DOS_StopJoy(int joystick)
{
}

bool IN_DOS_JoyPresent(int joystick)
{
	return false;
}

void IN_DOS_JoyGetAbs(int joystick, int *x, int *y)
{
	if (x)
		*x = 0;
	if (y)
		*y = 0;
}

uint16_t IN_DOS_JoyGetButtons(int joystick)
{
	bool button0 = false;
	bool button1 = false;
	return (button0) | (button1 << 1);
}

IN_Backend in_null_backend = {
	.startup = IN_DOS_Startup,
	.shutdown = 0,
	.pumpEvents = IN_DOS_PumpEvents,
	.waitKey = IN_DOS_WaitKey,
	.joyStart = IN_DOS_StartJoy,
	.joyStop = IN_DOS_StopJoy,
	.joyPresent = IN_DOS_JoyPresent,
	.joyGetAbs = IN_DOS_JoyGetAbs,
	.joyGetButtons = IN_DOS_JoyGetButtons,
};

IN_Backend *IN_Impl_GetBackend()
{
	return &in_null_backend;
}
