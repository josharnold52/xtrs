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

/*#define MOUSEDEBUG 1*/
/*#define XDEBUG 1*/
/*#define QDEBUG 1*/

/*
 * trs_xinterface.c
 *
 * X Windows interface for TRS-80 simulator
 */

#define _DEFAULT_SOURCE /* string.h: strcasecmp() */
#define _XOPEN_SOURCE 500 /* string.h: strdup() */

#include <stdio.h>
#include <fcntl.h>
//#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <grx20.h>
#include <grxkeys.h>


#include "trs_iodefs.h"
#include "trs.h"
#include "z80.h"
#include "trs_disk.h"
#include "trs_uart.h"
#include "trs_imp_exp.h"
#include "keytrap/scanbuf.h"

#include <dpmi.h>

#include "trs_djgpp.h"
#include "trs_vga.h"

GrColor COLOR_BORDER;
GrColor COLOR_PRIMARY;
GrColor COLOR_PRIMARY_DIM;
GrColor COLOR_SECONDARY;
GrColor COLOR_DISABLED;
GrColor COLOR_SECONDARY_BRIGHT;
GrColor COLOR_TERTIARY;

int trs_model1_lowercase = 0;
int trs_model1_grafix80 = 0;

const char *emulator_base_directory = 0;
const char *cassette_base_directory = 0;
const char *emulator_printer_directory =0;

/**
 * This gets initialized during screen init, and the
 * pat data is updated each time we write a glyph
 */
static GrPattern trs_screen_pattern;

// Private data
static unsigned char trs_screen[2048];

//Note that these get changed when we switch to/from 80x24 mode!
static int screen_chars = 1024;
static int row_chars = 64;


static int scale_x = 1;
static int scale_y = 0;
static int resize = -1;
static int grafyx_microlabs = 0;
static int border_width = 2;
static int cur_char_height, cur_char_width;
static int trs_charset;

static struct scan_buffer *pScanBuffer = 0;
static unsigned char scanBufferCursor;

//static char pattern_table_1[MAXCHARS][TRS_CHAR_HEIGHT];

//static char pattern_table_1_wideleft[MAXCHARS][TRS_CHAR_HEIGHT];
//static char pattern_table_1_wideright[MAXCHARS][TRS_CHAR_HEIGHT];

static trs_pattern_table *p_current_table;

static trs_pattern_table primary_pattern_table;
static trs_pattern_table grafix80_table; //TODO
static trs_pattern_table grafix80_programming_table; //TODO

#define NORMAL 0
#define EXPANDED 1
#define INVERSE 2
#define ALTERNATE 4
#define GR80_PROGRAM 8
#define GR80_ENABLE 16
static int trs_current_video_mode = 0;

static void trs_load_romfile();

#define VIDEO_DRIVER_VGA 1



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


static void not_implemented(const char *msg, int *counter) {
    int cnt;
    cnt = counter ? (*counter) : -1;
    if (cnt < 3) {
        joshlog("Not implemented: %s %d\n", msg, cnt);
        cnt++;
        if (counter) {
            *counter = cnt;
        }
    }

    //exit(100);
}


//TODO - Move to h file
extern void trs_xlate_pc_scancode(unsigned char scan_code, int shifted);


void trs_get_event(int wait) {
    //Argument is ignored!  (In old xtrs it caused us to sleep for a bit if no events)

    static int nest_count = 0;

    //TODO: I think there's a bug here (or in the trs_xlate_pc_scancode code that goes with it)
    // If shifted and unshifted IBM key maps to different TRS keys, and if shift is released
    // before IBM key, it may be that we send the incorrect key-up.   This causes the Level 1
    // keyboard driver (and maybe others) to hang because it loops waiting for a keyup that it
    // never sees.   Perhaps I need to keep track of whether shift is forced up or down when doing
    // keyups.


    while (pScanBuffer->next_offset != scanBufferCursor) {
        unsigned char keycode = pScanBuffer->key_ring[scanBufferCursor++];
        int ignoreKey = 0;
        if (nest_count == 0) {
            nest_count++;
            if (keycode == 0x3e) { //F4
                ignoreKey = 1;
                if (joshem_modal_ask_yn("Exit Simulator?")) {
                    exit(0);
                }
            } else if (keycode == 0x3F) { //F5
                
                 ignoreKey = 1;
                 /*
                 const char *p = josh_trace_enabled ? "Trace is ON.  Leave it on?" : "Trace is OFF.  Turn it on?";
                 if (joshem_modal_ask_yn(p)) {
                     josh_trace_enabled = 1;
                 } else {
                     josh_trace_enabled = 0;
                 }
                 */

                joshem_modal_message("Tracing not supported in this build");
                josh_trace_enabled = 0;
            } else if (keycode == 0x40) { //F6
                ignoreKey = 1;
                //Unusued
            } else if (keycode == 0x41) { //F7
                ignoreKey = 1;
                //Unused
            } else if (keycode == 0x42) { //F8
                ignoreKey = 1;
                joshem_request_tapedialog_status();
                //joshem_request_tapedialog();
            } else if (keycode == 0x43) { //F9
                ignoreKey = 1;
                const char *p = trs_is_realtime_enabled() ? "Fast Mode is OFF.  Turn it on?"
                                                          : "Fast mode is ON.  Leave it on?";
                if (joshem_modal_ask_yn(p)) {
                    trs_realtime_disable();
                } else {
                    trs_realtime_force_enable();
                }
            } else if (keycode == 0x44) { //F10
                ignoreKey = 1;
                int eec = joshem_emulator_control();
                if (eec == JOSHEM_EMULATOR_CONTROL_RESPONSE_EXIT) {
                    exit(0);
                } else if (eec == JOSHEM_EMULATOR_CONTROL_RESPONSE_RESET_HARD) {
                    trs_reset(1);
                } else if (eec == JOSHEM_EMULATOR_CONTROL_RESPONSE_RESET_SOFT) {
                    trs_reset(0);
                }
            }
            nest_count--;
        }
        if (ignoreKey) {
            continue;
        }

        int shifted = pScanBuffer->key_states[0x2A] || pScanBuffer->key_states[0x36];

        trs_xlate_pc_scancode(keycode, shifted);
        //joshlog("Keycode %x S=%u\n",(int)keycode, shifted);
    }

    //not_implemented("trs_get_event");
}

