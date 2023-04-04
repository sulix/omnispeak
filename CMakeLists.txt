cmake_minimum_required(VERSION 3.0)

project(omnispeak)

option(WITH_ASAN "Compile with Address Sanitizer" OFF)
add_compile_options("$<$<CONFIG:DEBUG>:-DCK_DEBUG>")

# Default to 'sdl2' on Windows
if (WIN32)
	set(RENDERER "sdl2" CACHE STRING "the type of video output API")
else ()
	set(RENDERER "sdl2gl" CACHE STRING "the type of video output API")
endif()

# Misc config options
option(VANILLA "whether to disable omnispeak-only features" OFF)
option(BUILDASCPP "compile all 'c' source files with the C++ compiler" OFF)
option(WITH_KEEN4 "include Keen 4: Secret of the Oracle support" ON)
option(WITH_KEEN5 "include Keen 5: The Armageddon Machine support" ON)
option(WITH_KEEN6 "include Keen 6: Aliens Ate My Baby Sitter support" ON)

set(KEENPATH "." CACHE STRING "set the default path to the Commander Keen data files")
set(USERPATH "." CACHE STRING "set the default path for user savegames")
set(OMNIPATH ${USERPATH} CACHE STRING "set the default path for Omnispeak's bundled data files")
option(XDGUSERPATH "prefer XDG user paths to the default path above" OFF)


# Optional SD backends
option(WITH_ALSA "whether to include ALSA support for real OPL2 hardware (Linux only)" OFF)
option(WITH_IEEE1284 "whether to include support for the OPL2LPT parallel port soundcard" OFF)
option(WITH_OPLHW "whether to include support for real OPL2 hardware via liboplhw" OFF)

# TODO: Handle this
#list(APPEND CMAKE_PREFIX_PATH "cmake/")

IF(WITH_ASAN)
	set(CMAKE_C_FLAGS "-fsanitize=address")
	set(CMAKE_CXX_FLAGS "-fsanitize=address")
	set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
ENDIF()

if(VANILLA)
	add_definitions(-DVANILLA=1)
endif()

# Handle path magic
if(KEENPATH)
	add_definitions(-DFS_DEFAULT_KEEN_PATH="${KEENPATH}")
endif()
if(USERPATH)
	add_definitions(-DFS_DEFAULT_USER_PATH="${USERPATH}")
endif()
if(OMNIPATH)
	add_definitions(-DFS_DEFAULT_OMNI_PATH="${OMNIPATH}")
endif()
if(XDGUSERPATH)
	add_definitions(-DFS_USER_PATH_PREFER_XDG=1)
endif()

if(RENDERER STREQUAL "sdl2")
	message(STATUS "Using SDL 2.0 renderer backend")
	find_package(SDL2 CONFIG)
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_sdl.c
		src/id_sd_sdl.c
		src/id_vl_sdl2.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${SDL2_LIBRARIES}
	)
	include_directories(
		${SDL2_INCLUDE_DIRS}
	)
	add_definitions(-DWITH_SDL)
elseif(RENDERER STREQUAL "sdl2vk")
	message(STATUS "Using SDL 2.0 + Vulkan renderer backend")
	find_package(SDL2 CONFIG)
	find_package(Vulkan)

	add_custom_command(OUTPUT id_vl_vk_vert.h
		COMMAND glslc -mfmt=c
			${CMAKE_CURRENT_SOURCE_DIR}/src/id_vl_vk_shader.vert
			-o id_vl_vk_vert.h
		DEPENDS src/id_vl_vk_shader.vert
	)

	add_custom_command(OUTPUT id_vl_vk_frag.h
		COMMAND glslc -mfmt=c
			${CMAKE_CURRENT_SOURCE_DIR}/src/id_vl_vk_shader.frag
			-o id_vl_vk_frag.h
		DEPENDS src/id_vl_vk_shader.frag
	)

	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_sdl.c
		src/id_sd_sdl.c
		src/id_vl_sdl2vk.c
		${CMAKE_CURRENT_BINARY_DIR}/id_vl_vk_vert.h
		${CMAKE_CURRENT_BINARY_DIR}/id_vl_vk_frag.h
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${Vulkan_LIBRARIES}
		${SDL2_LIBRARIES}
	)
	include_directories(
		${SDL2_INCLUDE_DIRS}
		${VULKAN_INCLUDE_DIRS}
		${CMAKE_CURRENT_BINARY_DIR}
	)
	add_definitions(-DWITH_SDL)
