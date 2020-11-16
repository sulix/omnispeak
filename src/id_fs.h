/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2020 Omnispeak Authors

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

// This is the "Filesystem Manager", which handles all file I/O, and determines
// which paths are used for which data files.

#include <stdio.h>
#include <stdbool.h>

typedef FILE *FS_File;

bool FS_IsFileValid(FS_File file);

size_t FS_GetFileSize(FS_File file);
size_t FS_Read(void *ptr, size_t size, size_t nmemb, FS_File file);
size_t FS_Write(const void *ptr, size_t size, size_t nmemb, FS_File file);
size_t FS_SeekTo(FS_File file, size_t offset);
void FS_CloseFile(FS_File file);

// Omnispeak has three search paths, for three different kinds of data files:
// - Keen Data (The original data files shipped with Keen)
// - Omnispeak Data (The datafiles shipped with omnispeak: headers, huffman dictionaries, and scripts)
// - User Data (Configs, Savegames, etc)
//
// These search paths may all point to the same directory, or they may
// each be independent. The defaults can be configured at runtime.

FS_File FS_OpenKeenFile(const char *fileName);
FS_File FS_OpenOmniFile(const char *fileName);
FS_File FS_OpenUserFile(const char *fileName);

FS_File FS_CreateUserFile(const char *fileName);

char *FS_AdjustExtension(const char *filename);

bool FS_IsKeenFilePresent(const char *filename);
bool FS_IsOmniFilePresent(const char *filename);
bool FS_IsUserFilePresent(const char *filename);

void FS_Startup();

// Used for reading buffers of a specific type, assuming Little-Endian
// byte order in the file's data itself. It gets converted to native order.
size_t FS_ReadInt8LE(void *ptr, size_t count, FS_File stream);
size_t FS_ReadInt16LE(void *ptr, size_t count, FS_File stream);
size_t FS_ReadInt32LE(void *ptr, size_t count, FS_File stream);
// Used for writing buffers of a specific type, converting
// native byte order to Little-Endian order within the file.
size_t FS_WriteInt8LE(const void *ptr, size_t count, FS_File stream);
size_t FS_WriteInt16LE(const void *ptr, size_t count, FS_File stream);
size_t FS_WriteInt32LE(const void *ptr, size_t count, FS_File stream);
// Similar methods for reading/writing bools from/to int16_t
// (0 as false, 1 as true and any nonzero as true for reading.)
// TODO: Maybe int16_t's should be used internally? (Same as vanilla Keen.)
size_t FS_ReadBoolFrom16LE(void *ptr, size_t count, FS_File stream);
size_t FS_WriteBoolTo16LE(const void *ptr, size_t count, FS_File stream);
