%ext ck6

%version 1.5

%patch $8F7C $EA $12541719RL # Jump to CAL_DialogUpdate

# Replace CAL_DialogUpdate itself
%patch $13C59 $75 $3C
              $6A $00
              $6A $04
              $1E
              $68 $E6C2W
              $6A $01
              $9A $12540136RL
              $83 $C4 $0A
              $6A $00
              $6A $5A
              $1E
              $68 $9408W
              $6A $01
              $9A $12540136RL
              $83 $C4 $0A
              $6A $00
              $68 $1DB0W
              $1E
              $68 $75CEW
              $6A $01
              $9A $12540136RL
              $83 $C4 $0A
              $EA $06D62221RL
              $EA $06D6212BRL

%end
