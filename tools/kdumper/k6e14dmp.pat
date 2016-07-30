%ext ck6

%ver 1.4

%patch $8CE3 $EA $125A17B0RL # Jump to CAL_DialogUpdate

# Replace CAL_DialogUpdate itself
%patch $13D50 $75 $3C
              $6A $00
              $6A $04
              $1E
              $68 $A6CBW
              $6A $01
              $9A $125A0131RL
              $83 $C4 $0A
              $6A $00
              $6A $5A
              $1E
              $68 $7552W
              $6A $01
              $9A $125A0131RL
              $83 $C4 $0A
              $6A $00
              $68 $1DB0W
              $1E
              $68 $A995W
              $6A $01
              $9A $125A0131RL
              $83 $C4 $0A
              $EA $069A2348RL
              $EA $069A2252RL

%end
