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
#include <signal.h>
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



// Private data
static unsigned char trs_screen[2048];
static int screen_chars = 1024;
static int row_chars = 64;
static int col_chars = 16;


static int scale_x = 1;
static int scale_y = 0;
static int resize = -1;
static int grafyx_microlabs = 0;
static int border_width = 2;
static int cur_char_height, cur_char_width;
static int trs_charset;

static struct scan_buffer *pScanBuffer = 0;
static unsigned char scanBufferCursor;

static char pattern_table_1[MAXCHARS][TRS_CHAR_HEIGHT];


static void trs_load_romfile();


static char reverse_bits(char c) {
  char r = 0;
  for(int i=0;i<8;i++) {
    r = (r << 1) | (c & 1);
    c >>= 1;
  }
  return r;
}
void reverse_bits_block(void *p, int len) {
  char *c = (char *)p;
  for(;len>0;c++,len--) {
    *c = reverse_bits(*c);
  }
}


static char rotate_left(char c, int steps) {
  //TODO - inline assembly
  int c2 = ((int)c) & 0xFF;
  c2 <<= (steps & 7);
  return (char)((c2 >> 8) | c2);
}
void rotate_left_block(void *p, int steps, int len) {
  char *c = (char *)p;
  for(;len>0;c++,len--) {
    *c = rotate_left(*c, steps);
  }
}


static void not_implemented(const char *msg) {

  joshlog("Not implemented: %s\n", msg);


  //exit(100);
}


//TODO - Move to h file
extern void trs_xlate_pc_scancode(unsigned char scan_code, int shifted);


void trs_get_event(int wait) {
  //TODO: I think there's a bug here (or in the trs_xlate_pc_scancode code that goes with it)
  // If shifted and unshifted IBM key maps to different TRS keys, and if shift is released
  // before IBM key, it may be that we send the incorrect key-up.   This causes the Level 1
  // keyboard driver (and maybe others) to hang because it loops waitinf for a keyup that it
  // never sees.   Perhaps I need to keep track of whether shift is forced up or down when doing
  // keyups.

  //TODO: Keep wait or get rid of it/
  if (wait && pScanBuffer->next_offset == scanBufferCursor) {
    usleep(55000);
  }
  while(pScanBuffer->next_offset != scanBufferCursor) {
    unsigned char keycode = pScanBuffer->key_ring[scanBufferCursor++];
    if (keycode == 0x3e) {
      exit(0);
    }
    if (keycode == 0x3F) {
      josh_trace_enabled = 1;
    }
    if (keycode == 0x40) {
      josh_trace_enabled = 0;
    }
    int shifted = pScanBuffer->key_states[0x2A] || pScanBuffer->key_states[0x36];
    trs_xlate_pc_scancode(keycode, shifted);
    //joshlog("Keycode %x\n",(int)keycode);
  }

  //not_implemented("trs_get_event");
}

static void repaint_screen() {
  GrFilledBox( 0,0,GrMaxX(),GrMaxY(),GrBlack() );

  for (int i = 0; i < screen_chars; i++) {
      trs_screen_write_char(i, trs_screen[i]);
  }
}

void trs_exit() {
  exit(0);
}



/* exits if something really bad happens */
void trs_screen_init()
{

    memset(trs_screen, 32, sizeof(trs_screen));
    memcpy(pattern_table_1, trs_char_data[1], sizeof(pattern_table_1));
    reverse_bits_block(pattern_table_1, sizeof(pattern_table_1));

    for(int grindex=0;grindex<64;grindex++) {
      char scans[3] = {0,0,0};
      scans[0] |= (grindex & 1) ? 0xE0 : 0;
      scans[0] |= (grindex & 2) ? 0x1C : 0; 
      scans[1] |= (grindex & 4) ? 0xE0 : 0;
      scans[1] |= (grindex & 8) ? 0x1C : 0; 
      scans[2] |= (grindex & 16) ? 0xE0 : 0;
      scans[2] |= (grindex & 32) ? 0x1C : 0; 
      for(int scanline=0;scanline<TRS_CHAR_HEIGHT;scanline++) {
        int row = scanline / (TRS_CHAR_HEIGHT / 3);
        pattern_table_1[128 + grindex][scanline] = scans[row];
        pattern_table_1[192 + grindex][scanline] = scans[row];
      }
    }

    //FILE * modout;
    GrSetDriver("VESA");

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
     
   char *message = "Hello, GRX world";
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
 
   sleep(1);

   //TODO: This really should be done elsewhere... It's in trs_djgpp.c because it
   //uses our command line options.
   repaint_screen();
   trs_load_romfile();

   return;

  //not_implemented("trs_screen_init"); 
}

