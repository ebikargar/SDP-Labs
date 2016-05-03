7150 #include "types.h"
7151 #include "x86.h"
7152 #include "defs.h"
7153 #include "kbd.h"
7154 
7155 int
7156 kbdgetc(void)
7157 {
7158   static uint shift;
7159   static uchar *charcode[4] = {
7160     normalmap, shiftmap, ctlmap, ctlmap
7161   };
7162   uint st, data, c;
7163 
7164   st = inb(KBSTATP);
7165   if((st & KBS_DIB) == 0)
7166     return -1;
7167   data = inb(KBDATAP);
7168 
7169   if(data == 0xE0){
7170     shift |= E0ESC;
7171     return 0;
7172   } else if(data & 0x80){
7173     // Key released
7174     data = (shift & E0ESC ? data : data & 0x7F);
7175     shift &= ~(shiftcode[data] | E0ESC);
7176     return 0;
7177   } else if(shift & E0ESC){
7178     // Last character was an E0 escape; or with 0x80
7179     data |= 0x80;
7180     shift &= ~E0ESC;
7181   }
7182 
7183   shift |= shiftcode[data];
7184   shift ^= togglecode[data];
7185   c = charcode[shift & (CTL | SHIFT)][data];
7186   if(shift & CAPSLOCK){
7187     if('a' <= c && c <= 'z')
7188       c += 'A' - 'a';
7189     else if('A' <= c && c <= 'Z')
7190       c += 'a' - 'A';
7191   }
7192   return c;
7193 }
7194 
7195 void
7196 kbdintr(void)
7197 {
7198   consoleintr(kbdgetc);
7199 }
