
#include "keyboard.h"

typedef struct
{
    byte ascii1;   // no shift code
    byte ascii2;   // shift code
    byte scode;    // scan code
    byte kcode;    // key code
} KeyCode;

enum
{
    KeyRelease = 0,
    KeyPress = 0x1000000
};

static const KeyCode gKeyMap[] = 
{
/* 0x00 - none      */ {  0,        0,        0,         0   },
/* 0x01 - ESC       */ {  0,        0,       0x01,      0x1B },
/* 0x02 - '1'       */ { '1',      '!',      0x02,      0x31 },
/* 0x03 - '2'       */ { '2',      '@',      0x03,      0x32 },
/* 0x04 - '3'       */ { '3',      '#',      0x04,      0x33 },
/* 0x05 - '4'       */ { '4',      '$',      0x05,      0x34 },
/* 0x06 - '5'       */ { '5',      '%',      0x06,      0x35 },
/* 0x07 - '6'       */ { '6',      '^',      0x07,      0x36 },
/* 0x08 - '7'       */ { '7',      '&',      0x08,      0x37 },
/* 0x09 - '8'       */ { '8',      '*',      0x09,      0x38 },
/* 0x0A - '9'       */ { '9',      '(',      0x0A,      0x39 },
/* 0x0B - '0'       */ { '0',      ')',      0x0B,      0x30 },
/* 0x0C - '-'       */ { '-',      '_',      0x0C,      0xBD },
/* 0x0D - '='       */ { '=',      '+',      0x0D,      0xBB },
/* 0x0E - BS        */ {  0,        0,       0x0E,      0x08 },
/* 0x0F - TAB       */ {  0,        0,       0x0F,      0x09 },
/* 0x10 - 'q'       */ { 'q',      'Q',      0x10,      0x51 },
/* 0x11 - 'w'       */ { 'w',      'W',      0x11,      0x57 },
/* 0x12 - 'e'       */ { 'e',      'E',      0x12,      0x45 },
/* 0x13 - 'r'       */ { 'r',      'R',      0x13,      0x52 },
/* 0x14 - 't'       */ { 't',      'T',      0x14,      0x54 },
/* 0x15 - 'y'       */ { 'y',      'Y',      0x15,      0x59 },
/* 0x16 - 'u'       */ { 'u',      'U',      0x16,      0x55 },
/* 0x17 - 'i'       */ { 'i',      'I',      0x17,      0x49 },
/* 0x18 - 'o'       */ { 'o',      'O',      0x18,      0x4F },
/* 0x19 - 'p'       */ { 'p',      'P',      0x19,      0x50 },
/* 0x1A - '['       */ { '[',      '{',      0x1A,      0xDB },
/* 0x1B - ']'       */ { ']',      '}',      0x1B,      0xDD },
/* 0x1C - CR/LF     */ {  0,        0,       0x1C,      0x0D },
/* 0x1D - l. Ctrl   */ {  0,        0,       0x1D,      0x11 },
/* 0x1E - 'a'       */ { 'a',      'A',      0x1E,      0x41 },
/* 0x1F - 's'       */ { 's',      'S',      0x1F,      0x53 },
/* 0x20 - 'd'       */ { 'd',      'D',      0x20,      0x44 },
/* 0x21 - 'f'       */ { 'f',      'F',      0x21,      0x46 },
/* 0x22 - 'g'       */ { 'g',      'G',      0x22,      0x47 },
/* 0x23 - 'h'       */ { 'h',      'H',      0x23,      0x48 },
/* 0x24 - 'j'       */ { 'j',      'J',      0x24,      0x4A },
/* 0x25 - 'k'       */ { 'k',      'K',      0x25,      0x4B },
/* 0x26 - 'l'       */ { 'l',      'L',      0x26,      0x4C },
/* 0x27 - ';'       */ { ';',      ':',      0x27,      0xBA },
/* 0x28 - '\''      */ { '\'',     '\"',     0x28,      0xDE },
/* 0x29 - '`'       */ { '`',      '~',      0x29,      0xC0 },
/* 0x2A - l. SHIFT  */ {  0,        0,       0x2A,      0x10 },
/* 0x2B - '\'       */ { '\\',     '|',      0x2B,      0xDC },
/* 0x2C - 'z'       */ { 'z',      'Z',      0x2C,      0x5A },
/* 0x2D - 'x'       */ { 'x',      'X',      0x2D,      0x58 },
/* 0x2E - 'c'       */ { 'c',      'C',      0x2E,      0x43 },
/* 0x2F - 'v'       */ { 'v',      'V',      0x2F,      0x56 },
/* 0x30 - 'b'       */ { 'b',      'B',      0x30,      0x42 },
/* 0x31 - 'n'       */ { 'n',      'N',      0x31,      0x4E },
/* 0x32 - 'm'       */ { 'm',      'M',      0x32,      0x4D },
/* 0x33 - ','       */ { ',',      '<',      0x33,      0xBC },
/* 0x34 - '.'       */ { '.',      '>',      0x34,      0xBE },
/* 0x35 - '/'       */ { '/',      '?',      0x35,      0xBF },
/* 0x36 - r. SHIFT  */ {  0,        0,       0x36,      0x10 },
/* 0x37 - '*'       */ { '*',      '*',      0x37,      0x6A },
/* 0x38 - ALT       */ {  0,        0,       0x38,      0x12 },
/* 0x39 - ' '       */ { ' ',      ' ',      0x39,      0x20 },
/* 0x3A - CapsLock  */ {  0,        0,       0x3A,      0x14 },
/* 0x3B - F1        */ {  0,        0,       0x3B,      0x70 },
/* 0x3C - F2        */ {  0,        0,       0x3C,      0x71 },
/* 0x3D - F3        */ {  0,        0,       0x3D,      0x72 },
/* 0x3E - F4        */ {  0,        0,       0x3E,      0x73 },
/* 0x3F - F5        */ {  0,        0,       0x3F,      0x74 },
/* 0x40 - F6        */ {  0,        0,       0x40,      0x75 },
/* 0x41 - F7        */ {  0,        0,       0x41,      0x76 },
/* 0x42 - F8        */ {  0,        0,       0x42,      0x77 },
/* 0x43 - F9        */ {  0,        0,       0x43,      0x78 },
/* 0x44 - F10       */ {  0,        0,       0x44,      0x79 },
/* 0x45 - NumLock   */ {  0,        0,       0x45,      0x90 },
/* 0x46 - ScrLock   */ {  0,        0,       0x46,      0x91 },
/* 0x47 - Home      */ {  0,        0,       0x47,      0x24 },
/* 0x48 - Up        */ {  0,        0,       0x48,      0x26 },
/* 0x49 - PgUp      */ {  0,        0,       0x49,      0x21 },
/* 0x4A - '-'       */ {  0,        0,       0x4A,      0x6D },
/* 0x4B - Left      */ {  0,        0,       0x4B,      0x25 },
/* 0x4C - MID       */ {  0,        0,       0x4C,      0x0C },
/* 0x4D - Right     */ {  0,        0,       0x4D,      0x27 },
/* 0x4E - '+'       */ {  0,        0,       0x4E,      0x6B },
/* 0x4F - End       */ {  0,        0,       0x4F,      0x23 },
/* 0x50 - Down      */ {  0,        0,       0x50,      0x28 },
/* 0x51 - PgDown    */ {  0,        0,       0x51,      0x22 },
/* 0x52 - Insert    */ {  0,        0,       0x52,      0x2D },
/* 0x53 - Del       */ {  0,        0,       0x53,      0x2E },
/* 0x54 - Enter     */ {  0,        0,       0x54,      0x0D },
/* 0x55 - ???       */ {  0,        0,        0,         0   },
/* 0x56 - ???       */ {  0,        0,        0,         0   },
/* 0x57 - F11       */ {  0,        0,       0x57,      0x7A },  
/* 0x58 - F12       */ {  0,        0,       0x58,      0x7B },
/* 0x59 - ???       */ {  0,        0,        0,         0   },
/* 0x5A - ???       */ {  0,        0,        0,         0   },
/* 0x5B - Left Win  */ {  0,        0,       0x5B,      0x5B },	
/* 0x5C - Right Win */ {  0,        0,       0x5C,      0x5C },
/* 0x5D - Apps      */ {  0,        0,       0x5D,      0x5D }
};

