IDOBJECTS=ck_quit.o id_mm.o id_ca.o id_in.o id_rf.o id_ti.o id_us_1.o id_vh.o id_vl.o 
CKOBJECTS=ck_keen.o ck_obj1.o ck_obj3.o ck_phys.o ck_play.o ck5_misc.o ck_decode.o

OUTBIN=keen5e

OBJ := $(IDOBJECTS:%=../obj/%)
OBJ += $(CKOBJECTS:%=../obj/%)
DEPS := $(OBJ:%=%.d)

.PHONY: all clean

all: $(OUTBIN)

clean:
	rm $(OUTBIN) $(OBJ) $(DEPS)

../obj/%.o: %.c
	@mkdir -p $(dir $@)
	cc -g `sdl-config --cflags` -c --std=c99 -o $@ $<
	@cpp $< -MM -MF $@.d

$(OUTBIN): $(OBJ)
	cc -g -lm -o $(OUTBIN) $(OBJ) `sdl-config --libs` 

-include $(DEPS)