static void repaint_screen() {
#if VIDEO_DRIVER_VGA
    vga_needs_reset();
#else
    GrFilledBox(0, 0, GrMaxX(), GrMaxY(), GrBlack());
#endif

    for (int i = 0; i < screen_chars; i++) {
        trs_screen_write_char(i, trs_screen[i]);
    }
}

void trs_exit() {
    exit(0);
}

static void reload_grx_colors() {
    GrResetColors();
    COLOR_BORDER = GrAllocColor(0, 0, 192);
    COLOR_PRIMARY = GrAllocColor(0, 192, 0);
    COLOR_PRIMARY_DIM = GrAllocColor(0, 96, 0);
    COLOR_SECONDARY = GrAllocColor(192, 0, 0);
    COLOR_DISABLED = GrAllocColor(64,64,64);
    COLOR_SECONDARY_BRIGHT = GrAllocColor(255,255,255);
    COLOR_TERTIARY = GrAllocColor(127, 127, 0);

}


/* exits if something really bad happens */
void trs_screen_init() {
    int pat_flags;
    if (trs_charset < CHARSET_MIN || trs_charset > CHARSET_MAX) {
        fatal("Invalid charset %d\n", trs_charset);
    }
    pat_flags = 0;
    if (trs_charset >= CHARSET_MIN_8_PIXEL) {
        pat_flags |= PATTERN_FLAG_8_PIXEL_CHARS;
    }
    pat_flags |= PATTERN_FLAG_GRAPHICS_AT_128;
    if (trs_model == 1) {
        pat_flags |= PATTERN_FLAG_GRAPHICS_AT_192;
    }

    joshlog("Init pattern table: %d - %08x\n", trs_charset, pat_flags);
    init_pattern_table(&primary_pattern_table, trs_char_data[trs_charset], pat_flags);

    if (trs_model == 1) {
        joshlog("Init pattern gr80 table: %d - %08x\n", trs_charset, pat_flags | PATTERN_FLAG_BLANK_HIGH_CHARS);
        init_pattern_table(&grafix80_table, trs_char_data[trs_charset], pat_flags | PATTERN_FLAG_BLANK_HIGH_CHARS);

        //Note: We can't emulate 80-grafix in a standard way for 8 pixel fonts, but we can extend the idea to 8 pixel chars
        //  The only difference is that we will have to blank _all_ characters during programming because we need all 8 bits
        //  to define the character
        joshlog("Init pattern gr80prog table: %d - %08x\n", trs_charset, pat_flags | PATTERN_FLAG_BLANK_HIGH_CHARS);
        init_pattern_table(&grafix80_programming_table, trs_char_data[trs_charset],
                           pat_flags | PATTERN_FLAG_BLANK_HIGH_CHARS |
                                   ((pat_flags & PATTERN_FLAG_8_PIXEL_CHARS) ? PATTERN_FLAG_BLANK_LOW_CHARS : 0));

        if (pat_flags & PATTERN_FLAG_8_PIXEL_CHARS) {
            joshlog("NOTE: 80-grafix emulation behaves in a non-standard way for 8ppc fonts!");
        }
    }

    p_current_table = &primary_pattern_table;
    memset(trs_screen, 32, sizeof(trs_screen));

    // TODO - Move GRX stuff out to its own driver (except what we need for modals?)
    //FILE * modout;
    GrSetDriver("stdvga");
    //GrSetDriver("s3");


    /*

       GrFrameMode fm;
       const GrVideoMode *mp;
       for(fm =GR_firstGraphicsFrameMode; fm <= GR_lastGraphicsFrameMode; fm++) {
         mp = GrFirstVideoMode(fm);
         while( mp != NULL ) {
            joshlog("mode %d %d %d %d %d\n", (int)mp->mode, (int)mp->width, (int)mp->height, (int)mp->bpp, (int)mp->present);
           mp = GrNextVideoMode(mp);
         }
       }


     //exit(0);
    */

    /*
     char *message = "Booting...";
     int x, y;
     GrTextOption grt;

     GrSetMode( GR_width_height_graphics, 640, 200 );


     grt.txo_font = &GrDefaultFont;
     grt.txo_fgcolor.v = GrWhite();
     grt.txo_bgcolor.v = GrBlack();
     grt.txo_direct = GR_TEXT_RIGHT;
     grt.txo_xalign = GR_ALIGN_CENTER;
     grt.txo_yalign = GR_ALIGN_CENTER;
     grt.txo_chrtype = GR_BYTE_TEXT;

     GrBox( 0,0,GrMaxX(),GrMaxY(),GrWhite() );
     GrBox( 4,4,GrMaxX()-4,GrMaxY()-4,GrWhite() );

     x = GrMaxX()/2;
     y = GrMaxY()/2;
    joshlog("II %s %d %d\n",message, x, y);

     GrDrawString( message,strlen( message ),x,y,&grt );

     //GrKeyRead();

     //sleep(1);
      */


    GrSetMode(GR_width_height_graphics, 640, 200);
    joshlog("Video Driver is %s %d\n", GrCurrentVideoDriver()->name, (int)GrAdapterType());

    int x, y;
    x = GrMaxX() / 2;
    y = GrMaxY() / 2;
    joshlog("Midpoint: %d %d\n", x, y);


    trs_screen_pattern.gp_bitmap.bmp_ispixmap = 0;
    trs_screen_pattern.gp_bitmap.bmp_height = TRS_CHAR_HEIGHT;
    trs_screen_pattern.gp_bitmap.bmp_data = 0;
    trs_screen_pattern.gp_bitmap.bmp_fgcolor = GrWhite();
    trs_screen_pattern.gp_bitmap.bmp_bgcolor = GrBlack();
    trs_screen_pattern.gp_bitmap.bmp_memflags = 0;
    reload_grx_colors();
    repaint_screen();
    //TODO: This really should be done elsewhere... It's in trs_djgpp.c because it
    // uses our command line options.
    trs_load_romfile();

    return;

    //not_implemented("trs_screen_init");
}