void trs_screen_expanded(int flag)
{
  not_implemented("trs_screen_expanded"); 
}
void trs_screen_alternate(int flag) {
  not_implemented("trs_screen_alternate"); 
}
void trs_screen_80x24(int flag) {
  not_implemented("trs_screen_80x24"); 
}
void trs_screen_inverse(int flag) {
  not_implemented("trs_screen_inverse"); 
}
void trs_screen_scroll() {
  int i = 0;

  for (i = row_chars; i < screen_chars; i++)
    trs_screen[i-row_chars] = trs_screen[i];

  repaint_screen();

}
void trs_screen_write_char(int position, int char_index) {
  //joshlog("WC %d %d\n", position, char_index);
   char_index = char_index & 0xff;
   position = position & 1023;  //TODO - Assume 64x16
   trs_screen[position] = (char)char_index;


   char patData[TRS_CHAR_HEIGHT];
   memcpy(patData, pattern_table_1[char_index], TRS_CHAR_HEIGHT);
   rotate_left_block(patData, (position & 3) << 1, TRS_CHAR_HEIGHT);

   //char * pData = pattern_table_1[char_index];

   GrPattern pat;
   pat.gp_bitmap.bmp_ispixmap = 0;
   pat.gp_bitmap.bmp_height = TRS_CHAR_HEIGHT;
   pat.gp_bitmap.bmp_data = patData;
   pat.gp_bitmap.bmp_fgcolor = GrWhite();
   pat.gp_bitmap.bmp_bgcolor = GrBlack();
   pat.gp_bitmap.bmp_memflags = 0;


   int x,y;

   x = (position & 63);
   y = position >> 6;
   int px, py;
   px = x * 6 + 120;   //offset (640-384)/2 then round down to a multiple of 24 so rotates work correctlt
   py = y * TRS_CHAR_HEIGHT + 0;  //offset (200-192)/2 then rown down to a multiple of 12 so patterns line up


   GrPatternFilledBox(px, py, px+5, py+TRS_CHAR_HEIGHT - 1, &pat);
   return;

   GrTextOption grt;


   grt.txo_font = &GrDefaultFont;
   grt.txo_fgcolor.v = GrWhite();
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;

   char message[2];

   position = position & 1023;
   x = (position & 63);
   y = position >> 6;
   message[0] = (char)char_index;
   message[1] = 0;


   x = x * 10 + 5;
   y = y * 12 + 6;
   
   //joshlog("WC2 %s %d %d\n",message, x, y);

   GrDrawString( message,strlen( message ),x,y,&grt );
   //joshlog("WC3 %s %d %d\n",message, x, y);

  //not_implemented("trs_screen_write_char"); 
}



void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons) {
  not_implemented("trs_get_mouse_pos");
}
void trs_set_mouse_pos(int x, int y) {
  not_implemented("trs_set_mouse_pos");
}
void trs_get_mouse_max(int *x, int *y, unsigned int *sens) {
  not_implemented("trs_get_mouse_max");
}
void trs_set_mouse_max(int x, int y, unsigned int sens) {
  not_implemented("trs_set_mouse_max");
}
int trs_get_mouse_type() {
  not_implemented("trs_get_mouse_type");
  return 0;
}






