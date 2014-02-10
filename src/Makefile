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

IDOBJECTS=ck_quit.o id_mm.o id_ca.o id_in.o id_rf.o id_sd.o id_ti.o id_us_1.o id_us_2.o id_vh.o id_vl.o id_str.o glad.o
CKOBJECTS=ck_act.o ck_inter.o ck_keen.o ck5_map.o ck5_obj1.o ck5_obj2.o ck5_obj3.o ck_phys.o ck_game.o ck_play.o ck5_misc.o ck_main.o ck_text.o

ifeq ($(PLATFORM), WINDOWS)
	EXE_EXT=.exe
endif

BINPREFIX=
CC=$(BINPREFIX)gcc

OUTBIN=../bin/omnispeak5e$(EXE_EXT)

WITH_SDL2=1
ifeq ($(WITH_SDL2), 1)
	IDOBJECTS+= id_vl_sdl2gl.o
	SDLCONFIGBIN=/local/cross-tools/x86_64-w64-mingw32/bin/sdl2-config
else
	IDOBJECTS+= id_vl_sdl12.o
	SDLCONFIGBIN=sdl-config
endif

OBJ := $(IDOBJECTS:%=../obj/%)
OBJ += $(CKOBJECTS:%=../obj/%)
DEPS := $(OBJ:%.o=%.d)

#SDL_CONFIG=/home/david/builds/SDL-1.2.15/libs-32/bin/$(SDLCONFIGBIN)
#SDL_CONFIG=/home/david/builds/libs/bin/$(SDLCONFIGBIN)
SDL_CONFIG=$(SDLCONFIGBIN)

.PHONY: all clean

all: $(OUTBIN) binfiles

clean:
	rm -f $(OUTBIN) $(OBJ) $(DEPS)

binfiles: ../data/keen5/ACTION.CK5
	cp -f ../data/keen5/ACTION.CK5 ../bin/ACTION.CK5

../obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -g `$(SDL_CONFIG) --cflags` -mno-ms-bitfields -c --std=c99 -MMD -o $@ $<

$(OUTBIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -g -lm -o $(OUTBIN) $(OBJ) `$(SDL_CONFIG) --libs` 

-include $(DEPS)
