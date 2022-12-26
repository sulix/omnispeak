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

// If we want to use the "linux" backend here, we need to define _GNU_SOURCE
// before any system headers are pulled in, so that O_PATH is defined.
#if defined(__linux__) && !defined(__STRICT_ANSI__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ck_cross.h"
#include "ck_ep.h"

#include "id_fs.h"
#include "id_mm.h"
#include "id_us.h"

const char *fs_keenPath;
const char *fs_omniPath;
const char *fs_userPath;

// NOTE: This will need to go into the per-platform bits below if we ever want
// to use something other than a pointer on some platforms (e.g., a faw file
// descriptor, a windows HANDLE, etc).
bool FS_IsFileValid(FS_File file)
{
	return (file != 0);
}

size_t FS_Read(void *ptr, size_t size, size_t nmemb, FS_File file)
{
	return fread(ptr, size, nmemb, file);
}

size_t FS_Write(const void *ptr, size_t size, size_t nmemb, FS_File file)
{
	return fwrite(ptr, size, nmemb, file);
}

size_t FS_SeekTo(FS_File file, size_t offset)
{
	size_t oldOff = ftell(file);
	fseek(file, offset, SEEK_SET);
	return oldOff;
}

void FS_CloseFile(FS_File file)
{
	fclose(file);
}

#if defined(__linux__) && !defined(__STRICT_ANSI__)
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

FS_File FSL_OpenFileInDirCaseInsensitive(const char *dirPath, const char *fileName, bool forWrite)
{
	int dirFd = open(dirPath, O_RDONLY | O_DIRECTORY);
	if (dirFd == -1)
		return 0;
	DIR *currentDirPtr = fdopendir(dirFd);
	if (!currentDirPtr)
	{
		close(dirFd);
		return 0;
	}

	// Search the current directory for matching names.
	for (struct dirent *dirEntry = readdir(currentDirPtr); dirEntry; dirEntry = readdir(currentDirPtr))
	{
		if (!CK_Cross_strcasecmp(dirEntry->d_name, fileName))
		{
			// We've found our file!
			if (forWrite)
			{
				int fd = openat(dirFd, dirEntry->d_name, O_WRONLY | O_TRUNC);
				closedir(currentDirPtr);
				return fdopen(fd, "wb");
			}
			else
			{
				int fd = openat(dirFd, dirEntry->d_name, O_RDONLY);
				closedir(currentDirPtr);
				return fdopen(fd, "rb");
			}
		}
	}
	closedir(currentDirPtr);
	return 0;
}

FS_File FSL_CreateFileInDir(const char *dirPath, const char *fileName)
{
	int dirFd = open(dirPath, O_PATH | O_DIRECTORY);
	if (dirFd == -1)
		return 0;

	int fd = openat(dirFd, fileName, O_CREAT | O_WRONLY, 0664);
	close(dirFd);
	if (fd == -1)
		return 0;
	return fdopen(fd, "wb");
}

bool FSL_IsDirWritable(const char *dirPath)
{
	return (access(dirPath, W_OK | X_OK) == 0);
}

size_t FS_GetFileSize(FS_File file)
{
	struct stat fileStat;
	if (fstat(fileno(file), &fileStat))
		return 0;

	return fileStat.st_size;
}

#elif _WIN32
#define WIN32_MEAN_AND_LEAN
#undef UNICODE
#include <io.h>
#include <windows.h>
FS_File FSL_OpenFileInDirCaseInsensitive(const char *dirPath, const char *fileName, bool forWrite)
{
	// TODO: We really should scan through the path on windows anyway, as that'll
	// make sure there isn't any nasty path manipulation shenanigans going on.
	char fullFileName[MAX_PATH];
	sprintf(fullFileName, "%s\\%s", dirPath, fileName);

	return fopen(fullFileName, forWrite ? "wb" : "rb");
}

FS_File FSL_CreateFileInDir(const char *dirPath, const char *fileName)
{
	// TODO: We really should scan through the path on windows anyway, as that'll
	// make sure there isn't any nasty path manipulation shenanigans going on.
	char fullFileName[MAX_PATH];
	sprintf(fullFileName, "%s\\%s", dirPath, fileName);

	return fopen(fullFileName, "wb");
}

