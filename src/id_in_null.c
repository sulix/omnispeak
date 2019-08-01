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

#include <string.h>

void IN_NULL_PumpEvents()
{
}

void IN_NULL_WaitKey()
{
}

void IN_NULL_Startup(bool disableJoysticks)
{
}

bool IN_NULL_StartJoy(int joystick)
{
	// No Joysticks present
	return false;
}

void IN_NULL_StopJoy(int joystick)
{
}

bool IN_NULL_JoyPresent(int joystick)
{
	return false;
}

void IN_NULL_JoyGetAbs(int joystick, int *x, int *y)
{
	if (x)
		*x = 0;
	if (y)
		*y = 0;
}

uint16_t IN_NULL_JoyGetButtons(int joystick)
{
	bool button0 = false;
	bool button1 = false;
	return (button0) | (button1 << 1);
}

const char* IN_NULL_JoyGetName(int joystick)
{
	return NULL;
}

IN_Backend in_null_backend = {
	.startup = IN_NULL_Startup,
	.shutdown = 0,
	.pumpEvents = IN_NULL_PumpEvents,
	.waitKey = IN_NULL_WaitKey,
	.joyStart = IN_NULL_StartJoy,
	.joyStop = IN_NULL_StopJoy,
	.joyPresent = IN_NULL_JoyPresent,
	.joyGetAbs = IN_NULL_JoyGetAbs,
	.joyGetButtons = IN_NULL_JoyGetButtons,
	.joyGetName = IN_NULL_JoyGetName,
	.joyAxisMin = -1000,
	.joyAxisMax =  1000,
};

IN_Backend *IN_Impl_GetBackend()
{
	return &in_null_backend;
}
