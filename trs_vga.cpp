extern "C" {
#include "trs.h"
}

#include <cstring>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include "trs_vga.h"
#include "trs_iodefs.h"




/** Returns the usable remaining space in the djgpp transfer buffer.
 * (Not sure we ever need to transfer memory for pci bios calls, but
 * we can use the transfer buffer if needed.  The ds, and es segment
 * registers are set so offset 0 is the start of the usable buffer.
 *
 * If more space is needed, use djgpp/dpmi calls to allocate our own
 * dos memory buffer.
 *
 * This should be used with "__dpmi_simulate_real_mode_interrupt" because
 * we manage the real-mode stack.
 * */
static unsigned long prepare_bios_regs(__dpmi_regs *regs) {
    memset(regs, 0, sizeof(__dpmi_regs));
    regs->x.ss = __tb >> 4;     /* nearest transfer buffer segment  */
    //Set SP.  Subtract 8 because I can never remember if top of stack is where
    // the next pushed value goes.  Round down to 16 bit alignment.    Note that
    // we also may be up to 15 bytes from the top of the transfer buffer because
    // it may not start on a real-mode segment boundary
    regs->x.sp = (_go32_info_block.size_of_transfer_buffer - 8) & 0xFFF0;

    //In case we need other dos memory, set ds and es to the transfer buffer.
    //This time we add 1 to the segments to ensure that offset 0 is in the buffer.
    regs->x.es = regs->x.ss + 1;
    regs->x.ds = regs->x.ss + 1;

    //Redundant, but keeping it here because _dpmi says we should do this or else
    //must set a valid? flags register...
    regs->x.flags = 0;

    //Return the usable space starting at offset 0 from the ds/es sergments.
    //We need to reserve 1K of stack (from the dpmi spec).  Also
    //subtract 16 because the ds/es are 1 above ss.  Finally subtract another
    //16 in case I did something wrong...
    return regs->x.sp - 1024 - 32;
}

static void setCGAHighRes() {
    __dpmi_regs regs;
    prepare_bios_regs(&regs);
    regs.h.ah  = 0;
    regs.h.al = 6;  //640x200 - 1 bit
    __dpmi_simulate_real_mode_interrupt(0x10, &regs);
}
static void setVGAHighRes() {
    __dpmi_regs regs;
    prepare_bios_regs(&regs);
    regs.h.ah  = 0;
    regs.h.al = 0x11;  //640x480 - 1 bit
    __dpmi_simulate_real_mode_interrupt(0x10, &regs);
}



//This was copied from trs_djppp - maybe need to make a common function but
//now worrying about that for now
static void rotate_left_block(void *p, int steps, int len) {
    char *c = (char *) p;
    //Using inline assembly, so we can do a single op-code rotation.
    //I think it may be worth it because we are rotating on the fly
    //in order to get our 6 pixel character at the proper 8 bit alignment).
    //Typically, we do this for a single character cell of height,
    //so we could maybe improve this by unrolling the loop.
    for (; len > 0; c++, len--) {
        //*c = rotate_left(*c, steps);
        asm ( "rolb %%cl, (%0)"
                : /* no output registers */
                : "r" (c), "c" (steps)
                : "cc"
                );
    }
}

#define VGA_INIT_MODE_NONE 0
#define VGA_INIT_MODE_6 1
#define VGA_INIT_MODE_17 2


static int init = VGA_INIT_MODE_NONE;

static unsigned char shadowbuf[0x10000];
static unsigned char shadowbuf_hires[0x10000];

static const int trsmode_normal = 0;
static const int trsmode_hires = 1;

static unsigned char trsmode = 0; // 0 for normal, 1 for hires, 2 for both (2 not implemented yet)

void vga_needs_reset() {
    init = VGA_INIT_MODE_NONE;
}

static const unsigned char maskl[] = {0xFC,0x3,0xF,0x3F};
static const unsigned char maskr[] = {0,0xF0,0xC0,0};


inline static void chk_init_6() {
    if (init != VGA_INIT_MODE_6) {
        setCGAHighRes();
        memset(shadowbuf, 0, sizeof(shadowbuf));
        memset(shadowbuf_hires, 0, sizeof(shadowbuf_hires));
        init = VGA_INIT_MODE_6;
    }
}
inline static void chk_init_17() {
    if (init != VGA_INIT_MODE_17) {
        setVGAHighRes();
        memset(shadowbuf, 0, sizeof(shadowbuf));
        memset(shadowbuf_hires, 0, sizeof(shadowbuf_hires));
        init = VGA_INIT_MODE_17;
    }
}