bool FSL_IsDirWritable(const char *dirPath)
{
	// TODO: This is horrible. We probably should handle this by checking
	// security contexts or whatnot.
	char fullFileName[MAX_PATH];
	sprintf(fullFileName, "%s\\TestFile", dirPath);
	FILE *testFile = fopen(fullFileName, "wb");
	if (testFile)
	{
		fclose(testFile);
		DeleteFile(fullFileName);
		return true;
	}
	return false;
}

size_t FS_GetFileSize(FS_File file)
{
	HANDLE fHandle = (HANDLE)_get_osfhandle(_fileno(file));

	// NOTE: size_t is 32-bit on win32 (and none of Keen's files should be big),
	// so we just use the low 32-bits of the filesize here. This stops the
	// annoying compiler warning we'd otherwise get.
	return GetFileSize(fHandle, NULL);
}

#else

#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

FS_File FSL_OpenFileInDirCaseInsensitive(const char *dirPath, const char *fileName, bool forWrite)
{
	DIR *currentDirPtr = opendir(dirPath);
	if (!currentDirPtr)
		return 0;

	// Search the current directory for matching names.
	for (struct dirent *dirEntry = readdir(currentDirPtr); dirEntry; dirEntry = readdir(currentDirPtr))
	{
		if (!CK_Cross_strcasecmp(dirEntry->d_name, fileName))
		{
			// We've found our file!
			char *fullFileName = (char *)malloc(strlen(dirPath) + 2 + strlen(dirEntry->d_name));
			sprintf(fullFileName, "%s/%s", dirPath, dirEntry->d_name);

			FS_File f;
			if (forWrite)
				f = fopen(fullFileName, "wb");
			else
				f = fopen(fullFileName, "rb");

			free(fullFileName);
			closedir(currentDirPtr);
			return f;
		}
	}
	closedir(currentDirPtr);
	return 0;
}

FS_File FSL_CreateFileInDir(const char *dirPath, const char *fileName)
{
	char *fullFileName = (char *)malloc(strlen(dirPath) + 2 + strlen(fileName));
	sprintf(fullFileName, "%s/%s", dirPath, fileName);

	FS_File f = fopen(fullFileName, "wb");
	free(fullFileName);
	return f;
}

bool FSL_IsDirWritable(const char *dirPath)
{
	// TODO: Check this works on DOS with (e.g.) write protected floppies.
	return (access(dirPath, W_OK | X_OK) == 0);
}

size_t FS_GetFileSize(FS_File file)
{
	long int oldPos = ftell(file);
	fseek(file, 0, SEEK_END);
	long int fileSize = ftell(file);
	fseek(file, oldPos, SEEK_SET);
	return fileSize;
}

#endif

FS_File FS_OpenKeenFile(const char *fileName)
{
	return FSL_OpenFileInDirCaseInsensitive(fs_keenPath, fileName, false);
}

FS_File FS_OpenOmniFile(const char *fileName)
{
#ifdef FS_PREFER_KEEN_PATH
	// We should look for Omnispeak files (headers, actions, etc) in the
	// Keen drictory first, in case we're dealing with a game which has
	// them (e.g., a mod)
	FS_File file = FSL_OpenFileInDirCaseInsensitive(fs_keenPath, fileName, false);
	if (FS_IsFileValid(file))
		return file;
#endif
	return FSL_OpenFileInDirCaseInsensitive(fs_omniPath, fileName, false);
}

FS_File FS_OpenUserFile(const char *fileName)
{
	return FSL_OpenFileInDirCaseInsensitive(fs_userPath, fileName, false);
}

FS_File FS_CreateUserFile(const char *fileName)
{
	FS_File file = FSL_OpenFileInDirCaseInsensitive(fs_userPath, fileName, true);

	if (!FS_IsFileValid(file))
		file = FSL_CreateFileInDir(fs_userPath, fileName);

	return file;
}

// Does a file exist (and is it readable)
bool FS_IsKeenFilePresent(const char *filename)
{
	FS_File file = FS_OpenKeenFile(filename);
	if (!FS_IsFileValid(file))
		return false;
	FS_CloseFile(file);
	return true;
}

