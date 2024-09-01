#include <string.h>
#include "trs_iodefs.h"
#include "z80.h"


static char reverse_bits(char c) {
    char r = 0;
    for (int i = 0; i < 8; i++) {
        r = (r << 1) | (c & 1);
        c >>= 1;
    }
    return r;
}

static void reverse_bits_block(void *p, int len) {
    char *c = (char *) p;
    for (; len > 0; c++, len--) {
        *c = reverse_bits(*c);
    }
}


static char expand_3to6bit(char c, int bitoffset) {
    char res;
    int i;
    int bt;
    res = 0;

    for (i = 0; i < 6; i++) {
        bt = bitoffset + (i / 2);
        if (c & (1 << bt)) {
            res |= (1 << i);
        }
    }
    //joshlog("(%d) %02X -> %02X\n", bitoffset, c & 0xFF, res);
    return res;
}


static void expand_3to6bit_block(void *p, int bitoffset, int len) {
    char *c = (char *) p;
    for (; len > 0; c++, len--) {
        *c = expand_3to6bit(*c, bitoffset);
    }
}
static char expand_4to8bit(char c, int bitoffset) {
    char res;
    int i;
    int bt;
    res = 0;

    for (i = 0; i < 8; i++) {
        bt = bitoffset + (i / 2);
        if (c & (1 << bt)) {
            res |= (1 << i);
        }
    }
    //joshlog("(%d) %02X -> %02X\n", bitoffset, c & 0xFF, res);
    return res;
}
static void expand_4to8bit_block(void *p, int bitoffset, int len) {
    char *c = (char *) p;
    for (; len > 0; c++, len--) {
        *c = expand_4to8bit(*c, bitoffset);
    }
}

void init_pattern_table(trs_pattern_table *dest, const trs_charset_definition src_data, int flags) {
    joshlog("Init pattern table entry %p\n", dest);
    memset(dest, 0, sizeof(trs_pattern_table));
    if (flags & PATTERN_FLAG_8_PIXEL_CHARS) {
        dest->pixels_per_char = 8;
    } else {
        dest->pixels_per_char = 6;
    }
    memcpy(dest->normal, src_data, sizeof(dest->normal));
    reverse_bits_block(dest->normal, sizeof(dest->normal));
    //Todo to generate the model3/4 ENALTSET, we need to copy the
    // chars from 128 to 191 to 192 to 255.  When I get around to supporting
    // this, I think I will create 2 pattern tables and have the emulator
    // swap between then when enaltset is set.
    // This is probably also the right thing to do if I want to emulate GRAFIX-80
    //  The only addition I will need is the ability to change the grafix-80 pattern
    //  set on the fly.

    char grhigh = (char)(flags & PATTERN_FLAG_8_PIXEL_CHARS ? 0xF0 : 0xE0);
    char grlow = (char)(flags & PATTERN_FLAG_8_PIXEL_CHARS ? 0x0F : 0x1C);

    if (flags & PATTERN_FLAG_BLANK_HIGH_CHARS) {
        memset(dest->normal[128], 0, 128 * TRS_CHAR_HEIGHT);
    } else {
        for (int grindex = 0; grindex < 64; grindex++) {
            char scans[3] = {0, 0, 0};
            scans[0] |= (grindex & 1) ? grhigh : 0;
            scans[0] |= (grindex & 2) ? grlow : 0;
            scans[1] |= (grindex & 4) ? grhigh : 0;
            scans[1] |= (grindex & 8) ? grlow : 0;
            scans[2] |= (grindex & 16) ? grhigh : 0;
            scans[2] |= (grindex & 32) ? grlow : 0;
            for (int scanline = 0; scanline < TRS_CHAR_HEIGHT; scanline++) {
                int row = scanline / (TRS_CHAR_HEIGHT / 3);
                if (flags & PATTERN_FLAG_GRAPHICS_AT_128) {
                    dest->normal[128 + grindex][scanline] = scans[row];
                }
                if (flags & PATTERN_FLAG_GRAPHICS_AT_192) {
                    dest->normal[192 + grindex][scanline] = scans[row];
                }
            }
        }
    }
    if (flags & PATTERN_FLAG_BLANK_LOW_CHARS) {
        memset(dest->normal, 0, 128 * TRS_CHAR_HEIGHT);
    }

    memcpy(dest->wideleft, dest->normal, sizeof(dest->normal));
    reverse_bits_block(dest->wideleft, sizeof(dest->wideleft));
    if (dest->pixels_per_char == 6) {
        expand_3to6bit_block(dest->wideleft, 0, sizeof(dest->wideleft));
    } else {
        expand_4to8bit_block(dest->wideleft, 0, sizeof(dest->wideleft));
    }
    reverse_bits_block(dest->wideleft, sizeof(dest->wideleft));

    memcpy(dest->wideright, dest->normal, sizeof(dest->normal));
    reverse_bits_block(dest->wideright, sizeof(dest->wideright));
    if (dest->pixels_per_char == 6) {
        expand_3to6bit_block(dest->wideright, 3, sizeof(dest->wideright));
    } else {
        expand_4to8bit_block(dest->wideright, 4, sizeof(dest->wideright));
    }
    reverse_bits_block(dest->wideright, sizeof(dest->wideright));

}

void update_pattern_table(trs_pattern_table *dest, int charNum, int row, int bits) {
    if (charNum < 0 || charNum >= 256) {
        return;
    }
    if (row < 0 || row >= TRS_CHAR_HEIGHT) {
        return;
    }
    bits = dest->pixels_per_char == 8 ? (bits & 0xFF) : ((bits << 2) & 0xFF);
    char bits_rev = reverse_bits((char)bits);

    dest->normal[charNum][row] = (char)bits;
    if (dest->pixels_per_char == 6) {
        dest->wideleft[charNum][row] = reverse_bits(expand_3to6bit(bits_rev, 0));
        dest->wideright[charNum][row] = reverse_bits(expand_3to6bit(bits_rev, 3));
    } else {
        dest->wideleft[charNum][row] = reverse_bits(expand_4to8bit(bits_rev, 0));
        dest->wideright[charNum][row] = reverse_bits(expand_4to8bit(bits_rev, 4));
    }

}