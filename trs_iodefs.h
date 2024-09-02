/*
 * Copyright (C) 1992 Clarendon Hill Software.
 *
 * Permission is granted to any individual or institution to use, copy,
 * or redistribute this software, provided this copyright notice is retained. 
 *
 * This software is provided "as is" without any expressed or implied
 * warranty.  If this software brings on any sort of damage -- physical,
 * monetary, emotional, or brain -- too bad.  You've got no one to blame
 * but yourself. 
 *
 * The software may be modified for your own purposes, but modified versions
 * must retain this notice.
 */

/*
   Modified by Timothy Mann, 1996 and later
   $Id$
*/

#define MAXCHARS 256
#define TRS_CHAR_WIDTH 8
#define TRS_CHAR_HEIGHT 12
#define TRS_CHAR_HEIGHT4 10

extern char trs_char_data[][MAXCHARS][TRS_CHAR_HEIGHT];

#define CHARSET_MODEL_1_OLD 0
#define CHARSET_MODEL_1_STANDARD 1
#define CHARSET_MODEL_1_LOWER 2
#define CHARSET_MODEL_1_PETROFSKY 3
#define CHARSET_MODEL_3_ORIGINAL 4
#define CHARSET_MODEL_3_M4ERA 5
#define CHARSET_MODEL_3_REPLACE 6
#define CHARSET_MODEL_4_KATANA 7
#define CHARSET_MODEL_4_STANDARD 8
#define CHARSET_MODEL_4_REPLACE 9
#define CHARSET_MODEL_4_GERMAN 10

#define CHARSET_MIN 0
#define CHARSET_MAX 10

// Above this number, all chars are 8 pixels wide
#define CHARSET_MIN_8_PIXEL 3

typedef struct trs_pattern_table {
    char normal[MAXCHARS][TRS_CHAR_HEIGHT];
    char wideleft[MAXCHARS][TRS_CHAR_HEIGHT];
    char wideright[MAXCHARS][TRS_CHAR_HEIGHT];
    int pixels_per_char; // 6 or 8
} trs_pattern_table;

#define PATTERN_FLAG_GRAPHICS_AT_128  1
#define PATTERN_FLAG_GRAPHICS_AT_192  2
#define PATTERN_FLAG_8_PIXEL_CHARS    4
#define PATTERN_FLAG_BLANK_HIGH_CHARS 8
#define PATTERN_FLAG_BLANK_LOW_CHARS 16

typedef char trs_charset_definition[MAXCHARS][TRS_CHAR_HEIGHT];

//TODO - get the src_data type correct
void init_pattern_table(trs_pattern_table *dest, const trs_charset_definition, int flags);
void update_pattern_table(trs_pattern_table *dest, int charNum, int row, int bits);

// No real reason this should be here except sometimes it's handy for io/memory handlers
char reverse_bits_char(char c);