void trs_screen_expanded(int flag) {
    int bit = flag ? EXPANDED : 0;
    if ((trs_current_video_mode ^ bit) & EXPANDED) {
        trs_current_video_mode ^= EXPANDED;
        repaint_screen();
    }
}

void trs_screen_alternate(int flag) {
    static int nicounter = 0;
    not_implemented("trs_screen_alternate", &nicounter);
}

void trs_screen_80x24(int flag) {
    if (flag && row_chars != 80) {
        joshlog("Switching to 80x24 mode");
        row_chars = 80;
        screen_chars = 80 * 24;
        repaint_screen();
    } else if (!flag && row_chars != 64) {
        joshlog("Switching to 80x24 mode");
        row_chars = 64;
        screen_chars = 64 * 16;
        repaint_screen();
    }

}

void trs_screen_inverse(int flag) {
    static int nicounter = 0;
    not_implemented("trs_screen_inverse", &nicounter);
}

void trs_screen_grafix80(int mode_bits) {
    mode_bits = mode_bits & 0xC0;
    if (mode_bits == 0 || mode_bits == 0xC0) {
        if (trs_current_video_mode & (GR80_ENABLE | GR80_PROGRAM)) {
            joshlog("SWITCHING TO NON-GRAFIX-80 MODE\n");
            trs_current_video_mode &= ~(GR80_ENABLE | GR80_PROGRAM);
            p_current_table = &primary_pattern_table;
            repaint_screen();
        }
    } else if (mode_bits == 0x80) {
        if (!(trs_current_video_mode & GR80_ENABLE)) {
            joshlog("SWITCHING TO GRAFIX-80 MODE\n");
            trs_current_video_mode |= GR80_ENABLE;
            trs_current_video_mode &= ~GR80_PROGRAM;
            p_current_table = &grafix80_table;
            repaint_screen();
        }
    } else if (mode_bits == 0x40) {
        if (!(trs_current_video_mode & GR80_PROGRAM)) {
            joshlog("SWITCHING TO GRAFIX-80 PROGRAM MODE\n");
            trs_current_video_mode |= GR80_PROGRAM;
            trs_current_video_mode &= ~GR80_ENABLE;
            p_current_table = &grafix80_programming_table;
            repaint_screen();
        }
    }
}

void trs_screen_grafix80_program(int location, int value) {
    if ((trs_current_video_mode & GR80_PROGRAM) == 0) {
        return;
    }
    int charNum = (location & 1023) >> 4;
    int row = location & 0xF;
    int bitmap = reverse_bits_char((char)value) & 0xFF;
    if (primary_pattern_table.pixels_per_char == 6) {
        bitmap = (bitmap >> 1) & 0x3F;
    }
    joshlog("Programming %d %d : %02x from %02x\n", charNum, row, bitmap, value & 0xFF);
    update_pattern_table(&grafix80_table, charNum + 128, row, bitmap);
    update_pattern_table(&grafix80_table, charNum + 196, row, bitmap);
}

void trs_screen_grafix80_program_block(int location_start, const char * value_start, int count) {
    if ((trs_current_video_mode & GR80_PROGRAM) == 0) {
        return;
    }
    for(int i=0;i<count;i++) {
        trs_screen_grafix80_program(location_start++, *value_start++);
    }
}

void trs_screen_scroll() {
    //int i = 0;
    //for (i = row_chars; i < screen_chars; i++)
    //  trs_screen[i-row_chars] = trs_screen[i];
    //repaint_screen();

    //TODO: Need to define variables for some of these magic numbers (120, 6, etc.)
    //   Note that they can change due to grafix mode

    trs_realtime_sync(5000);
    memmove(trs_screen, trs_screen + row_chars, screen_chars - row_chars);
#if VIDEO_DRIVER_VGA
    if (row_chars != 80) {
        vga_screen_scroll_64_16();
    } else {
        //TODO
        joshlog("OOPS!  80x24 scroll not implemented yet");
    }
#else
    if (row_chars != 80) {
        GrBitBlt(NULL, 120, 0, NULL, 120, TRS_CHAR_HEIGHT, 120 + 64 * 6, 16 * TRS_CHAR_HEIGHT, GrWRITE);
    } else {
        //TODO
        joshlog("OOPS!  80x24 scroll not implemented yet");
    }
#endif

}