static void redraw() {
    const int mx17 = 80 * 480;
    const int mx6 = 80 * 100;

    _farsetsel(_dos_ds);
    if (trsmode == trsmode_normal) {
        if (init == VGA_INIT_MODE_17) {
            for (int i = 0; i < mx17; i++) {
                _farnspokeb(0xA0000 + i, shadowbuf[i]);
            }
        } else if (init == VGA_INIT_MODE_6) {
            for (int i = 0; i < mx6; i++) {
                _farnspokeb(0xB8000 + i, shadowbuf[i]);
                _farnspokeb(0xBA000 + i, shadowbuf[i + 0x2000]);
            }
        }
    } else if (trsmode == trsmode_hires) {
        if (init == VGA_INIT_MODE_17) {
            for (int i = 0; i < mx17; i++) {
                _farnspokeb(0xA0000 + i, shadowbuf_hires[i]);
            }
        } else if (init == VGA_INIT_MODE_6) {
            for (int i = 0; i < mx6; i++) {
                _farnspokeb(0xB8000 + i, shadowbuf_hires[i]);
                _farnspokeb(0xBA000 + i, shadowbuf_hires[i + 0x2000]);
            }
        }
    }

}

void vga_set_hires() {
    if (trsmode != trsmode_hires) {
        trsmode = trsmode_hires;
        redraw();
    }
}
void vga_set_text() {
    if (trsmode != trsmode_normal) {
        trsmode = trsmode_normal;
        redraw();
    }
}


void vga_screen_write_glyph_64_16(char *glyphRows, int position) {

    chk_init_6();

    char patData[TRS_CHAR_HEIGHT];
    memcpy(patData, glyphRows, TRS_CHAR_HEIGHT);
    rotate_left_block(patData, (position & 3) << 1, TRS_CHAR_HEIGHT);

    unsigned char ml = maskl[position & 3];
    unsigned char mr = maskr[position & 3];

    int x, y;

    x = (position & 63);
    y = (position >> 6) & 0xF;
    int px, py;
    px = x * 6 + 120;   //offset (640-384)/2 then round down to a multiple of 24 so rotates work correctly
    py = y * TRS_CHAR_HEIGHT + 0;  //offset (200-192)/2 then round down to a multiple of 12 so patterns line up

    int offset = ((py>>1) * 80);

    _farsetsel(_dos_ds);
    offset += (px >> 3);
    for(int r = 0; r < TRS_CHAR_HEIGHT; r++) {
        unsigned char curb = shadowbuf[offset];
        unsigned char newb = (patData[r] & ml) | (curb & ~ml);
        if (curb != newb) {
            if (trsmode == trsmode_normal) {
                _farnspokeb(0xB8000 + offset, newb);
            }
            shadowbuf[offset] = newb;
        }
        if (mr) {
            curb = shadowbuf[offset + 1];
            newb = (patData[r] & mr) | (curb & ~mr);
            if (curb != newb) {
                if (trsmode == trsmode_normal) {
                    _farnspokeb(0xB8000 + offset + 1, newb);
                }
                shadowbuf[offset+1] = newb;
            }
        }
        if (r & 1) offset = offset - 0x2000 + 80;
        else offset += 0x2000;
    }

}

void vga_screen_write_glyph_64_16_8ppc(char *glyphRows, int position) {
    chk_init_6();

    int x, y;

    x = (position & 63);
    y = (position >> 6) & 0xF;
    int px, py;
    px = x * 8 + 64;   //offset (640-64*8)/2
    py = y * TRS_CHAR_HEIGHT + 0;  //offset (200-192)/2 then round down to a multiple of 12 so patterns line up

    int offset = ((py>>1) * 80);

    _farsetsel(_dos_ds);
    offset += (px >> 3);
    for(int r = 0; r < TRS_CHAR_HEIGHT; r++) {
        unsigned char curb = shadowbuf[offset];
        unsigned char newb = glyphRows[r];
        if (curb != newb) {
            if (trsmode == trsmode_normal) {
                _farnspokeb(0xB8000 + offset, newb);
            }
            shadowbuf[offset] = newb;
        }
        //Interleaved planes
        if (r & 1) offset = offset - 0x2000 + 80;
        else offset += 0x2000;
    }
}