// Does a file exist (and is it readable)
bool FS_IsOmniFilePresent(const char *filename)
{
	FS_File file = FS_OpenOmniFile(filename);
	if (!FS_IsFileValid(file))
		return false;
	FS_CloseFile(file);
	return true;
}

// Adjusts the extension on a filename to match the current episode.
// This function is NOT thread safe, and the string returned is only
// valid until the NEXT invocation of this function.
char *FS_AdjustExtension(const char *filename)
{
	static char newname[16];
	strcpy(newname, filename);
	size_t fnamelen = strlen(filename);
	newname[fnamelen - 3] = ck_currentEpisode->ext[0];
	newname[fnamelen - 2] = ck_currentEpisode->ext[1];
	newname[fnamelen - 1] = ck_currentEpisode->ext[2];
	return newname;
}

// Does a file exist (and is it readable)
bool FS_IsUserFilePresent(const char *filename)
{
	FS_File file = FS_OpenUserFile(filename);
	if (!FS_IsFileValid(file))
		return false;
	FS_CloseFile(file);
	return true;
}

bool FSL_IsGoodOmniPath(const char *ext)
{
	if (!FS_IsOmniFilePresent("ACTION.CK4"))
		return false;
	return true;
}

bool FSL_IsGoodUserPath()
{
	return FSL_IsDirWritable(fs_userPath);
}

static const char *fs_parmStrings[] = {"GAMEPATH", "USERPATH", NULL};

void FS_Startup()
{
	// For now, all of the paths will be the current directory.

	fs_keenPath = FS_DEFAULT_KEEN_PATH;
	fs_omniPath = FS_DEFAULT_OMNI_PATH;
	fs_userPath = FS_DEFAULT_USER_PATH;

#ifdef FS_USER_PATH_PREFER_XDG
#ifdef WITH_SDL
#if SDL_VERSION_ATLEAST(2, 0, 1)
	fs_userPath = SDL_GetPrefPath(FS_XDG_ORGANISATION, FS_XDG_APPLICATION);
	if (!fs_userPath || !FSL_IsGoodUserPath())
		fs_userPath = FS_DEFAULT_USER_PATH;
#else
#warning Tried to enable FS_USER_PATH_PREFER_XDG but SDL version is too old.
#endif
#else
#warning FS_USER_PATH_PREFER_XDG requires an SDL2-based backend.
#endif
#endif

#ifdef FS_OMNI_EXEDIR_FALLBACK
#ifdef WITH_SDL
#if SDL_VERSION_ATLEAST(2, 0, 1)
	if (!FSL_IsGoodOmniPath(ck_currentEpisode->ext))
	{
		fs_omniPath = SDL_GetBasePath();
	}
#else
#warning Tried to enable FS_OMNI_EXEDIR_FALLBACK but SDL version is too old.
#endif
#else
#warning FS_OMNI_EXEDIR_FALLBACK requires an SDL2-based backend.
#endif
#endif

#ifdef FS_USER_XDG_FALLBACK
#ifdef WITH_SDL
#if SDL_VERSION_ATLEAST(2, 0, 1)
	if (!FSL_IsGoodUserPath())
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "The default user path \"%s\" is not writable.\n");
		fs_userPath = SDL_GetPrefPath(FS_XDG_ORGANISATION, FS_XDG_APPLICATION);
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Falling back to \"%s\"\n", fs_userPath);
	}
#else
#warning Tried to enable FS_USER_XDG_FALLBACK but SDL version is too old.
#endif
#else
#warning FS_USER_XDG_FALLBACK requires an SDL2-based backend.
#endif
#endif

	// Check command line args.
	for (int i = 1; i < us_argc; ++i)
	{
		int parmIdx = US_CheckParm(us_argv[i], fs_parmStrings);
		switch (parmIdx)
		{
		case 0:
			fs_keenPath = us_argv[++i];
			if (i >= us_argc)
				Quit("/GAMEPATH requires an argument!");
			break;
		case 1:
			fs_userPath = us_argv[++i];
			if (i >= us_argc)
				Quit("/USERPATH requires an argument!");
			break;
		}
	}

	// Check if the paths are good, and warn if not.
	if (!FSL_IsGoodUserPath())
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Cannot write to user path \"%s\": savegames et al will not work.\n", fs_userPath);
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Use the /USERPATH option to set a different path.\n");
	}
}

