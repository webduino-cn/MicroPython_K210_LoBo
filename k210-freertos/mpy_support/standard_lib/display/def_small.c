// This comes with no warranty, implied or otherwise

// This data structure was designed to support Proportional fonts
// on Arduinos. It can however handle any ttf font that has been converted
// using the conversion program. These could be fixed width or proportional 
// fonts. Individual characters do not have to be multiples of 8 bits wide. 
// Any width is fine and does not need to be fixed.

// The data bits are packed to minimize data requirements, but the tradeoff
// is that a header is required per character.

// def_small.c
// Point Size   : 9
// Memory usage : 928 bytes
// # characters : 95

// Header Format (to make Arduino UTFT Compatible):
// ------------------------------------------------
// Character Width (Used as a marker to indicate use this format. i.e.: = 0x00)
// Character Height
// First Character (Reserved. 0x00)
// Number Of Characters (Reserved. 0x00)


#include "mpconfigport.h"

#ifdef MICROPY_USE_DISPLAY

const unsigned char tft_def_small[] = 
{
0x00, 0x08, 0x00, 0x00,

// Individual Character Format:
// ----------------------------
// Character Code
// Adjusted Y Offset
// Width
// Height
// xOffset
// xDelta (the distance to move the cursor. Effective width of the character.)
// Data[n]

// NOTE: You can remove any of these characters if they are not needed in
// your application. The first character number in each Glyph indicates
// the ASCII character code. Therefore, these do not have to be sequential.
// Just remove all the content for a particular character to save space.

// ' '
0x20,0x08,0x00,0x00,0x00,0x03,

// '!'
0x21,0x01,0x01,0x07,0x01,0x03,
0xFA,
// '"'
0x22,0x01,0x03,0x02,0x01,0x04,
0xB4,
// '#'
0x23,0x01,0x06,0x07,0x01,0x08,
0x28,0xAF,0xCA,0xFD,0x45,0x00,
// '$'
0x24,0x01,0x06,0x08,0x00,0x06,
0x21,0xEA,0x38,0x38,0xAF,0x08,
// '%'
0x25,0x01,0x08,0x07,0x00,0x09,
0x44,0xA4,0xA8,0x5A,0x15,0x25,0x22,
// '&'
0x26,0x01,0x06,0x07,0x01,0x08,
0x31,0x04,0x19,0x9E,0x66,0xC0,
// '''
0x27,0x01,0x01,0x02,0x01,0x02,
0xC0,
// '('
0x28,0x00,0x02,0x08,0x01,0x04,
0x4A,0xA1,
// ')'
0x29,0x00,0x02,0x08,0x01,0x04,
0x85,0x52,
// '*'
0x2A,0x01,0x05,0x04,0x00,0x05,
0xAB,0x9D,0x50,
// '+'
0x2B,0x03,0x05,0x05,0x01,0x08,
0x21,0x3E,0x42,0x00,
// ','
0x2C,0x07,0x01,0x02,0x01,0x03,
0xC0,
// '-'
0x2D,0x05,0x02,0x01,0x01,0x03,
0xC0,
// '.'
0x2E,0x07,0x01,0x01,0x01,0x03,
0x80,
// '/'
0x2F,0x01,0x03,0x07,0x00,0x03,
0x25,0x25,0x20,
// '0'
0x30,0x01,0x04,0x07,0x01,0x06,
0x69,0x99,0x99,0x60,
// '1'
0x31,0x01,0x03,0x07,0x02,0x06,
0xC9,0x24,0xB8,
// '2'
0x32,0x01,0x05,0x07,0x01,0x06,
0x64,0x84,0x44,0x43,0xC0,
// '3'
0x33,0x01,0x04,0x07,0x01,0x06,
0x69,0x16,0x11,0x60,
// '4'
0x34,0x01,0x05,0x07,0x01,0x06,
0x11,0x94,0xA9,0x7C,0x40,
// '5'
0x35,0x01,0x04,0x07,0x01,0x06,
0xF8,0x8E,0x11,0xE0,
// '6'
0x36,0x01,0x04,0x07,0x01,0x06,
0x7C,0x8E,0x99,0x60,
// '7'
0x37,0x01,0x04,0x07,0x01,0x06,
0xF1,0x22,0x24,0x40,
// '8'
0x38,0x01,0x04,0x07,0x01,0x06,
0x69,0x96,0x99,0x60,
// '9'
0x39,0x01,0x04,0x07,0x01,0x06,
0x69,0x97,0x13,0xE0,
// ':'
0x3A,0x03,0x01,0x05,0x01,0x03,
0x88,
// ';'
0x3B,0x03,0x01,0x06,0x01,0x03,
0x8C,
// '<'
0x3C,0x03,0x06,0x05,0x01,0x08,
0x04,0xEE,0x0E,0x04,
// '='
0x3D,0x04,0x06,0x03,0x01,0x08,
0xFC,0x0F,0xC0,
// '>'
0x3E,0x03,0x06,0x05,0x01,0x08,
0x81,0xC1,0xDC,0x80,
// '?'
0x3F,0x01,0x04,0x07,0x01,0x05,
0xE1,0x24,0x40,0x40,
// '@'
0x40,0x01,0x08,0x08,0x01,0x0A,
0x3C,0x42,0x9D,0xA5,0xA5,0x9E,0x40,0x38,
// 'A'
0x41,0x01,0x06,0x07,0x00,0x06,
0x30,0xC4,0x92,0x7A,0x18,0x40,
// 'B'
0x42,0x01,0x05,0x07,0x01,0x07,
0xF4,0x63,0xE8,0xC7,0xC0,
// 'C'
0x43,0x01,0x05,0x07,0x01,0x07,
0x72,0x61,0x08,0x25,0xC0,
// 'D'
0x44,0x01,0x05,0x07,0x01,0x07,
0xF4,0xE3,0x18,0xCF,0xC0,
// 'E'
0x45,0x01,0x04,0x07,0x01,0x06,
0xF8,0x8F,0x88,0xF0,
// 'F'
0x46,0x01,0x04,0x07,0x01,0x06,
0xF8,0x8F,0x88,0x80,
// 'G'
0x47,0x01,0x05,0x07,0x01,0x07,
0x76,0x61,0x38,0xE5,0xC0,
// 'H'
0x48,0x01,0x05,0x07,0x01,0x07,
0x8C,0x63,0xF8,0xC6,0x20,
// 'I'
0x49,0x01,0x01,0x07,0x01,0x03,
0xFE,
// 'J'
0x4A,0x01,0x02,0x09,0x00,0x03,
0x55,0x55,0x80,
// 'K'
0x4B,0x01,0x05,0x07,0x01,0x06,
0x8C,0xA9,0x8A,0x4A,0x20,
// 'L'
0x4C,0x01,0x04,0x07,0x01,0x05,
0x88,0x88,0x88,0xF0,
// 'M'
0x4D,0x01,0x06,0x07,0x01,0x08,
0x87,0x3C,0xED,0xB6,0x18,0x40,
// 'N'
0x4E,0x01,0x05,0x07,0x01,0x07,
0x8E,0x73,0x59,0xCE,0x20,
// 'O'
0x4F,0x01,0x05,0x07,0x01,0x07,
0x76,0xE3,0x18,0xED,0xC0,
// 'P'
0x50,0x01,0x04,0x07,0x01,0x06,
0xE9,0x9E,0x88,0x80,
// 'Q'
0x51,0x01,0x05,0x08,0x01,0x07,
0x76,0xE3,0x18,0xE9,0xC2,
// 'R'
0x52,0x01,0x05,0x07,0x01,0x06,
0xE4,0xA5,0xCA,0x4A,0x20,
// 'S'
0x53,0x01,0x06,0x07,0x01,0x07,
0x72,0x28,0x1C,0x0A,0x27,0x00,
// 'T'
0x54,0x01,0x05,0x07,0x00,0x05,
0xF9,0x08,0x42,0x10,0x80,
// 'U'
0x55,0x01,0x05,0x07,0x01,0x07,
0x8C,0x63,0x18,0xC5,0xC0,
// 'V'
0x56,0x01,0x06,0x07,0x00,0x06,
0x86,0x14,0x92,0x48,0xC3,0x00,
// 'W'
0x57,0x01,0x09,0x07,0xFF,0x07,
0x49,0x24,0x8A,0x85,0x43,0xE0,0xA0,0x50,
// 'X'
0x58,0x01,0x06,0x07,0x00,0x06,
0xCD,0x23,0x0C,0x31,0x28,0xC0,
// 'Y'
0x59,0x01,0x05,0x07,0x00,0x05,
0x8A,0x9C,0x42,0x10,0x80,
// 'Z'
0x5A,0x01,0x05,0x07,0x00,0x05,
0xF8,0x44,0x44,0x43,0xE0,
// '['
0x5B,0x01,0x02,0x08,0x01,0x04,
0xEA,0xAB,
// '\'
0x5C,0x01,0x03,0x07,0x00,0x03,
0x91,0x24,0x48,
// ']'
0x5D,0x01,0x02,0x08,0x01,0x04,
0xD5,0x57,
// '^'
0x5E,0x01,0x06,0x02,0x01,0x08,
0x31,0x20,
// '_'
0x5F,0x09,0x05,0x01,0x00,0x05,
0xF8,
// '`'
0x60,0x00,0x02,0x02,0x01,0x05,
0x90,
// 'a'
0x61,0x03,0x04,0x05,0x01,0x06,
0x61,0x79,0xF0,
// 'b'
0x62,0x00,0x04,0x08,0x01,0x06,
0x88,0x8E,0x99,0x9E,
// 'c'
0x63,0x03,0x04,0x05,0x01,0x06,
0x78,0x88,0x70,
// 'd'
0x64,0x00,0x04,0x08,0x01,0x06,
0x11,0x17,0x99,0x97,
// 'e'
0x65,0x03,0x04,0x05,0x01,0x06,
0x69,0xF8,0x70,
// 'f'
0x66,0x00,0x04,0x08,0x00,0x03,
0x34,0x4E,0x44,0x44,
// 'g'
0x67,0x03,0x04,0x07,0x01,0x06,
0x79,0x99,0x71,0x60,
// 'h'
0x68,0x00,0x04,0x08,0x01,0x06,
0x88,0x8E,0x99,0x99,
// 'i'
0x69,0x01,0x01,0x07,0x01,0x03,
0xBE,
// 'j'
0x6A,0x01,0x02,0x09,0x00,0x03,
0x45,0x55,0x80,
// 'k'
0x6B,0x00,0x04,0x08,0x01,0x05,
0x88,0x89,0xAC,0xA9,
// 'l'
0x6C,0x00,0x01,0x08,0x01,0x03,
0xFF,
// 'm'
0x6D,0x03,0x07,0x05,0x01,0x09,
0xED,0x26,0x4C,0x99,0x20,
// 'n'
0x6E,0x03,0x04,0x05,0x01,0x06,
0xE9,0x99,0x90,
// 'o'
0x6F,0x03,0x04,0x05,0x01,0x06,
0x69,0x99,0x60,
// 'p'
0x70,0x03,0x04,0x07,0x01,0x06,
0xE9,0x99,0xE8,0x80,
// 'q'
0x71,0x03,0x04,0x07,0x01,0x06,
0x79,0x99,0x71,0x10,
// 'r'
0x72,0x03,0x03,0x05,0x01,0x04,
0xF2,0x48,
// 's'
0x73,0x03,0x04,0x05,0x01,0x05,
0x68,0x62,0xE0,
// 't'
0x74,0x02,0x04,0x06,0x00,0x04,
0x4F,0x44,0x47,
// 'u'
0x75,0x03,0x04,0x05,0x01,0x06,
0x99,0x99,0x70,
// 'v'
0x76,0x03,0x07,0x05,0xFF,0x05,
0x44,0x98,0xA1,0xC1,0x00,
// 'w'
0x77,0x03,0x07,0x05,0x00,0x07,
0x93,0x76,0xBA,0x24,0x40,
// 'x'
0x78,0x03,0x05,0x05,0x00,0x05,
0x8A,0x88,0xA8,0x80,
// 'y'
0x79,0x03,0x07,0x07,0xFF,0x05,
0x44,0x88,0xA1,0xC1,0x02,0x18,0x00,
// 'z'
0x7A,0x03,0x04,0x05,0x01,0x06,
0xF1,0x24,0xF0,
// '{'
0x7B,0x01,0x03,0x08,0x01,0x05,
0x69,0x64,0x93,
// '|'
0x7C,0x01,0x01,0x09,0x01,0x03,
0xFF,0x80,
// '}'
0x7D,0x01,0x03,0x08,0x01,0x05,
0xC9,0x34,0x96,
// '~'
0x7E,0x03,0x06,0x03,0x01,0x08,
0x01,0x91,0x80,

// Terminator
0x00
};

#endif