void vga_screen_write_glyph_80_24_8ppc(char *glyphRows, int position) {
    chk_init_17(); //640 x 480

    int x, y;
    position = position & 0xFFFF;  //Mostly to ensure signed

    x = (position % 80);
    y = (position / 80) % 24;

    // Note that model 4 only uses 10 vertical pixels per character (which is why the block graphics
    // looked funny in model 4 mode and probably is why they didn't provide set/reset functions in model 4 basic

    int px, py;
    px = x * 8;   //No offset because 8 * 80 = 640
    py = y * TRS_CHAR_HEIGHT4;  //No offset - we will do 2 rows per trs-80 row which gives us 24 * 10 * 2 = 480

    int offset = py * 2 * 80; // 2 rows per trs-80 row

    _farsetsel(_dos_ds);
    offset += (px >> 3);
    for(int r = 0; r < TRS_CHAR_HEIGHT4; r++) {
        unsigned const char newb = glyphRows[r];
        unsigned char curb;

        // 2 rows
        curb = shadowbuf[offset];
        if (curb != newb) {
            if (trsmode == trsmode_normal) {
                _farnspokeb(0xA0000 + offset, newb);
            }
            shadowbuf[offset] = newb;
        }
        offset += 80;

        curb = shadowbuf[offset];
        if (curb != newb) {
            if (trsmode == trsmode_normal) {
                _farnspokeb(0xA0000 + offset, newb);
            }
            shadowbuf[offset] = newb;
        }
        offset += 80;
    }

}



void vga_screen_scroll_64_16() {
    chk_init_6();

    const int shift = 80 * TRS_CHAR_HEIGHT / 2;
    const int copySize = 80 * 100  - shift;

    memmove(shadowbuf, shadowbuf + shift, copySize);
    memmove(shadowbuf + 0x2000, shadowbuf + 0x2000 + shift, copySize);

    if (trsmode == trsmode_normal) {
        _farsetsel(_dos_ds);
        for (int i = 0; i < copySize; i += 4) {
            _farnspokel(0xB8000 + i, *(reinterpret_cast<unsigned long *>(shadowbuf + i)));
            _farnspokel(0xB8000 + i + 0x2000, *(reinterpret_cast<unsigned long *>(shadowbuf + i + 0x2000)));
        }
    }

}


/** HRG Notes
 *
 * So...I think we need to rethink the split between the screen related entry points in trs_djgpp and the
 * ones here.   To properly support all of the HRG options, I feel like we may need to "know" the various
 * mode bits, etc., kept in trs_djgpp (originally trs_xinterface)
 *
 * The "holy grail" would be to extract out an emulator front-end interface that we can plug in to whatever
 * system we are compiling for.
 *
 * For now, I will try a first-pass support that only works in model 4 mode and does not handle overlays.
 * That should suffice to get the BASICG code running
 ***/
void vga_hires_set(int x, int y, unsigned char data) {
    if (init == VGA_INIT_MODE_6) {
        // TODO - Mod 3 support...Maybe even Mod 1 support?
        return;
    }
    x &= 0x7F;
    y &= 0xFF;
    //Note - I should probably change this so that the shadow_hires mimics the layout of the trs-80
    // screen memory (256 rows of 128 bytes) and then translate to VGA coordinates when copying the
    // shadow to main hires.  This would let me implement the undocumented scrolling feature of the hires
    // card.   For now, I'll just limit the size
    if (x > 79) {
        x = 79;
    }
    if (y > 239) {
        y = 239;
    }
    _farsetsel(_dos_ds);
    int offset = (2 * y * 80) + x;
    if (shadowbuf_hires[offset] != data) {
        shadowbuf_hires[offset] = data;
        if (trsmode == trsmode_hires) {
            _farnspokeb(0xA0000 + offset, data);
        }
    }
    offset += 80;
    if (shadowbuf_hires[offset] != data) {
        shadowbuf_hires[offset] = data;
        if (trsmode == trsmode_hires) {
            _farnspokeb(0xA0000 + offset, data);
        }
    }
}

unsigned char vga_hires_get(int x, int y) {
    if (init == VGA_INIT_MODE_6) {
        // TODO - Mod 3 support...Maybe even Mod 1 support?
        return 0;
    }
    x &= 0x7F;
    y &= 0xFF;
    //Note - I should probably change this so that the shadow_hires mimics the layout of the trs-80
    // screen memory (256 rows of 128 bytes) and then translate to VGA coordinates when copying the
    // shadow to main hires.  This would let me implement the undocumented scrolling feature of the hires
    // card.   For now, I'll just limit the size
    if (x > 79) {
        x = 79;
    }
    if (y > 239) {
        y = 239;
    }
    const int offset = (2 * y * 80) + x;
    return shadowbuf_hires[offset];
}