void grafyx_write_byte(int x, int y, char byte) { not_implemented("grafyx_write_byte"); }
void grafyx_write_x(int value) { not_implemented("grafyx_write_x"); }
void grafyx_write_y(int value) { not_implemented("grafyx_write_y"); }
void grafyx_write_data(int value) { not_implemented("grafyx_write_data"); }
int grafyx_read_data() { not_implemented("grafyx_read_data"); return 0; }
void grafyx_write_mode(int value) { not_implemented("grafyx_write_mode"); }
void grafyx_write_xoffset(int value) { not_implemented("grafyx_write_xoffset"); }
void grafyx_write_yoffset(int value) { not_implemented("grafyx_write_yoffset"); }
void grafyx_write_overlay(int value) { not_implemented("grafyx_write_overlay"); }
int grafyx_get_microlabs() { not_implemented("grafyx_get_microlabs"); return 0; }
void grafyx_set_microlabs(int on_off) { not_implemented("grafyx_set_microlabs"); }
void grafyx_m3_reset() { not_implemented("grafyx_m3_reset"); }
void grafyx_m3_write_mode(int value) { not_implemented("grafyx_m3_write_mode"); }
int grafyx_m3_write_byte(int position, int byte) { not_implemented("grafyx_m3_write_byte"); return 0; }
unsigned char grafyx_m3_read_byte(int position) { not_implemented("grafyx_m3_read_byte"); return 0; }
int grafyx_m3_active() { not_implemented("grafyx_m3_active"); return 0; }

int hrg_read_data()  { not_implemented("hrg_read_data"); return 0; }
void hrg_write_addr(int addr, int mask) { not_implemented("hrg_write_addr"); }
void hrg_write_data(int data) { not_implemented("hrg_write_data"); }
void hrg_onoff(int enable) { not_implemented("hrg_onoff"); }





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
  int * flag;
  int val;
};

struct option options[] = {
  /* Name, takes argument?, store int value at, value to store */
  {"iconic",         FALSE, &opt_iconic,       TRUE  },
  {"noiconic",       FALSE, &opt_iconic,       FALSE },
  {"background",     TRUE,  NULL,              0     },
  {"bg",       TRUE,  NULL,              0     },
  {"foreground",     TRUE,  NULL,              0     },
  {"fg",             TRUE,  NULL,              0     },
  {"title",          TRUE,  NULL,              0     },
  {"borderwidth",    TRUE,  NULL,              0     },
  {"scale",          TRUE,  NULL,              0     },
  {"scale1",         FALSE, &scale_x,          1     },
  {"scale2",         FALSE, &scale_x,          2     },
  {"scale3",         FALSE, &scale_x,          3     },
  {"scale4",         FALSE, &scale_x,          4     },
  {"resize",       FALSE, &resize,           TRUE  },
  {"noresize",       FALSE, &resize,           FALSE },
  {"charset",        TRUE,  NULL,              0     },
  {"microlabs",      FALSE, &grafyx_microlabs, TRUE  },
  {"nomicrolabs",    FALSE, &grafyx_microlabs, FALSE },
  {"debug",      FALSE, &opt_debug,        TRUE  },
  {"nodebug",        FALSE, &opt_debug,        FALSE },
  {"romfile",      TRUE,  NULL,              0     },
  {"romfile3",       TRUE,  NULL,              0     },
  {"romfile4p",      TRUE,  NULL,              0     },
  {"model",          TRUE,  NULL,              0     },
  {"model1",         FALSE, &trs_model,        1     },
  {"model3",         FALSE, &trs_model,        3     },
  {"model4",         FALSE, &trs_model,        4     },
  {"model4p",        FALSE, &trs_model,        5     },
  {"delay",          TRUE,  NULL,              0     },
  {"autodelay",      FALSE, &trs_autodelay,    TRUE  },
  {"noautodelay",    FALSE, &trs_autodelay,    FALSE },
  {"keystretch",     TRUE,  NULL,              0     },
  {"shiftbracket",   FALSE, &opt_shiftbracket, TRUE  },
  {"noshiftbracket", FALSE, &opt_shiftbracket, FALSE },
  {"diskdir",        TRUE,  NULL,              0     },
  {"doubler",        TRUE,  NULL,              0     },
  {"doublestep",     FALSE, &opt_stepdefault,  2     },
  {"nodoublestep",   FALSE, &opt_stepdefault,  1     },
  {"stepmap",        TRUE,  NULL,              0     },
  {"sizemap",        TRUE,  NULL,              0     },
  {"truedam",        FALSE, &trs_disk_truedam, TRUE  },
  {"notruedam",      FALSE, &trs_disk_truedam, FALSE },
  {"samplerate",     TRUE,  NULL,              0     },
  {"serial",         TRUE,  NULL,              0     },
  {"switches",       TRUE,  NULL,              0     },
  {"emtsafe",        FALSE, &trs_emtsafe,      TRUE  },
  {"noemtsafe",      FALSE, &trs_emtsafe,      FALSE },
  {NULL, 0, 0, 0}
};