#if VIDEO_DRIVER_VGA
#define WRITE_GLYPH_FN vga_screen_write_glyph_64_16
#define WRITE_GLYPH_FN8 vga_screen_write_glyph_64_16_8ppc
#define WRITE_GLYPH_FN8_8024 vga_screen_write_glyph_80_24_8ppc
#else
#define WRITE_GLYPH_FN trs_screen_write_glyph
//TODO - these are not supported
#define WRITE_GLYPH_FN8 trs_screen_write_glyph_8ppc
#define WRITE_GLYPH_FN8_8024 trs_screen_write_glyph_80_24_8ppc
#endif

static void trs_screen_write_glyph(char *glyphRows, int position) {

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
    trs_screen_pattern.gp_bitmap.bmp_data = patData;

    int x, y;

    x = (position & 63);
    y = position >> 6;
    int px, py;
    px = x * 6 + 120;   //offset (640-384)/2 then round down to a multiple of 24 so rotates work correctly
    py = y * TRS_CHAR_HEIGHT + 0;  //offset (200-192)/2 then round down to a multiple of 12 so patterns line up


    GrPatternFilledBox(px, py, px + 5, py + TRS_CHAR_HEIGHT - 1, &trs_screen_pattern);
    trs_screen_pattern.gp_bitmap.bmp_data = 0;
}

void trs_screen_write_char(int position, int char_index) {
    //joshlog("WC %d %d\n", position, char_index);

    void (*gfn)(char *glyphRows, int position);
    if (p_current_table->pixels_per_char == 6) {
        gfn = WRITE_GLYPH_FN;
    } else if (row_chars == 64) {
        gfn = WRITE_GLYPH_FN8;
    } else {
        gfn = WRITE_GLYPH_FN8_8024;
    }


    //trs_realtime_sync(5000);
    char_index = char_index & 0xff;

    if (row_chars == 64) {
        position = position & 1023;
    } else {
        position = position & 2047;
        if (position >= screen_chars) {
            return;
        }
    }

    trs_screen[position] = (char) char_index;

    if (!(trs_current_video_mode & EXPANDED)) {
        gfn(p_current_table->normal[char_index], position);
    } else {
        if (position & 1) {
            return;
        }
        gfn(p_current_table->wideleft[char_index], position);
        gfn(p_current_table->wideright[char_index], position | 1);
    }
}


void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons) {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_pos", &nicounter);
}

void trs_set_mouse_pos(int x, int y) {
    static int nicounter = 0;
    not_implemented("trs_set_mouse_pos", &nicounter);
}

void trs_get_mouse_max(int *x, int *y, unsigned int *sens) {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_max", &nicounter);
}

void trs_set_mouse_max(int x, int y, unsigned int sens) {
    static int nicounter = 0;
    not_implemented("trs_set_mouse_max", &nicounter);
}

int trs_get_mouse_type() {
    static int nicounter = 0;
    not_implemented("trs_get_mouse_type", &nicounter);
    return 0;
}


void grafyx_write_byte(int x, int y, char byte) {
    //Write a byte to HRG memory after x and y have been decoded.  For the Radio Shack boards and the
    // microlabs model 4 board, the x and y registers are set separately and then this method should be called
    // from grafyx_write_data
    // For the microlabs model 3 board, which is memory mapped, they get partially? decoded from the
    // address via grafyx_m3_write_byte.

    //Note that auto incrementing/decrementing does not happen hea
    static int nicounter = 0;
    not_implemented("grafyx_write_byte", &nicounter); }

void grafyx_write_x(int value) {
    //Set the "X" register for the next HRG write
    static int nicounter = 0;
    not_implemented("grafyx_write_x", &nicounter); }

void grafyx_write_y(int value) {
    //Set the "Y" register for the next HRG write
    static int nicounter = 0;
    not_implemented("grafyx_write_y", &nicounter); }

void grafyx_write_data(int value) {
    // Call grafyx_write_byte, then do any auto-inc/auto-dec
    static int nicounter = 0;
    not_implemented("grafyx_write_data", &nicounter); }

int grafyx_read_data() {
    // Read the grafyx byte, do auto-inc/auto-dec, and then return the value
    static int nicounter = 0;
    not_implemented("grafyx_read_data", &nicounter);
    return 0;
}

void grafyx_write_mode(int value) {
    // Set all the auto-inc/auto-dec values
    static int nicounter = 0;
    not_implemented("grafyx_write_mode", &nicounter); }

void grafyx_write_xoffset(int value) {
    // Write the x-offset (undocumented scroll feature)
    static int nicounter = 0;
    not_implemented("grafyx_write_xoffset", &nicounter); }

void grafyx_write_yoffset(int value) {
    // Write the y-offset (undocumented scroll feature)
    static int nicounter = 0;
    not_implemented("grafyx_write_yoffset", &nicounter); }

