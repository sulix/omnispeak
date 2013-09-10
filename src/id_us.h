/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2012 David Gow <david@ingeniumdigital.com>

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

#ifndef ID_US_H
#define ID_US_H

#include <stdbool.h>

/* This keeps clang's static analyzer quiet. */
#ifdef __GNUC__
#define _NORETURN __attribute__((noreturn))
#else
#define _NORETURN
#endif

// In ck_quit.c, as it may be customized by individual games.
void Quit(const char *msg) _NORETURN;

// id_us_1.c:
// Parameter Checking
int US_CheckParm(const char *parm, char **strings);
// UI functions
void US_Print(const char *str);
void US_PrintF(const char *str, ...);
void US_PrintCentered(const char *str);
void US_CPrintLine(const char *str);
void US_CPrint(const char *str);
void US_CPrintF(const char *str, ...);
void US_ClearWindow();
void US_DrawWindow(int x, int y, int w, int h);
void US_CenterWindow(int w, int h);
// Random Number Generation
void US_InitRndT(bool randomize);
int US_RndT();
void US_SetRndI(int index);
int US_GetRndI();

// Common
extern bool us_noWait; // Debug mode enabled.
extern bool us_tedLevel; // Launching a level from TED
extern int us_tedLevelNumber; // Number of level to launch from TED

// We need to steal these from main().
extern char **us_argv;
extern int us_argc;

void US_Startup();

#endif //ID_US_H