elseif(RENDERER STREQUAL "sdl2gl")
	message(STATUS "Using SDL 2.0 + OpenGL renderer backend")
	find_package(SDL2 CONFIG)
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_sdl.c
		src/id_sd_sdl.c
		src/id_vl_sdl2gl.c
	)
	if(APPLE)
		set(OMNISPEAK_PLATFORM_LIBRARIES
			${SDL2_LIBRARIES}
			"-framework OpenGL"
		)
	elseif (UNIX)
		set(OMNISPEAK_PLATFORM_LIBRARIES
			${SDL2_LIBRARIES}
			GL
		)
	elseif(WIN32)
		set(OMNISPEAK_PLATFORM_LIBRARIES
			${SDL2_LIBRARIES}
			OpenGL32
		)
	else()
		message(WARNING "No OpenGL library specified for your platform!")
	endif()
		
	include_directories(
		${SDL2_INCLUDE_DIRS}
	)
	add_definitions(-DWITH_SDL)
elseif(RENDERER STREQUAL "dos")
	message(STATUS "Using DOS/EGA renderer backend")
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_dos.c
		src/id_sd_dos.c
		src/id_vl_dos.c
	)
elseif(RENDERER STREQUAL "sdl1")
	message(STATUS "Using SDL 1.2 renderer backend")
	find_package(SDL REQUIRED)
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_sdl.c
		src/id_sd_sdl.c
		src/id_vl_sdl12.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${SDL_LIBRARY}
	)
	include_directories(
		${SDL_INCLUDE_DIR}
	)
	add_definitions(-DWITH_SDL)
elseif(RENDERER STREQUAL "sdl3")
	message(STATUS "Using SDL3 renderer backend")
	find_package(SDL3 REQUIRED)
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_sdl3.c
		src/id_sd_sdl3.c
		src/id_vl_sdl3.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${SDL3_LIBRARIES}
	)
	include_directories(
		${SDL3_INCLUDE_DIRS}
	)
	add_definitions(-DWITH_SDL=3)
else()
	message(WARNING "Using NULL platform layer.")
	set(OMNISPEAK_PLATFORM_SRCS
		src/id_in_null.c
		src/id_sd_null.c
		src/id_vl_null.c
	)
endif()

#TODO: rpath magic on Linux

if(WITH_ALSA)
	message(STATUS "Enabling ALSA OPL2 support...")
	if (NOT UNIX)
		message(WARNING "Attempting to use ALSA on a non-Unix platform! Good Luck!")
	endif ()
	# TODO: find_package(ALSA) doesn't seem to find ALSA on my machine,
	# so we won't use it for now. This really shouldn't be hardcoded,
	# though, so we'll revisit this later.
	set(OMNISPEAK_PLATFORM_SRCS
		${OMNISPEAK_PLATFORM_SRCS}
		src/id_sd_opl2alsa.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${OMNISPEAK_PLATFORM_LIBRARIES}
		asound
	)
	add_definitions(-DSD_OPL2_WITH_ALSA)
endif()

if(WITH_IEEE1284)
	message(STATUS "Enabling OPL2LPT support (libieee1284)...")

	set(OMNISPEAK_PLATFORM_SRCS
		${OMNISPEAK_PLATFORM_SRCS}
		src/id_sd_opl2lpt.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${OMNISPEAK_PLATFORM_LIBRARIES}
		ieee1284
	)
	add_definitions(-DSD_OPL2_WITH_IEEE1284)
endif()

if(WITH_OPLHW)
	message(STATUS "Enabling hardware OPL2 support (liboplhw)...")

	set(OMNISPEAK_PLATFORM_SRCS
		${OMNISPEAK_PLATFORM_SRCS}
		src/id_sd_liboplhw.c
	)
	set(OMNISPEAK_PLATFORM_LIBRARIES
		${OMNISPEAK_PLATFORM_LIBRARIES}
		oplhw
	)
	add_definitions(-DSD_OPL2_WITH_LIBOPLHW)
endif()

if(WIN32)
	set(OMNISPEAK_PLATFORM_SRCS
		${OMNISPEAK_PLATFORM_SRCS}
		src/windowsres.rc
	)
endif()

set(OMNISPEAK_ID_SRCS
	src/id_heads.h
	src/id_ca.h
	src/id_ca.c
	src/id_cfg.h
	src/id_cfg.c
	src/id_fs.h
	src/id_fs.c
	src/id_in.h
	src/id_in.c
	src/id_mm.h
	src/id_mm.c
	src/id_rf.h
	src/id_rf.c
	src/id_sd.h
	src/id_sd.c
	src/id_str.h
	src/id_str.c
	src/id_ti.c
	src/id_ti.h
	src/id_us.h
	src/id_us_1.c
	src/id_us_2.c
	src/id_us_textscreen.c
	src/id_vh.h
	src/id_vh.c
	src/id_vl.h
	src/id_vl.c
	src/id_vl_private.h
)

