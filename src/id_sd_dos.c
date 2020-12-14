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

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "id_sd.h"
#include "ck_cross.h"

#define PC_PIT_RATE 1193182
#define SD_SFX_PART_RATE 140
/* In the original exe, upon setting a rate of 140Hz or 560Hz for some
 * interrupt handler, the value 1192030 divided by the desired rate is
 * calculated, to be programmed for timer 0 as a consequence.
 * For THIS value, it is rather 1193182 that should be divided by it, in order
 * to obtain a better approximation of the actual rate.
 */
#define SD_SOUND_PART_RATE_BASE 1192030

#define SD_ADLIB_REG_PORT 0x388
#define SD_ADLIB_DATA_PORT 0x389

static int16_t ScaledTimerDivisor = 1;

void SD_DOS_SetTimer0(int16_t int_8_divisor)
{
	//Configure the PIT frequency.
	outportb(0x43, 0x36);
	outportb(0x40, int_8_divisor & 0xFF);
	outportb(0x40, int_8_divisor >> 8);
	ScaledTimerDivisor = ((int32_t)SD_SOUND_PART_RATE_BASE / (int32_t)int_8_divisor) & 0xFFFF;
}

void SD_DOS_alOut(uint8_t reg, uint8_t val)
{
	int wereIntsEnabled = disable();

	outportb(SD_ADLIB_REG_PORT, reg);

	// Delay 6 'in's to give the OPL2 some processing time.
	for (int timer = 6; timer; --timer)
		(void)inportb(SD_ADLIB_REG_PORT);

	outportb(SD_ADLIB_DATA_PORT, val);

	// And wait a lot more after sending the data.
	for (int timer = 35; timer; --timer)
		(void)inportb(SD_ADLIB_REG_PORT);

	if (wereIntsEnabled)
		enable();
}

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_t0Service(void);

void SD_DOS_t0InterruptProxy()
{
	SDL_t0Service();
	outportb(0x20, 0x20);
}

void SD_DOS_PCSpkOn(bool on, int freq)
{
	if (on)
	{
		outportb(0x43, 0xb6);

		outportb(0x42, freq & 0xFF);
		outportb(0x42, freq >> 8);

		uint8_t oldPcState = inportb(0x61);
		outportb(0x61, oldPcState | 3);
	}
	else
	{
		uint8_t oldPcState = inportb(0x61);
		outportb(0x61, oldPcState & 0xFC);
	}
}

_go32_dpmi_seginfo sd_dos_oldISR, sd_dos_newISR;

void SD_DOS_Startup(void)
{
	sd_dos_newISR.pm_offset = (intptr_t)&SD_DOS_t0InterruptProxy;
	sd_dos_newISR.pm_selector = _go32_my_cs();

	_go32_dpmi_get_protected_mode_interrupt_vector(8, &sd_dos_oldISR);
	_go32_dpmi_allocate_iret_wrapper(&sd_dos_newISR);
	_go32_dpmi_set_protected_mode_interrupt_vector(8, &sd_dos_newISR);
}

void SD_DOS_Shutdown(void)
{
	_go32_dpmi_set_protected_mode_interrupt_vector(8, &sd_dos_oldISR);
	_go32_dpmi_free_iret_wrapper(&sd_dos_newISR);
}

bool SD_DOS_WereInterruptsEnabled = false;

void SD_DOS_Lock()
{
	SD_DOS_WereInterruptsEnabled = disable();
}

void SD_DOS_Unlock()
{
	if (SD_DOS_WereInterruptsEnabled)
		enable();
}

SD_Backend sd_dos_backend = {
	.startup = SD_DOS_Startup,
	.shutdown = SD_DOS_Shutdown,
	.lock = SD_DOS_Lock,
	.unlock = SD_DOS_Unlock,
	.alOut = SD_DOS_alOut,
	.pcSpkOn = SD_DOS_PCSpkOn,
	.setTimer0 = SD_DOS_SetTimer0};

SD_Backend *SD_Impl_GetBackend()
{
	return &sd_dos_backend;
}
