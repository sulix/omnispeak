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

// A private header with contents used only by the VL code

#ifndef ID_VL_PRIVATE_H
#define ID_VL_PRIVATE_H

#include <stdint.h>

#define VL_EGAVGA_GFX_WIDTH 320
#define VL_EGAVGA_GFX_HEIGHT 200
/* Graphics mode 0xD is scand-doubled on the VGA, so effectively a total of
 * 320x400 pixels consist of the displayed area. According to experimental
 * observations, the vertical overscan bars have the thickness of 8 pixels,
 * while each of the horizontal bars has 7 scanlines (after double-scanning).
 */
#define VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH (vl_hasOverscanBorder ? 8 : 0)
#define VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH (vl_hasOverscanBorder ? 8 : 0)
#define VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT (vl_hasOverscanBorder ? 7 : 0)
#define VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT (vl_hasOverscanBorder ? 7 : 0)
#define VL_VGA_GFX_WIDTH_SCALEFACTOR 1
#define VL_VGA_GFX_HEIGHT_SCALEFACTOR 2
/* If 320x200 is desired, then the bars should be
 * shrunken separately if symmetry is desired.
 */
#define VL_VGA_GFX_SHRUNK_LEFTBORDER_WIDTH (VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH/VL_VGA_GFX_WIDTH_SCALEFACTOR)
#define VL_VGA_GFX_SHRUNK_RIGHTBORDER_WIDTH (VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH/VL_VGA_GFX_WIDTH_SCALEFACTOR)
#define VL_VGA_GFX_SHRUNK_TOPBORDER_HEIGHT (VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT/VL_VGA_GFX_HEIGHT_SCALEFACTOR)
#define VL_VGA_GFX_SHRUNK_BOTTOMBORDER_HEIGHT (VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT/VL_VGA_GFX_HEIGHT_SCALEFACTOR)

#define VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER (VL_VGA_GFX_WIDTH_SCALEFACTOR*VL_EGAVGA_GFX_WIDTH+VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH+VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH)
#define VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER (VL_VGA_GFX_HEIGHT_SCALEFACTOR*VL_EGAVGA_GFX_HEIGHT+VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT+VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT)
#define VL_VGA_GFX_SHRUNK_WIDTH_PLUS_BORDER (VL_EGAVGA_GFX_WIDTH+VL_VGA_GFX_SHRUNK_LEFTBORDER_WIDTH+VL_VGA_GFX_SHRUNK_RIGHTBORDER_WIDTH)
#define VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER (VL_EGAVGA_GFX_HEIGHT+VL_VGA_GFX_SHRUNK_TOPBORDER_HEIGHT+VL_VGA_GFX_SHRUNK_BOTTOMBORDER_HEIGHT)


// Here is how the dimensions of the window are currently picked:
// 1. The emulated 320x200 sub-window is first zoomed
// by a factor of 3 (for each dimension) to 960x600.
// 2. The height is then multiplied by 1.2, so the internal contents
// (without the borders) have the aspect ratio of 4:3.
//
// There are a few more tricks in use to handle the overscan border
// and VGA line doubling.
#define VL_DEFAULT_WINDOW_WIDTH (VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER*3/VL_VGA_GFX_WIDTH_SCALEFACTOR)
#define VL_DEFAULT_WINDOW_HEIGHT (6*VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER*3/(5*VL_VGA_GFX_HEIGHT_SCALEFACTOR))

#define VL_WINDOW_TITLE "Omnispeak"

// EGA color table in RGB format (technically more can be chosen with the VGA)
extern const uint8_t VL_EGARGBColorTable[16][3];

// TODO: Consider adding more fields (like panning) and having multiple
// structs (emulated EGA/VGA adapter struct, SDL-specific struct, etc).
typedef struct VL_EGAVGAAdapter {
	uint8_t bordercolor;
	uint8_t palette[16];
	void *screen;
} VL_EGAVGAAdapter;

extern VL_EGAVGAAdapter vl_emuegavgaadapter;

extern bool vl_isFullScreen;
extern bool vl_isAspectCorrected;
extern bool vl_hasOverscanBorder;

// The full on-screen region, including overscan border.
extern int vl_fullRgn_x;
extern int vl_fullRgn_y;
extern int vl_fullRgn_w;
extern int vl_fullRgn_h;

// The region of fullRgn, excluding overscan border.
extern int vl_renderRgn_x;
extern int vl_renderRgn_y;
extern int vl_renderRgn_w;
extern int vl_renderRgn_h;

// The integer scaled render size.
extern int vl_integerWidth;
extern int vl_integerHeight;

// Calculates render regions taking the integer scaling into account.
void VL_CalculateRenderRegions(int realW, int realH);

#ifndef max
#define max(a,b) (((a) < (b))?(b):(a))
#endif
#ifndef min
#define min(a,b) (((a) < (b))?(a):(b))
#endif
#endif