void grafyx_write_overlay(int value) {
    // Write the undocumented model 4 byte that lets you overlay
    //  text with hi-res grafix.  In model 3 grafyx (microlabs only?), this is
    //  set via the write_mode?
    static int nicounter = 0;
    not_implemented("grafyx_write_overlay", &nicounter); }

int grafyx_get_microlabs() {
    //This returns true if we are emulating the microlabs board (model 3 or 4 board, which are very different from
    // each other)
    static int nicounter = 0;
    not_implemented("grafyx_get_microlabs", &nicounter);
    return 0;
}

void grafyx_set_microlabs(int on_off) {
    //Sets if we are emulating the microlabs board (model 3 or 4 board, which are very different from
    // each other)
    static int nicounter = 0;
    not_implemented("grafyx_set_microlabs", &nicounter);
}

void grafyx_m3_reset() {
    //Called when M3 resets - in old xtrs, really only needed for the
    //microlabs card but it is called regardless
    static int nicounter = 0;
    not_implemented("grafyx_m3_reset", &nicounter); }

void grafyx_m3_write_mode(int value) {
    //Called for m3 microlabs - seems to be mapped to port 0xFF
    static int nicounter = 0;
    not_implemented("grafyx_m3_write_mode", &nicounter); }

int grafyx_m3_write_byte(int position, int byte) {
    //Called for m3 microlabs - memory mapped
    static int nicounter = 0;
    not_implemented("grafyx_m3_write_byte", &nicounter);
    return 0;
}

unsigned char grafyx_m3_read_byte(int position) {
    //Called for m3 microlabs - memory mapped
    static int nicounter = 0;
    not_implemented("grafyx_m3_read_byte", &nicounter);
    return 0;
}

int grafyx_m3_active() {
    // For microlabs model3, returns true if is enabled
    static int nicounter = 0;
    not_implemented("grafyx_m3_active", &nicounter);
    return 0;
}

int hrg_read_data() {
    //Model 1 I think
    static int nicounter = 0;
    not_implemented("hrg_read_data", &nicounter);
    return 0;
}

void hrg_write_addr(int addr, int mask) {
    //Model 1 I think
    static int nicounter = 0;
    not_implemented("hrg_write_addr", &nicounter); }

void hrg_write_data(int data) {
    //Model 1 I think
    static int nicounter = 0;
    not_implemented("hrg_write_data", &nicounter); }

void hrg_onoff(int enable) {
    //Model 1 I think
    static int nicounter = 0;
    not_implemented("hrg_onoff", &nicounter); }


/*
 * Command line parsing.  In trs_gtkinterface mostly for historical
 * reasons (the old trs_xinterface looked in the X resource database)
 * and partly because it sets a lot of values that are used here.  Ugh.
 */

int opt_iconic = FALSE;
char *opt_background = NULL;
char *opt_foreground = NULL;
int opt_debug = FALSE;
char *opt_title;
int opt_shiftbracket = -1;
char *opt_charset = NULL;
char *opt_scale = NULL;
char *opt_romfile = NULL;
char *opt_romfile3 = NULL;
char *opt_romfile4p = NULL;
int opt_stepdefault = 1;
char *opt_stepmap = NULL;
char *opt_sizemap = NULL;

struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

struct option options[] = {
        /* Name, takes argument?, store int value at, value to store */
        {"iconic",         FALSE, &opt_iconic,              TRUE},
        {"noiconic",       FALSE, &opt_iconic,              FALSE},
        {"background",     TRUE, NULL,              0},
        {"bg",             TRUE, NULL,              0},
        {"foreground",     TRUE, NULL,              0},
        {"fg",             TRUE, NULL,              0},
        {"title",          TRUE, NULL,              0},
        {"borderwidth",    TRUE, NULL,              0},
        {"scale",          TRUE, NULL,              0},
        {"scale1",         FALSE, &scale_x,         1},
        {"scale2",         FALSE, &scale_x,         2},
        {"scale3",         FALSE, &scale_x,         3},
        {"scale4",         FALSE, &scale_x,         4},
        {"resize",         FALSE, &resize,                  TRUE},
        {"noresize",       FALSE, &resize,                  FALSE},
        {"charset",        TRUE, NULL,              0},
        {"microlabs",      FALSE, &grafyx_microlabs,        TRUE},
        {"nomicrolabs",    FALSE, &grafyx_microlabs,        FALSE},
        {"debug",          FALSE, &opt_debug,               TRUE},
        {"nodebug",        FALSE, &opt_debug,               FALSE},
        {"romfile",        TRUE, NULL,              0},
        {"romfile3",       TRUE, NULL,              0},
        {"romfile4p",      TRUE, NULL,              0},
        {"model",          TRUE, NULL,              0},
        {"model1",         FALSE, &trs_model,       1},
        {"model3",         FALSE, &trs_model,       3},
        {"model4",         FALSE, &trs_model,       4},
        {"model4p",        FALSE, &trs_model,       5},
        {"delay",          TRUE, NULL,              0},
        {"autodelay",      FALSE, &trs_autodelay,           TRUE},
        {"noautodelay",    FALSE, &trs_autodelay,           FALSE},
        {"keystretch",     TRUE, NULL,              0},
        {"shiftbracket",   FALSE, &opt_shiftbracket,        TRUE},
        {"noshiftbracket", FALSE, &opt_shiftbracket,        FALSE},
        {"diskdir",        TRUE, NULL,              0},
        {"doubler",        TRUE, NULL,              0},
        {"doublestep",     FALSE, &opt_stepdefault, 2},
        {"nodoublestep",   FALSE, &opt_stepdefault, 1},
        {"stepmap",        TRUE, NULL,              0},
        {"sizemap",        TRUE, NULL,              0},
        {"truedam",        FALSE, &trs_disk_truedam,        TRUE},
        {"notruedam",      FALSE, &trs_disk_truedam,        FALSE},
        {"samplerate",     TRUE, NULL,              0},
        {"serial",         TRUE, NULL,              0},
        {"switches",       TRUE, NULL,              0},
        {"emtsafe",        FALSE, &trs_emtsafe,             TRUE},
        {"noemtsafe",      FALSE, &trs_emtsafe,             FALSE},
        {"m1lc",           FALSE, &trs_model1_lowercase,    TRUE},
        {"nom1lc",         FALSE, &trs_model1_lowercase,    FALSE},
        {"expintf",        FALSE, &trs_expansion_interface, TRUE},
        {"noexpintf",      FALSE, &trs_expansion_interface, FALSE},
        {"grafix80",        FALSE, &trs_model1_grafix80, TRUE},
        {"nografix80",      FALSE, &trs_model1_grafix80, FALSE},
        {NULL, 0,                 0,                0}
};

