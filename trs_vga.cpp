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
 * This should be used "__dpmi_simulate_real_mode_interrupt" because
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
    regs.h.al = 6;
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

static int init = 0;

static unsigned char shadowbuf[0x4000];

void vga_needs_reset() {
    init = 0;
}

static const unsigned char maskl[] = {0xFC,0x3,0xF,0x3F};
static const unsigned char maskr[] = {0,0xF0,0xC0,0};

void vga_screen_write_glyph_64_16(char *glyphRows, int position) {


    if (!init) {
        setCGAHighRes();
        memset(shadowbuf, 0, sizeof(shadowbuf));
        init = 1;
    }

    char patData[TRS_CHAR_HEIGHT];
    memcpy(patData, glyphRows, TRS_CHAR_HEIGHT);
    rotate_left_block(patData, (position & 3) << 1, TRS_CHAR_HEIGHT);

    /*
   GrPattern pat;
   pat.gp_bitmap.bmp_ispixmap = 0;
   pat.gp_bitmap.bmp_height = TRS_CHAR_HEIGHT;
   pat.gp_bitmap.bmp_data = patData;
   pat.gp_bitmap.bmp_fgcolor = GrWhite();
   pat.gp_bitmap.bmp_bgcolor = GrBlack();
   pat.gp_bitmap.bmp_memflags = 0;
   */
    //trs_screen_pattern.gp_bitmap.bmp_data = patData;
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
            _farnspokeb( 0xB8000 + offset, newb);
            shadowbuf[offset] = newb;
        }
        if (mr) {
            curb = shadowbuf[offset + 1];
            newb = (patData[r] & mr) | (curb & ~mr);
            if (curb != newb) {
                _farnspokeb(0xB8000 + offset + 1, newb);
                shadowbuf[offset+1] = newb;
            }
        }
        if (r & 1) offset = offset - 0x2000 + 80;
        else offset += 0x2000;
    }

}

void vga_screen_scroll_64_16() {
    if (!init) {
        setCGAHighRes();
        memset(shadowbuf, 0, sizeof(shadowbuf));
        init = 1;
    }
    const int shift = 80 * TRS_CHAR_HEIGHT / 2;
    const int copySize = 80 * 100  - shift;

    memmove(shadowbuf, shadowbuf + shift, copySize);
    memmove(shadowbuf + 0x2000, shadowbuf + 0x2000 + shift, copySize);

    _farsetsel(_dos_ds);
    for(int i=0;i<copySize;i+=4) {
        _farnspokel(0xB8000 + i, *(reinterpret_cast<unsigned long *>(shadowbuf + i)));
        _farnspokel(0xB8000 + i + 0x2000, *(reinterpret_cast<unsigned long *>(shadowbuf + i + 0x2000)));
    }



}
