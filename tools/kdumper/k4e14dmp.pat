%ext ck4

%patch $8F09 $EA $12A617BFRL # Jump to CAL_DialogUpdate

# Replace CAL_DialogUpdate itself
%patch $1421F $75 $3C
              $6A $00
              $6A $04
              $1E
              $68 $A53DW
              $6A $01
              $9A $12A60140RL
              $83 $C4 $0A
              $6A $00
              $6A $56
              $1E
              $68 $7A1AW
              $6A $01
              $9A $12A60140RL
              $83 $C4 $0A
              $6A $00
              $68 $1DB0W
              $1E
              $68 $A807W
              $6A $01
              $9A $12A60140RL
              $83 $C4 $0A
              $EA $06BD233ERL
              $EA $06BD2248RL

%end