static int find_opt_match(const char *arg, const struct option *longopts) {
    int i;
    for (i = 0; longopts && longopts->name; longopts++, i++) {
        if (strcmp(arg, longopts->name) == 0) {
            return i;
        }
    }
    return -1;
}

static int getopt_long_only(int argc, char *const argv[],
                            const char *optstring,
                            const struct option *longopts, int *longindex) {

    const char *cur;
    int match_index;
    static const struct option *match;
    if (optind <= 1) {
        optind = 1;
    }
    for (; optind < argc;) {
        cur = argv[optind++];
        joshlog("on %s\n", cur);
        if (cur[0] != '-') {
            fatal("Bad option: %s", cur);
        }
        cur += 1;
        match_index = find_opt_match(cur, longopts);
        if (match_index < 0) {
            fatal("Bad option: -%s", cur);
            return -1;
        }
        match = longopts + match_index;
        *longindex = match_index;

        if (match->has_arg) {
            if (optind >= argc) {
                fatal("Missing argument to -%s", cur);
                return -1;
            }
            optarg = argv[optind++];
            joshlog("getopt : -%s with %s\n", cur, optarg);
        } else {
            joshlog("getopt : -%s\n", cur);
        }
        if (match->flag) {
            *(match->flag) = match->val;
            return 0;
        }
        return match->val;
    }
    return -1;


}


