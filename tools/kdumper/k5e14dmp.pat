%ext ck5

%version 1.4

%patch $8E7B $EA $13A017BCRL # Jump to CAL_DialogUpdate

# Replace CAL_DialogUpdate itself
%patch $151BC $75 $3C
              $6A $00
              $6A $04
              $1E
              $68 $9BA5W
              $6A $01
              $9A $13A0013DRL
              $83 $C4 $0A
              $6A $00
              $6A $58
              $1E
              $68 $6F18W
              $6A $01
              $9A $13A0013DRL
              $83 $C4 $0A
              $6A $00
              $68 $1DB0W
              $1E
              $68 $9E6FW
              $6A $01
              $9A $13A0013DRL
              $83 $C4 $0A
              $EA $06B922F0RL
              $EA $06B921FARL

%end