static int find_opt_match(const char *arg, const struct option *longopts) {
  int i;
  for(i=0; longopts && longopts->name; longopts++, i++) {
    if (strcmp(arg, longopts->name) == 0) {
      return i;
    }
  }
  return -1;
}

static int getopt_long_only(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex) {

  const char *cur;
  int match_index;
  static const struct option * match;
  if (optind <= 1) {
    optind = 1;
  }
  for(;optind < argc;) {
    cur  = argv[optind++];
    joshlog("on %s\n", cur);
    if (cur[0] != '-') {
      fatal("Bad option: %s",cur);
    }
    cur += 1;
    match_index = find_opt_match(cur, longopts);
    if (match_index < 0) {
      fatal("Bad option: -%s",cur);
      return -1;
    }
    match = longopts + match_index;
    *longindex = match_index;

    if (match->has_arg) {
      if (optind>=argc) {
        fatal("Missing argument to -%s",cur);
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
trs_parse_command_line(int argc, char **argv, int *debug)
{
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
    unsigned int real_addr = ((unsigned long)cps) * 16 + cpo;
    unsigned int real_page = real_addr & (~4095);
    unsigned int real_offset = real_addr - real_page;
    printf("A %x %x %x\n", real_addr, real_page, real_offset);


    char * p;
    p = malloc(3*4096);
    p += 4096 - (((unsigned int)p) & 4095);

    printf("A %p %d\n", p, errno);
    int x = -1;
    printf("B %x %p %x %d\n", real_page, p,x,errno);
    x = __djgpp_map_physical_memory(p, 8192, real_page);
    printf("C %x %p %x %d\n", real_page, p,x,errno);


    pScanBuffer = (struct scan_buffer *)(p + real_offset);

    pScanBuffer->suppress_flag = 1;
    sleep(1);
    scanBufferCursor = pScanBuffer->next_offset;
  }

  trs_model = 1;

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
  char* home = getenv("HOME");
  if (home) {
    char *p = (char*)malloc(strlen(home) + strlen(trs_disk_dir) + 1);
    sprintf(p, "%s/%s", home, trs_disk_dir+1);
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
      opt_charset = "wider"; /* default */
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
      } else {
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
  trs_charset = 4 + 3*(trs_model > 3);
      } else if (opt_charset[0] == 'i'/*international*/) {
  trs_charset = 5 + 3*(trs_model > 3);
      } else if (opt_charset[0] == 'b'/*bold*/) {
  trs_charset = 6 + 3*(trs_model > 3);
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
trs_load_romfile()
{
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

  case 3: case 4:
    if (opt_romfile3) {
      romfile = opt_romfile3;
#ifdef DEFAULT_ROM3
    } else if (stat(DEFAULT_ROM3, &statbuf) == 0) {
      romfile = DEFAULT_ROM3;
#endif
    }
    if (romfile != NULL) {
      trs_load_rom(romfile);
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


