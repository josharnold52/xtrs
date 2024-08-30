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

void init_pattern_table(trs_pattern_table *dest, const char **src_data, int model) {
    joshlog("Init pattern table entry %p\n", dest);
    //TODO
    memset(dest, 0, sizeof(trs_pattern_table));
    memcpy(dest->normal, src_data, sizeof(dest->normal));
    reverse_bits_block(dest->normal, sizeof(dest->normal));
    //Todo to generate the model3/4 ENALTSET, we need to copy the
    // chars from 128 to 191 to 192 to 255.  When I get around to supporting
    // this, I think I will create 2 pattern tables and have the emulator
    // swap between then when enaltset is set.
    // This is probably also the right thing to do if I want to emulate GRAFIX-80
    //  The only addition I will need is the ability to change the grafix-80 pattern
    //  set on the fly.

    for (int grindex = 0; grindex < 64; grindex++) {
        char scans[3] = {0, 0, 0};
        scans[0] |= (grindex & 1) ? 0xE0 : 0;
        scans[0] |= (grindex & 2) ? 0x1C : 0;
        scans[1] |= (grindex & 4) ? 0xE0 : 0;
        scans[1] |= (grindex & 8) ? 0x1C : 0;
        scans[2] |= (grindex & 16) ? 0xE0 : 0;
        scans[2] |= (grindex & 32) ? 0x1C : 0;
        for (int scanline = 0; scanline < TRS_CHAR_HEIGHT; scanline++) {
            int row = scanline / (TRS_CHAR_HEIGHT / 3);
            dest->normal[128 + grindex][scanline] = scans[row];
            if (model == 1) {
                //Models 3 and 4 display extended chars instead of repeating the graphic chars
                dest->normal[192 + grindex][scanline] = scans[row];
            }
        }
    }

    memcpy(dest->wideleft, dest->normal, sizeof(dest->normal));
    reverse_bits_block(dest->wideleft, sizeof(dest->wideleft));
    if (model == 1) {
        expand_3to6bit_block(dest->wideleft, 0, sizeof(dest->wideleft));
    } else {
        expand_4to8bit_block(dest->wideleft, 0, sizeof(dest->wideleft));
    }
    reverse_bits_block(dest->wideleft, sizeof(dest->wideleft));

    memcpy(dest->wideright, dest->normal, sizeof(dest->normal));
    reverse_bits_block(dest->wideright, sizeof(dest->wideright));
    if (model == 1) {
        expand_3to6bit_block(dest->wideright, 3, sizeof(dest->wideright));
    } else {
        expand_4to8bit_block(dest->wideright, 3, sizeof(dest->wideright));
    }
    reverse_bits_block(dest->wideright, sizeof(dest->wideright));

    return;
}