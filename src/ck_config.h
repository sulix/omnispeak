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
// Episode Selection
// =================================

// We always need at least one episode enabled.
#if !defined(WITH_KEEN4) && !defined(WITH_KEEN5) && !defined(WITH_KEEN6)
#error Need to enable at least one episode!
#endif


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
// Episode-specific config options
// =================================
#if defined(WITH_KEEN4) || defined(WITH_KEEN5)
#define HAS_HELPSCREEN
#endif

// =================================
// Options for DEBUG builds only.
// =================================
#ifdef CK_DEBUG
// Enable all debug messages by default.
#define CK_DEFAULT_LOG_LEVEL CK_LOG_MSG_NORMAL
// Debug version of the ID_MM_Arena allocator.
#define ID_MM_DEBUGARENA
// Typechecking for variables.
#ifndef CK_VAR_TYPECHECK // Might already be set for varparser
#define CK_VAR_TYPECHECK
#endif
// Warn on use of unset variable
#define CK_VAR_WARNONNOTSET
// Warn on out-of-bounds access to tileinfo.
#define CK_WARN_ON_TILEINFO_OOB
// Support the /DUMPFILE option for the playloop dumper
#define CK_ENABLE_PLAYLOOP_DUMPER
#else
// Release builds only show warnings and higher
#define CK_DEFAULT_LOG_LEVEL CK_LOG_MSG_WARNING
#endif

// =================================
// Options for VANILLA builds only.
// =================================
#ifndef CK_VANILLA

// Define a boolean for new features
#define CK_NEW_FEATURE_DEFAULT false

// The Fullscreen/Aspect Ratio/etc options
#define EXTRA_GRAPHICS_OPTIONS

// Support for rebinding 'Status'
#define EXTRA_KEYBOARD_OPTIONS

// Joystick names, Joystick Configuration
#define EXTRA_JOYSTICK_OPTIONS

// QuickSave
#define QUICKSAVE_ENABLED

// Use all input devices by default
#define DEFAULT_INPUT IN_ctrl_All
#define FORCE_DEFAULT_INPUT
#else

// Otherwise, enable default new features
#define CK_NEW_FEATURE_DEFAULT false
#endif

// =================================
// Filesystem (ID_FS) options.
// =================================

// Default paths for Omni / Keen / User files.
// (See the id_fs.h documentation for more info.)
#ifndef FS_DEFAULT_KEEN_PATH
#define FS_DEFAULT_KEEN_PATH "."
#endif
#ifndef FS_DEFAULT_OMNI_PATH
#define FS_DEFAULT_OMNI_PATH FS_DEFAULT_KEEN_PATH
#endif
#ifndef FS_DEFAULT_USER_PATH
#define FS_DEFAULT_USER_PATH "."
#endif

// Look for Omnispeak-specific files in the Keen directory first
#ifndef FS_NO_PREFER_KEEN_PATH
#define FS_PREFER_KEEN_PATH
#endif

// If the Omni path isn't valid, fall back to the directory
// containing the executable. SDL 2.0.1 or higher required.
#if !defined(FS_NO_OMNI_EXEDIR_FALLBACK) && defined(WITH_SDL)
#define FS_OMNI_EXEDIR_FALLBACK
#endif

// Enable this to prefer XDG paths for the user directory to the "DEFAULT"
// path above. This is disabled by default, in which case XDG paths are
// used as a fallback. SDL 2.0.1 is required.
//#define FS_USER_PATH_PREFER_XDG

// Fallback to the XDG paths if the current user path is not writable.
#ifdef WITH_SDL
#define FS_USER_XDG_FALLBACK
#endif

#define FS_XDG_ORGANISATION "Commander Keen"
#define FS_XDG_APPLICATION "Omnispeak"

// =================================
// Cache Manager (ID_CA) options.
// =================================

// Read uncompressed audio data (AUDIOT, AUDIOHED)
//#define CA_AUDIOUNCOMPRESSED

#endif //CK_CONFIG_H
