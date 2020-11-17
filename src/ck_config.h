/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2019 Omnispeak Authors

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

#ifndef CK_CONFIG_H
#define CK_CONFIG_H

// =================================
// Always-enabled config options
// =================================

// We now support dirty-rectangle drawing, so there's no need to always
// redraw the whole screen on each frame. You can enable this, though, if you
// prefer (e.g., to have something to compare the dirty-rectangle drawing
// against.)
// (NOTE: You'll need to enable this on double-buffered backends for accuracy
// at the moment, as VL_SyncPages() isn't implemented. The performance hit on
// DOS isn't really worth it, though.)
//#define ALWAYS_REDRAW

// =================================
// Options for DEBUG builds only.
// =================================
#ifdef CK_DEBUG
// Debug version of the ID_MM_Arena allocator.
#define ID_MM_DEBUGARENA
// Warn on out-of-bounds access to tileinfo.
#define CK_WARN_ON_TILEINFO_OOB
#endif

// =================================
// Options for VANILLA builds only.
// =================================
#ifndef CK_VANILLA
// The Fullscreen/Aspect Ratio/etc options
#define EXTRA_GRAPHICS_OPTIONS

// Support for rebinding 'Status'
#define EXTRA_KEYBOARD_OPTIONS

// Joystick names, Joystick Configuration
#define EXTRA_JOYSTICK_OPTIONS

// QuickSave
#define QUICKSAVE_ENABLED
#endif

#endif //CK_CONFIG_H