//判断是否为按下的扫描码
static uint KeyType(byte sc)
{
    uint ret = KeyPress;

    if( sc & 0x80 )
    {
       ret = KeyRelease;
    }

    return ret;
}

static uint IsShift(byte sc)
{
    return (sc == 0x2A) || (sc == 0x36);
}

static uint IsCapsLock(byte sc)
{
    return (sc == 0x3A);
}

static uint IsNumLock(byte sc)
{
    return (sc == 0x45);
}

static uint PauseHandler(byte sc)
{
    uint ret = 0;

    return ret;
}

static uint MakeCode(KeyCode* pkc, int shift, int capsLock, int numLock, int E0)
{
    uint ret = 0;
    
    PrintChar('\n');
    PrintChar(pkc->ascii1);
    PrintChar(' ');
    PrintIntDec(shift);
    PrintChar(' ');
    PrintIntDec(capsLock);
    PrintChar(' ');
    PrintIntDec(numLock);
    PrintChar(' ');
    PrintIntDec(E0);
    PrintChar('\n');
    
    return ret;
}

static uint KeyHandler(byte sc)
{
    static int sShift = 0;
    static int cCapsLock = 0;
    static int cNumLock = 0;
    static int E0 = 0;

    uint ret = 0;

    //检测到e0 直接返回1
    if( sc == 0xE0 )
    {
       E0 = 1;
       ret = 1;
    }
    else
    {
       uint pressed = KeyType(sc);
       KeyCode* pkc = NULL;
    
    }
    
}


void PutScanCode(byte sc)
{
    //先判断是否是Pause键
    if( PauseHandler(sc) )
    {
        /* Pause Key */
    }
    else if( KeyHandler(sc) )
    {
        /* Normal Key */
    }
    else
    {
        /* Unknown Key */
    }

}

uint FetchKeyCode()
{
    uint ret = 0;
	
	return ret;
}