int
trs_parse_command_line(int argc, char **argv, int *debug) {
    int i;
    int s[8];
    char *charpeek;

    charpeek = getenv("CHARPEEK");
    if (!charpeek) {
        fatal("Unable to read CHARPEEK environment variable");
    } else {
        unsigned short cps, cpo;
        if (sscanf(charpeek, "%hx:%hx", &cps, &cpo) != 2) {
            fatal("Cannot parse CHARPEEK");
        }
        unsigned int real_addr = ((unsigned long) cps) * 16 + cpo;
        unsigned int real_page = real_addr & (~4095);
        unsigned int real_offset = real_addr - real_page;
        joshlog("CHARPEEK REAL MODE ADDRESS RADDR=%x RPAGE=%x ROFF=%x\n", real_addr, real_page, real_offset);


        char *p;
        p = malloc(3 * 4096);
        p += 4096 - (((unsigned int) p) & 4095);

        //printf("A %p %d\n", p, errno);
        int x = -1;
        //printf("B %x %p %x %d\n", real_page, p,x,errno);
        x = __djgpp_map_physical_memory(p, 8192, real_page);
        joshlog("CHARPEAK MAPPED RPAGE=%x PPAGE=%p MAPRES=%x ERRNO=%d\n", real_page, p, x, errno);


        pScanBuffer = (struct scan_buffer *) (p + real_offset);
        joshlog("CHARPEAK MAPPED BUFFER=%p\n", pScanBuffer);

        pScanBuffer->suppress_flag = 1;
        //sleep(1);
        scanBufferCursor = pScanBuffer->next_offset;
    }


    emulator_base_directory = getcwd(0, 1024);
    if (!emulator_base_directory) {
        joshlog("Unable to get emulator base dir\n");
        emulator_base_directory = ".";
    }
    joshlog("Emulator base dir = %s\n", emulator_base_directory);

    cassette_base_directory = malloc(32 + strlen(emulator_base_directory));
    if (!cassette_base_directory) {
        joshlog("Unable to allocate cassette base directory");
        cassette_base_directory = "./CAS";
    } else {
        sprintf((char *) cassette_base_directory, "%s/CAS", emulator_base_directory);
    }
    mkdir(cassette_base_directory, S_IWUSR);  //S_IWUSR ==> not read only

    emulator_printer_directory = malloc(32 + strlen(emulator_base_directory));
    if (!emulator_printer_directory) {
        joshlog("Unable to allocate printer base directory");
        emulator_printer_directory = "./PRINT";
    } else {
        sprintf((char *) emulator_printer_directory, "%s/PRINT", emulator_base_directory);
    }
    mkdir(emulator_printer_directory, S_IWUSR);  //S_IWUSR ==> not read only

    trs_model = 1;
    trs_model1_lowercase = FALSE;

    //Ugh - getopt is a horrible API
    optind = 1;
    opterr = 0;
    for (;;) {
        int c;
        int option_index = 0;
        const char *name;

        c = getopt_long_only(argc, argv, "", options, &option_index);
        if (c == -1) break;
        if (c == '?') {
            fatal("unrecognized option %s", argv[optind - 1]);
        }
        name = options[option_index].name;
        if (strcmp(name, "background") == 0 ||
            strcmp(name, "bg") == 0) {
            opt_background = optarg;
        } else if (strcmp(name, "foreground") == 0 ||
                   strcmp(name, "fg") == 0) {
            opt_foreground = optarg;
        } else if (strcmp(name, "title") == 0) {
            opt_title = optarg;
        } else if (strcmp(name, "borderwidth") == 0) {
            border_width = strtoul(optarg, NULL, 0);
        } else if (strcmp(name, "scale") == 0) {
            sscanf(optarg, "%u,%u", &scale_x, &scale_y);
        } else if (strcmp(name, "charset") == 0) {
            opt_charset = optarg;
        } else if (strcmp(name, "romfile") == 0) {
            opt_romfile = optarg;
        } else if (strcmp(name, "romfile3") == 0) {
            opt_romfile3 = optarg;
        } else if (strcmp(name, "romfile4p") == 0) {
            opt_romfile4p = optarg;
        } else if (strcmp(name, "model") == 0) {
            if (strcmp(optarg, "1") == 0 ||
                strcasecmp(optarg, "I") == 0) {
                trs_model = 1;
            } else if (strcmp(optarg, "3") == 0 ||
                       strcasecmp(optarg, "III") == 0) {
                trs_model = 3;
            } else if (strcmp(optarg, "4") == 0 ||
                       strcasecmp(optarg, "IV") == 0) {
                trs_model = 4;
            } else if (strcasecmp(optarg, "4P") == 0 ||
                       strcasecmp(optarg, "IVp") == 0) {
                trs_model = 5;
            } else {
                fatal("TRS-80 Model %s not supported", optarg);
            }
        } else if (strcmp(name, "delay") == 0) {
            z80_state.delay = strtol(optarg, NULL, 0);
        } else if (strcmp(name, "keystretch") == 0) {
            stretch_amount = strtol(optarg, NULL, 0);
        } else if (strcmp(name, "diskdir") == 0) {
            trs_disk_dir = strdup(optarg);
            if (trs_disk_dir[0] == '~' &&
                (trs_disk_dir[1] == '/' || trs_disk_dir[1] == '\0')) {
                char *home = getenv("HOME");
                if (home) {
                    char *p = (char *) malloc(strlen(home) + strlen(trs_disk_dir) + 1);
                    sprintf(p, "%s/%s", home, trs_disk_dir + 1);
                    trs_disk_dir = p;
                }
            }
        } else if (strcmp(name, "doubler") == 0) {
            switch (optarg[0]) {
                case 'p':
                case 'P':
                    trs_disk_doubler = TRSDISK_PERCOM;
                    break;
                case 'r':
                case 'R':
                case 't':
                case 'T':
                    trs_disk_doubler = TRSDISK_TANDY;
                    break;
                case 'b':
                case 'B':
                    trs_disk_doubler = TRSDISK_BOTH;
                    break;
                case 'n':
                case 'N':
                    trs_disk_doubler = TRSDISK_NODOUBLER;
                    break;
                default:
                    fatal("unrecognized doubler type %s\n", optarg);
            }
        } else if (strcmp(name, "stepmap") == 0) {
            opt_stepmap = optarg;
        } else if (strcmp(name, "sizemap") == 0) {
            opt_sizemap = optarg;
        } else if (strcmp(name, "samplerate") == 0) {
            cassette_default_sample_rate = strtol(optarg, NULL, 0);
        } else if (strcmp(name, "serial") == 0) {
            trs_uart_name = strdup(optarg);
        } else if (strcmp(name, "switches") == 0) {
            trs_uart_switches = strtol(optarg, NULL, 0);
        }
    }
    if (optind != argc) {
        fatal("unrecognized argument %s", argv[optind]);
    }

    trs_video_ram_7_bit = trs_model == 1 && !trs_model1_lowercase;

    if (trs_video_ram_7_bit) {
        joshlog("Video RAM is 7 bits\n");
    } else {
        joshlog("Video RAM is 8 bits\n");
    }
    if (trs_model == 1 && !trs_expansion_interface) {
        trs_ram_end = 0x8000;
    }

    /*
     * Some additional processing needed after all options are parsed.
     * In some cases the order is important; e.g., trs_model must be known.
     */
    *debug = opt_debug;

    if (resize == -1) {
        resize = (trs_model == 3);
    }

    if (opt_shiftbracket == -1) {
        opt_shiftbracket = trs_model >= 4;
    }
    trs_kb_bracket(opt_shiftbracket);

    if (scale_y == 0) scale_y = 2 * scale_x;

    /* Note: charset numbers must match trs_chars.c */
    if (trs_model == 1) {
        if (opt_charset == NULL) {
            if (!trs_model1_lowercase) {
                opt_charset = "stock"; /* default */
            } else {
                opt_charset = "lcmod";
            }
        }
        if (isdigit(*opt_charset)) {
            trs_charset = strtol(opt_charset, NULL, 0);
            cur_char_width = 8 * scale_x;
        } else {
            if (opt_charset[0] == 'e'/*early*/) {
                trs_charset = 0;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 's'/*stock*/) {
                trs_charset = 1;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 'l'/*lcmod*/) {
                trs_charset = 2;
                cur_char_width = 6 * scale_x;
            } else if (opt_charset[0] == 'w'/*wider*/) {
                trs_charset = 3;
                cur_char_width = 8 * scale_x;
            } else if (opt_charset[0] == 'g'/*genie or german*/) {
                trs_charset = 10;
                cur_char_width = 8 * scale_x;
            }
            else {
                fatal("unknown charset name %s", opt_charset);
            }
        }
        cur_char_height = TRS_CHAR_HEIGHT * scale_y;
    } else /* trs_model > 1 */ {
        if (opt_charset == NULL) {
            /* default */
            opt_charset = (trs_model == 3) ? "katakana" : "international";
        }
        if (isdigit(*opt_charset)) {
            trs_charset = strtol(opt_charset, NULL, 0);
        } else {
            if (opt_charset[0] == 'k'/*katakana*/) {
                trs_charset = 4 + 3 * (trs_model > 3);
            } else if (opt_charset[0] == 'i'/*international*/) {
                trs_charset = 5 + 3 * (trs_model > 3);
            } else if (opt_charset[0] == 'b'/*bold*/) {
                trs_charset = 6 + 3 * (trs_model > 3);
            } else {
                fatal("unknown charset name %s", opt_charset);
            }
        }
        cur_char_width = TRS_CHAR_WIDTH * scale_x;
        cur_char_height = TRS_CHAR_HEIGHT * scale_y;
    }

    for (i = 0; i <= 7; i++) {
        s[i] = opt_stepdefault;
    }
    if (opt_stepmap) {
        sscanf(opt_stepmap, "%d,%d,%d,%d,%d,%d,%d,%d",
               &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]);
    }
    for (i = 0; i <= 7; i++) {
        if (s[i] != 1 && s[i] != 2) {
            fatal("bad value %d for disk %d single/double step\n", s[i], i);
        } else {
            trs_disk_setstep(i, s[i]);
        }
    }

    /* Defaults for sizemap */
    s[0] = 5;
    s[1] = 5;
    s[2] = 5;
    s[3] = 5;
    s[4] = 8;
    s[5] = 8;
    s[6] = 8;
    s[7] = 8;
    if (opt_sizemap) {
        sscanf(opt_sizemap, "%d,%d,%d,%d,%d,%d,%d,%d",
               &s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]);
    }
    for (i = 0; i <= 7; i++) {
        if (s[i] != 5 && s[i] != 8) {
            fatal("bad value %d for disk %d size", s[i], i);
        } else {
            trs_disk_setsize(i, s[i]);
        }
    }

    return 1;
}


