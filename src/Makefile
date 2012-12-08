IDOBJECTS=ck_quit.o id_mm.o id_ca.o id_in.o id_rf.o id_ti.o id_us_1.o id_us_2.o id_vh.o id_vl.o id_str.o
CKOBJECTS=ck_act.o ck_keen.o ck5_obj1.o ck5_obj3.o ck_phys.o ck_game.o ck_play.o ck5_misc.o ck_main.o

OUTBIN=../bin/keen5e

OBJ := $(IDOBJECTS:%=../obj/%)
OBJ += $(CKOBJECTS:%=../obj/%)
DEPS := $(OBJ:%.o=%.d)

.PHONY: all clean

all: $(OUTBIN)

clean:
	rm -f $(OUTBIN) $(OBJ) $(DEPS)

../obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -g `sdl-config --cflags` -c --std=c99 -MMD -o $@ $<

$(OUTBIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -g -lm -o $(OUTBIN) $(OBJ) `sdl-config --libs` 

-include $(DEPS)