set(OMNISPEAK_CK4_SRCS
	src/ck4_ep.h
	src/ck4_map.c
	src/ck4_misc.c
	src/ck4_obj1.c
	src/ck4_obj2.c
	src/ck4_obj3.c
)

set(OMNISPEAK_CK5_SRCS
	src/ck5_ep.h
	src/ck5_map.c
	src/ck5_misc.c
	src/ck5_obj1.c
	src/ck5_obj2.c
	src/ck5_obj3.c
)

set(OMNISPEAK_CK6_SRCS
	src/ck6_ep.h
	src/ck6_map.c
	src/ck6_misc.c
	src/ck6_obj1.c
	src/ck6_obj2.c
	src/ck6_obj3.c
)

set(OMNISPEAK_CK_SRCS
	src/ck_act.c
	src/ck_act.h
	src/ck_cross.c
	src/ck_cross.h
	src/ck_def.h
	src/ck_ep.h
	src/ck_game.c
	src/ck_game.h
	src/ck_inter.c
	src/ck_keen.c
	src/ck_main.c
	src/ck_map.c
	src/ck_misc.c
	src/ck_obj.c
	src/ck_phys.c
	src/ck_phys.h
	src/ck_play.c
	src/ck_play.h
	src/ck_quit.c
	src/ck_text.c
	src/ck_text.h
	src/icon.c
)

set(OMNISPEAK_OPL_SRCS
	src/opl/dbopl.h
	src/opl/dbopl.c
	src/opl/nuked_opl3.h
	src/opl/nuked_opl3.c
)

# Build all episodes
if (WITH_KEEN4)
	add_definitions(-DWITH_KEEN4=1)
	set(OMNISPEAK_EPISODE_SRCS
		${OMNISPEAK_EPISODE_SRCS}
		${OMNISPEAK_CK4_SRCS}
	)
endif()
if (WITH_KEEN5)
	add_definitions(-DWITH_KEEN5=1)
	set(OMNISPEAK_EPISODE_SRCS
		${OMNISPEAK_EPISODE_SRCS}
		${OMNISPEAK_CK5_SRCS}
	)
endif()
if (WITH_KEEN6)
	add_definitions(-DWITH_KEEN6=1)
	set(OMNISPEAK_EPISODE_SRCS
		${OMNISPEAK_EPISODE_SRCS}
		${OMNISPEAK_CK6_SRCS}
	)
endif()

if (BUILDASCPP)
	set(OMNISPEAK_C_SRC_FILES
		${OMNISPEAK_ID_SRCS}
		${OMNISPEAK_EPISODE_SRCS}
		${OMNISPEAK_CK_SRCS}
		${OMNISPEAK_PLATFORM_SRCS}
		${OMNISPEAK_OPL_SRCS}
	)

	list(FILTER OMNISPEAK_C_SRC_FILES INCLUDE REGEX ".*\\.c")

	set_source_files_properties(
		${OMNISPEAK_C_SRC_FILES}
		PROPERTIES LANGUAGE CXX
	)
endif()


# On Watcom-based compilers, we need to force enums to be the same size as
# 'int', else we have an ABI mismatch with SDL. Equally, we need to enable
# support for non-constant intitialisers, which we use a bit in the SDL
# backends for intitialising (e.g.) SDL_Rects.
if (CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom")
	add_definitions(-ei -aa -za99)
endif()

# In Microsoft compilers, the C Runtime is dynamically linked by default.
# Since Microsoft's C Runtime is distributed with a separate "redistributable"
# package, and is compiler-version-specific, we'd either need to package the
# redist installer (and get people to install it, except on versions prior to
# 2005), or hope the user already has it from another app, before the binary
# can run. This is even worse for Debug builds, as very few end users will
# have the seprate debug CRT installed. Therefore, statically link the rutime.
if (MSVC)
	# There is a better way of doing this, but it only works on very new
	# CMake versions.
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} /MT")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_DEBUG} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} /MT")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_DEBUG} /MT")
endif()

add_executable(omnispeak
	${OMNISPEAK_ID_SRCS}
	${OMNISPEAK_EPISODE_SRCS}
	${OMNISPEAK_CK_SRCS}
	${OMNISPEAK_PLATFORM_SRCS}
	${OMNISPEAK_OPL_SRCS}
)

if (NOT BUILDASCPP)
	set_property(TARGET omnispeak PROPERTY C_STANDARD 99)
endif ()
target_link_libraries(omnispeak
	${OMNISPEAK_PLATFORM_LIBRARIES}
)

# On Unix, we need to link libm
# TODO: Should this be for Unix, or for gcc?
if (UNIX)
	target_link_libraries(omnispeak m)
endif()