/*
 * XXX This really does not belong in trs_gtkinterface.  Need to
 * communicate the opt_romfile* values, though.
 */
void
trs_load_romfile() {
    char *romfile = NULL;
    struct stat statbuf;

    switch (trs_model) {
        case 1:
            if (opt_romfile) {
                romfile = opt_romfile;
#ifdef DEFAULT_ROM
            } else if (stat(DEFAULT_ROM, &statbuf) == 0) {
                romfile = DEFAULT_ROM;
#endif
            }
            if (romfile != NULL) {
                joshlog("Loading rom %s\n", romfile);
                trs_load_rom(romfile);
                joshlog("Loaded rom %s\n", romfile);
            } else if (trs_rom1_size > 0) {
                trs_load_compiled_rom(trs_rom1_size, trs_rom1);
            } else {
                fatal("ROM file not specified!");
            }
            break;

        case 3:
        case 4:
            if (opt_romfile3) {
                romfile = opt_romfile3;
#ifdef DEFAULT_ROM3
            } else if (stat(DEFAULT_ROM3, &statbuf) == 0) {
                romfile = DEFAULT_ROM3;
#endif
            }
            if (romfile != NULL) {
                joshlog("Loading rom %s\n", romfile);
                trs_load_rom(romfile);
                joshlog("Loaded rom %s\n", romfile);
            } else if (trs_rom3_size > 0) {
                trs_load_compiled_rom(trs_rom3_size, trs_rom3);
            } else {
                fatal("ROM file not specified!");
            }
            break;

        default: /* 4P */
            if (opt_romfile4p) {
                romfile = opt_romfile4p;
#ifdef DEFAULT_ROM4P
            } else if (stat(DEFAULT_ROM4P, &statbuf) == 0) {
                romfile = DEFAULT_ROM4P;
#endif
            }
            if (romfile != NULL) {
                trs_load_rom(romfile);
            } else if (trs_rom4p_size > 0) {
                trs_load_compiled_rom(trs_rom4p_size, trs_rom4p);
            } else {
                fatal("ROM file not specified!");
            }
            break;
    }
}


int joshem_do_modal(joshem_modal_handler handler, void *input) {
#if VIDEO_DRIVER_VGA
    GrSetMode(GR_width_height_graphics, 640, 200);
    reload_grx_colors();
#endif
    joshem_modal_context context;

    memset(&context, 0, sizeof(context));

    context.input = input;

    trs_wait_for_all_keys_up();
    pScanBuffer->suppress_flag = 0;

    handler(&context);
    repaint_screen();
    pScanBuffer->suppress_flag = 1;

    scanBufferCursor = pScanBuffer->next_offset;
    trs_realtime_reset();
    return context.result;

}
