# This is a hacky sdl2-config.cmake implementation for the SDL mingw32 packages on libsdl.org
# It's based loosely on the version contained in the tarball, combined with the MSVC one here:
#  https://trenki2.github.io/blog/2017/06/02/using-sdl2-with-cmake/

# Support both 32 and 64 bit builds
if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/x86_64-w64-mingw32/include/SDL2")
	set(SDL2_LIBRARIES "-L${CMAKE_CURRENT_LIST_DIR}/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows")
else ()
	set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/i686-w64-mingw32/include/SDL2")
	set(SDL2_LIBRARIES "-L${CMAKE_CURRENT_LIST_DIR}/i686-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows")
endif ()

string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
