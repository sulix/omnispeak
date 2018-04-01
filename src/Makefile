#Omnispeak: A Commander Keen Reimplementation
#Copyright (C) 2012 David Gow
#Omnispeak is available at http://davidgow.net/keen/omnispeak.html
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

KEEN6VER=keen6e15

IDOBJECTS=ck_quit.o id_mm.o id_ca.o id_in.o id_in_sdl.o id_rf.o id_sd.o id_sd_sdl.o id_ti.o id_us_1.o id_us_2.o id_us_textscreen.o id_vh.o id_vl.o id_str.o
CKOBJECTS=ck_act.o ck_inter.o ck_keen.o ck_obj.o ck_map.o ck4_map.o ck5_map.o ck6_map.o ck4_obj1.o ck4_obj2.o ck4_obj3.o ck5_obj1.o ck5_obj2.o ck5_obj3.o ck6_obj1.o ck6_obj2.o ck6_obj3.o ck_phys.o ck_game.o ck_play.o ck_misc.o ck4_misc.o ck5_misc.o ck6_misc.o ck_main.o ck_text.o ck_cross.o icon.o
OPLOBJECTS=opl/dbopl.o

K4DATA=../bin/ACTION.CK4 ../bin/AUDINFOE.CK4 ../bin/AUDIODCT.CK4 ../bin/AUDIOHHD.CK4 ../bin/EGADICT.CK4 ../bin/EGAHEAD.CK4 ../bin/GFXINFOE.CK4 ../bin/MAPHEAD.CK4 ../bin/TILEINFO.CK4
K5DATA=../bin/ACTION.CK5 ../bin/AUDINFOE.CK5 ../bin/AUDIODCT.CK5 ../bin/AUDIOHHD.CK5 ../bin/EGADICT.CK5 ../bin/EGAHEAD.CK5 ../bin/GFXINFOE.CK5 ../bin/MAPHEAD.CK5 ../bin/TILEINFO.CK5
K6DATA=../bin/ACTION.CK6 ../bin/AUDINFOE.CK6 ../bin/AUDIODCT.CK6 ../bin/AUDIOHHD.CK6 ../bin/EGADICT.CK6 ../bin/EGAHEAD.CK6 ../bin/GFXINFOE.CK6 ../bin/MAPHEAD.CK6 ../bin/TILEINFO.CK6

ifeq ($(PLATFORM), WINDOWS)
	EXE_EXT=.exe
endif

BUILDASCPP=1
BINPREFIX=

ifeq ($(BUILDASCPP), 1)
	CXX=$(BINPREFIX)g++
else
	CXX=$(BINPREFIX)gcc
endif

INTCXXFLAGS=

ifeq ($(BUILDASCPP), 0)
	INTCXXFLAGS+= -std=c99
endif

WITH_DUMPER=0
ifeq ($(WITH_DUMPER), 1)
	INTCXXFLAGS += -DCK_ENABLE_PLAYLOOP_DUMPER
endif

OUTBIN=../bin/omnispeak$(EXE_EXT)

LIBS=
WITH_SDL2=1
WITH_OPENGL=0
WITH_VULKAN=0
ifeq ($(WITH_SDL2), 1)
	ifeq ($(WITH_VULKAN), 1)
		LIBS+= -lvulkan -lX11 -lX11-xcb
		IDOBJECTS+= id_vl_sdl2vk.o 
	else
		ifeq ($(WITH_OPENGL), 1)
			ifeq ($(PLATFORM), WINDOWS)
				LIBS+= -lopengl32
			else
				LIBS+= -lGL
			endif
			IDOBJECTS+= id_vl_sdl2gl.o 
		else
			IDOBJECTS+= id_vl_sdl2.o 
		endif
	endif
	SDLCONFIGBIN=sdl2-config #/local/cross-tools/x86_64-w64-mingw32/bin/sdl2-config
else
	IDOBJECTS+= id_vl_sdl12.o
	SDLCONFIGBIN=sdl-config
endif

OBJ := $(IDOBJECTS:%=../obj/%)
OBJ += $(CKOBJECTS:%=../obj/%)
OBJ += $(OPLOBJECTS:%=../obj/%)
DEPS := $(OBJ:%.o=%.d)

#SDL_CONFIG=/home/david/builds/SDL-1.2.15/libs-32/bin/$(SDLCONFIGBIN)
#SDL_CONFIG=/home/david/builds/libs/bin/$(SDLCONFIGBIN)
SDL_CONFIG=$(SDLCONFIGBIN)

.PHONY: all clean keen4data keen5data keen6data binfiles

all: $(OUTBIN) binfiles

clean:
	rm -f $(OUTBIN) $(OBJ) $(DEPS) $(K4DATA) $(K5DATA) $(K6DATA)

../bin/%.CK4: ../data/keen4/%.CK4
	cp -f $< $@ 

../bin/%.CK5: ../data/keen5/%.CK5
	cp -f $< $@ 

../bin/%.CK6: ../data/$(KEEN6VER)/%.CK6
	cp -f $< $@

keen4data: $(K4DATA)
keen5data: $(K5DATA)
keen6data: $(K6DATA)
	
binfiles: keen4data keen5data keen6data

../obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CXX) $(INTCXXFLAGS) $(CXXFLAGS) -g `$(SDL_CONFIG) --cflags` -mno-ms-bitfields -c -MMD -o $@ $<

$(OUTBIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(INTCXXFLAGS) $(CXXFLAGS) -g -o $(OUTBIN) $(OBJ) `$(SDL_CONFIG) --libs` -lm $(LIBS)

-include $(DEPS)
