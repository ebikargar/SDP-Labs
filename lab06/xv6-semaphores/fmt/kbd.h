7000 // PC keyboard interface constants
7001 
7002 #define KBSTATP         0x64    // kbd controller status port(I)
7003 #define KBS_DIB         0x01    // kbd data in buffer
7004 #define KBDATAP         0x60    // kbd data port(I)
7005 
7006 #define NO              0
7007 
7008 #define SHIFT           (1<<0)
7009 #define CTL             (1<<1)
7010 #define ALT             (1<<2)
7011 
7012 #define CAPSLOCK        (1<<3)
7013 #define NUMLOCK         (1<<4)
7014 #define SCROLLLOCK      (1<<5)
7015 
7016 #define E0ESC           (1<<6)
7017 
7018 // Special keycodes
7019 #define KEY_HOME        0xE0
7020 #define KEY_END         0xE1
7021 #define KEY_UP          0xE2
7022 #define KEY_DN          0xE3
7023 #define KEY_LF          0xE4
7024 #define KEY_RT          0xE5
7025 #define KEY_PGUP        0xE6
7026 #define KEY_PGDN        0xE7
7027 #define KEY_INS         0xE8
7028 #define KEY_DEL         0xE9
7029 
7030 // C('A') == Control-A
7031 #define C(x) (x - '@')
7032 
7033 static uchar shiftcode[256] =
7034 {
7035   [0x1D] CTL,
7036   [0x2A] SHIFT,
7037   [0x36] SHIFT,
7038   [0x38] ALT,
7039   [0x9D] CTL,
7040   [0xB8] ALT
7041 };
7042 
7043 static uchar togglecode[256] =
7044 {
7045   [0x3A] CAPSLOCK,
7046   [0x45] NUMLOCK,
7047   [0x46] SCROLLLOCK
7048 };
7049 
7050 static uchar normalmap[256] =
7051 {
7052   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7053   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7054   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7055   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7056   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7057   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7058   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7059   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7060   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7061   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7062   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7063   [0x9C] '\n',      // KP_Enter
7064   [0xB5] '/',       // KP_Div
7065   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7066   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7067   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7068   [0x97] KEY_HOME,  [0xCF] KEY_END,
7069   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7070 };
7071 
7072 static uchar shiftmap[256] =
7073 {
7074   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7075   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7076   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7077   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7078   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7079   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7080   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7081   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7082   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7083   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7084   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7085   [0x9C] '\n',      // KP_Enter
7086   [0xB5] '/',       // KP_Div
7087   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7088   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7089   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7090   [0x97] KEY_HOME,  [0xCF] KEY_END,
7091   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7092 };
7093 
7094 
7095 
7096 
7097 
7098 
7099 
7100 static uchar ctlmap[256] =
7101 {
7102   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7103   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7104   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7105   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7106   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7107   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7108   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7109   [0x9C] '\r',      // KP_Enter
7110   [0xB5] C('/'),    // KP_Div
7111   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7112   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7113   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7114   [0x97] KEY_HOME,  [0xCF] KEY_END,
7115   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7116 };
7117 
7118 
7119 
7120 
7121 
7122 
7123 
7124 
7125 
7126 
7127 
7128 
7129 
7130 
7131 
7132 
7133 
7134 
7135 
7136 
7137 
7138 
7139 
7140 
7141 
7142 
7143 
7144 
7145 
7146 
7147 
7148 
7149 