size_t FS_ReadInt8LE(void *ptr, size_t count, FS_File stream)
{
	return FS_Read(ptr, 1, count, stream);
}

size_t FS_ReadInt16LE(void *ptr, size_t count, FS_File stream)
{
	count = FS_Read(ptr, 2, count, stream);
#ifdef CK_CROSS_IS_BIGENDIAN
	uint16_t *uptr = (uint16_t *)ptr;
	for (size_t loopVar = 0; loopVar < count; loopVar++, uptr++)
		*uptr = CK_Cross_Swap16(*uptr);
#endif
	return count;
}

size_t FS_ReadInt32LE(void *ptr, size_t count, FS_File stream)
{
	count = FS_Read(ptr, 4, count, stream);
#ifdef CK_CROSS_IS_BIGENDIAN
	uint32_t *uptr = (uint32_t *)ptr;
	for (size_t loopVar = 0; loopVar < count; loopVar++, uptr++)
		*uptr = CK_Cross_Swap32(*uptr);
#endif
	return count;
}

size_t FS_WriteInt8LE(const void *ptr, size_t count, FS_File stream)
{
	return FS_Write(ptr, 1, count, stream);
}

size_t FS_WriteInt16LE(const void *ptr, size_t count, FS_File stream)
{
#ifndef CK_CROSS_IS_BIGENDIAN
	return FS_Write(ptr, 2, count, stream);
#else
	uint16_t val;
	size_t actualCount = 0;
	uint16_t *uptr = (uint16_t *)ptr;
	for (size_t loopVar = 0; loopVar < count; loopVar++, uptr++)
	{
		val = CK_Cross_Swap16(*uptr);
		actualCount += FS_Write(&val, 2, 1, stream);
	}
	return actualCount;
#endif
}

size_t FS_WriteInt32LE(const void *ptr, size_t count, FS_File stream)
{
#ifndef CK_CROSS_IS_BIGENDIAN
	return FS_Write(ptr, 4, count, stream);
#else
	uint32_t val;
	size_t actualCount = 0;
	uint32_t *uptr = (uint32_t *)ptr;
	for (size_t loopVar = 0; loopVar < count; loopVar++, uptr++)
	{
		val = CK_Cross_Swap32(*uptr);
		actualCount += FS_Write(&val, 4, 1, stream);
	}
	return actualCount;
#endif
}

size_t FS_ReadBoolFrom16LE(void *ptr, size_t count, FS_File stream)
{
	uint16_t val;
	size_t actualCount = 0;
	bool *currBoolPtr = (bool *)ptr; // No lvalue compilation error
	for (size_t loopVar = 0; loopVar < count; loopVar++, currBoolPtr++)
	{
		if (FS_Read(&val, 2, 1, stream)) // Should be either 0 or 1
		{
			*currBoolPtr = (val); // NOTE: No need to byte-swap
			actualCount++;
		}
	}
	return actualCount;
}

size_t FS_WriteBoolTo16LE(const void *ptr, size_t count, FS_File stream)
{
	uint16_t val;
	size_t actualCount = 0;
	bool *currBoolPtr = (bool *)ptr; // No lvalue compilation error
	for (size_t loopVar = 0; loopVar < count; loopVar++, currBoolPtr++)
	{
		val = CK_Cross_SwapLE16((*currBoolPtr) ? 1 : 0);
		actualCount += FS_Write(&val, 2, 1, stream);
	}
	return actualCount;
}

int FS_PrintF(FS_File stream, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	int ret = vfprintf(stream, fmt, args);
	va_end(args);
	return ret;
}

bool FS_LoadUserFile(const char *filename, mm_ptr_t *ptr, int *memsize)
{
	FS_File f = FS_OpenUserFile(filename);

	if (!FS_IsFileValid(f))
	{
		*ptr = 0;
		*memsize = 0;
		return false;
	}

	//Get length of file
	int length = FS_GetFileSize(f);

	MM_GetPtr(ptr, length);

	if (memsize)
		*memsize = length;

	int amountRead = FS_Read(*ptr, 1, length, f);

	fclose(f);

	if (amountRead != length)
		return false;
	return true;
}
