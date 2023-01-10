
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

#include <dirent.h>  
#include <assert.h>
#include <io.h>
//#include <dpmi.h>
#include "trs_metafile.h"
#include "trs_djgpp.h"

static int min(int x, int y) {
    return x < y ? x : y;
}

#define ENTRY_TYPE_NONE 0
#define ENTRY_TYPE_CASSETTE 1
#define ENTRY_TYPE_SUBDIR 2
#define ENTRY_TYPE_PARENTDIR 3

typedef char cassette_instructions[256];
typedef char cassette_description[256];
typedef char cassette_entry_fn[sizeof(cassette_filename_buffer) - 16];
typedef char cassette_simple_filename[128];

typedef char cassette_display_name[32];

typedef struct cassette_entry {
    cassette_display_name display_name;
    cassette_simple_filename filename;
    unsigned char entry_type;
} cassette_entry;

typedef struct cassette_table {
    unsigned int size;
    cassette_entry pEntries[];
} cassette_table;


static unsigned int get_file_length(const char *fn) {
    FILE *f;
    int fhandle;
    long long len;

    f = fopen(fn, "rb");
    if (!f) {
        return 0;
    }
    fhandle = fileno(f);
    len = lfilelength(fhandle);
    fclose(f);
    if (len == -1LL) {
        return 0;
    }
    if (len >  0xFFFFFFFFLL) {
        return 0xFFFFFFFF;
    }
    return (unsigned int)len;
}

static unsigned char get_cassette_entry_type(struct dirent *dir) {
    if (!dir) {
        return ENTRY_TYPE_NONE;
    }
    if (dir->d_type == DT_DIR &&  dir->d_namlen > 1 && dir->d_name[0] != '.' ) {
        return ENTRY_TYPE_SUBDIR;
    }
    if (dir->d_type != DT_REG) {
        return ENTRY_TYPE_NONE;
    }
    if (dir -> d_namlen < 4) {
        return ENTRY_TYPE_NONE;
    }
    char *p;
    p = (dir -> d_name) + (dir ->d_namlen) - 1;
    if (tolower(*(p--)) != 's')
        return 0;
    if (tolower(*(p--)) != 'a')
        return 0;
    if (tolower(*(p--)) != 'c')
        return 0;
    if (tolower(*(p--)) != '.')
        return 0;

    return ENTRY_TYPE_CASSETTE;

}

static void init_entry(cassette_entry *entry, struct dirent *de) {
    assert(entry != 0);
    memset(entry, 0, sizeof(cassette_entry));
    if (!de) {
        //Sentinel de==null --> is parentdir
        entry->entry_type = ENTRY_TYPE_PARENTDIR;
        strcpy(entry->display_name,"( Parent Dir )");
        strcpy(entry->filename, "..");
        return;
    }

    unsigned char entry_type = get_cassette_entry_type(de);
    assert(entry_type != ENTRY_TYPE_NONE);
    entry->entry_type = entry_type;
    strncpy(entry->filename, de->d_name, sizeof(cassette_simple_filename));
    entry->filename[sizeof(cassette_simple_filename)-1] = 0;

    if (entry_type == ENTRY_TYPE_CASSETTE) {
        strncpy(entry->display_name, de->d_name, sizeof(cassette_display_name));
        entry->display_name[sizeof(cassette_display_name)-1] = 0;
        char *p = strrchr(entry->display_name, '.');
        //Chop off extension in display name
        if (p) {
            *p = 0;
        }
    } else {
        const char * prefix  = "< ";
        const char * suffix = " >";
        const unsigned int room = sizeof(cassette_display_name) - strlen(prefix) - strlen(suffix);
        strcpy(entry->display_name, prefix);
        strncpy(entry->display_name + strlen(prefix), de->d_name, room - strlen(prefix));
        entry->display_name[sizeof(cassette_display_name)-1-strlen(suffix)] = 0;
        strcat(entry->display_name, suffix);
    }
    unsigned int fnl = strlen(entry->display_name);
    for(unsigned int i=0;i<fnl;i++) {
        entry->display_name[i] = toupper(entry->display_name[i]);
    }
}

static int compare_cassette_entries(const cassette_entry *e1, const cassette_entry *e2) {
    if (e1->display_name[0] == '(') {
        return e2->display_name[0] == '(' ? 0 : 1;
    }
    if (e2->display_name[0] == '(') {
        return -1;
    }
    return stricmp(e1->display_name, e2->display_name);
}

static cassette_table * load_cassette_table(char *base_dir, int nestCount) {
   struct dirent *de;
   DIR *d;
   int tableSize = 0;
   cassette_table *pRes;

   d = opendir(base_dir);
   if (!d) {
    joshlog("opendir failed\n");
    return 0;
   }
   while ((de = readdir(d))) {
      if (get_cassette_entry_type(de)) {
        tableSize ++;
      }
   }
   closedir(d);
   if (nestCount) {
       tableSize ++;
   }

   pRes = (cassette_table *)malloc(sizeof(cassette_table) + tableSize * sizeof(cassette_entry));
   if (!pRes) {
    return 0;
   }
   memset(pRes, 0 , sizeof(cassette_table) + tableSize * sizeof(cassette_entry));

   d = opendir(base_dir);
   if (!d) {
    joshlog("opendir failed 2\n");
    free(pRes);
    return 0;
   }
   int ts2 = 0;
   if (nestCount) {
       init_entry(pRes->pEntries, 0);
       ts2 += 1;
   }
   while ((de = readdir(d)) && ts2 < tableSize) {
      if (get_cassette_entry_type(de)) {
        init_entry(pRes->pEntries + ts2, de);
        ts2 += 1;
      }
   }
   closedir(d);

   qsort(pRes->pEntries, ts2, sizeof(cassette_entry), (int (*)(const void *, const void *))compare_cassette_entries);
   pRes->size = ts2;





   return pRes;
}


static struct mem_block *load_cassette_meta(const char *base_dir, const char *cassette_filename) {
    char fn[2000];
    strncpy(fn, base_dir, sizeof(fn)-2);
    fn[sizeof(fn) - 1] = 0;
    unsigned long dirlen = strlen(base_dir);
    if (dirlen) {
        char ec = fn[dirlen-1];
        if (!(ec == '/' || ec == '\\')) {
            fn[dirlen++]='/';
            fn[dirlen]=0;
        }
    }

    strncpy(fn + dirlen, cassette_filename, sizeof(fn) - dirlen);
    fn[sizeof(fn) - 1] = 0;
    unsigned long fulllen = strlen(fn);
    if (fulllen > 4 && stricmp(".cas",fn + (fulllen - 4)) == 0) {
        fn[fulllen-3] = 'M';
        fn[fulllen-2] = 'E';
        fn[fulllen-1] = 'T';
    } else if (fulllen < (sizeof(fn) - 5)) {
        strcat(fn, ".MET");
    } else {
        return 0;
    }
    joshlog("Looking for metafile: %s\n", fn);
    if (!__file_exists(fn)){
        return 0;
    }
    joshlog("metafile: %s exists!\n", fn);
    return read_mem_block_from_file(fn);
}

static int choose_cassette(cassette_entry_fn pDest) {
    assert(pDest != 0);
   int x,y;
   int insety = 50;
   int insetx = 80;
   int return_value = 0;
   typedef const char *choice;
   char baseDir[sizeof(cassette_filename_buffer)];
   strncpy(baseDir, cassette_base_directory, sizeof(cassette_filename_buffer));
   baseDir[sizeof(cassette_filename_buffer) - 1 ] = 0;
   const unsigned long initial_basedir_len = strlen(baseDir);
   int dirLevels = 0;

   static const choice choices[]={"(B)ack", "(S)elect", "(C)ancel", "(N)ext"};
   static const char *chooseMsg = "Choose Cassette";
   cassette_table *pTable = 0;
   int currentEntry = 0;
   GrClearScreen(GrBlack());
   GrTextOption grt, baset;
   for(;;) {
       if (!pTable) {
           pTable = load_cassette_table(baseDir,dirLevels);
           if (!pTable) {
               joshlog("ERR: Failed to load_cassette_table\n");
               goto done;
           }
           if (!pTable->size) {
               joshlog("ERR: No cassettes found\n");
               goto done;
           }
           currentEntry = 0;
       }
       // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
       // so I don't have to use the text measurement functions.
       grt.txo_font = &GrFont_PC8x14;
       grt.txo_fgcolor.v = COLOR_PRIMARY;
       grt.txo_bgcolor.v = GrBlack();
       grt.txo_direct = GR_TEXT_RIGHT;
       grt.txo_xalign = GR_ALIGN_CENTER;
       grt.txo_yalign = GR_ALIGN_CENTER;
       grt.txo_chrtype = GR_BYTE_TEXT;
       baset = grt;


       GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
       GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,COLOR_BORDER );
       GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,COLOR_BORDER );
     
       x = GrMaxX()/2;
       y = GrMaxY()/2;

       GrDrawString( (void*)chooseMsg,strlen( chooseMsg),x,y-30,&grt );
       grt.txo_fgcolor.v = GrWhite();
       GrDrawString( pTable->pEntries[currentEntry].display_name,strlen( pTable->pEntries[currentEntry].display_name ),x,y-10,&grt );
       grt = baset;


       /** DISPLAY DESCRIPTION */
       grt.txo_font = &GrFont_PC6x8;
       grt.txo_fgcolor.v = COLOR_SECONDARY;
       if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_CASSETTE || pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_SUBDIR) {
           struct mem_block *pmeta = load_cassette_meta(baseDir, pTable->pEntries[currentEntry].filename);
           if (pmeta) {
               joshlog("Looking for description...\n");
               boundary b = find_meta_data(pmeta, "description");
               if (b.end > b.start) {
                   grt.txo_font = &GrFont_PC6x8;
                   grt.txo_fgcolor.v = COLOR_SECONDARY;
                   GrDrawString(pmeta->data + b.start, b.end - b.start, x, y+5, &grt);
               }

               free_mem_block(pmeta);
           }
       } else if (pTable->pEntries[currentEntry].entry_type == ENTRY_TYPE_PARENTDIR) {
           char pdmsg[64];
           strcpy(pdmsg, "Return to the previous directory");
           GrDrawString(pdmsg, (int)strlen(pdmsg), x, y+5, &grt);
       }
       grt=baset;
       /** END DISPLAY DESCRIPTION */



       if (dirLevels) {
           unsigned long curdirlen = strlen(baseDir);
           if (curdirlen > initial_basedir_len) {
               grt.txo_font = &GrFont_PC6x8;
               grt.txo_xalign = GR_ALIGN_LEFT;
               grt.txo_fgcolor.v = COLOR_SECONDARY;
               GrDrawString(baseDir + initial_basedir_len, curdirlen - initial_basedir_len, insetx + 14, insety+14, &grt);
               grt = baset;
           }
       }

       for(int ii=0;ii<4;ii++) {
           GrDrawString( (void*)choices[ii],strlen(choices[ii]),x-180 + ii * 120,y+35,&grt );
       }

       int choice = -1;
       for(;choice < 0;) {
         GrKeyType key;
         key = GrKeyRead();
         //joshlog("KEY %u\n", key);
         if (key >= 'a' && key <= 'z') {
            key = key + ('A' - 'a');
         }
         switch(key) {
            case GrKey_Up:
                //HACK - Up arrow key is like left but brings us all the way to the top
                currentEntry = 0;
                choice = 0;
                break;
            case GrKey_Left:
            case 'B': choice = 0; break;
            case GrKey_Down:
                //HACK - Down arrow key is like right but brings us all the way to the top
                currentEntry = pTable->size - 1;
                choice = 3;
                break;
            case GrKey_Right:
            case 'N': choice = 3; break;
            case GrKey_Return:
            case 'S': choice = 1; break;
            case GrKey_Escape:
            case 'C': choice = 2; break;
             default: choice = -1;
         }
       }
       grt.txo_fgcolor.v = GrBlack();
       grt.txo_bgcolor.v = GrWhite();
       GrDrawString( (void*)choices[choice],strlen(choices[choice]),x-180 + choice * 120,y+35,&grt );
       usleep(50000);

       if (choice == 0) {
         if (currentEntry > 0)
           currentEntry --;
       } else if (choice == 3) {
         if (currentEntry <(pTable->size - 1))
           currentEntry ++;
       } else if (choice == 1) {
           //select
           cassette_entry *pSel = pTable->pEntries+currentEntry;
           if (pSel->entry_type == ENTRY_TYPE_CASSETTE) {
               strcpy(pDest, baseDir);
               strcat(pDest, "\\");
               strcat(pDest, pSel->filename);
               return_value = 1;
               break;
           } else if (pSel->entry_type == ENTRY_TYPE_SUBDIR) {
               strcat(baseDir, "\\");
               strcat(baseDir, pSel->filename);
               free(pTable);
               pTable = 0;
               dirLevels++;
               continue;
           } else if (pSel->entry_type == ENTRY_TYPE_PARENTDIR && dirLevels > 0) {
               char *p = strrchr(baseDir, '\\');
               if (p) {
                   *p = 0;
                   dirLevels--;
                   free(pTable);
                   pTable = 0;
               }
               joshlog("%s %d\n", baseDir, dirLevels);
           }
           continue;
       } else {
         //cancel
         break;
       }

       //grt.txo_fgcolor.v = GrBlack();
       //grt.txo_bgcolor.v = GrWhite();
       //if (key == 'Y') GrDrawString( "(Y)es",5,x-80,y+20,&grt );
       //else GrDrawString( "(N)o",5,x+80,y+20,&grt );
       //usleep(100000);

   }



 
  


 done:
   if (pTable) {
       free(pTable);
   }
   return return_value;
}


static void yesno_message_handler(joshem_modal_context *pContext);

int joshem_modal_ask_yn(const char *pPrompt) {
    void * c = (void *)(pPrompt ? pPrompt : "<NULL>");
    return joshem_do_modal(yesno_message_handler, c);
}


static void yesno_message_handler(joshem_modal_context *pContext) {
   char *message;
   int x,y;
   int insety = 50;
   int insetx = 80;
   message =  (char *)(pContext->input);
   GrTextOption grt;
 
      GrClearScreen(GrBlack());


   // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
   // so I don't have to use the text measurement functions.
   grt.txo_font = &GrFont_PC8x14;
   grt.txo_fgcolor.v = COLOR_PRIMARY;
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;


   GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
   GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,COLOR_BORDER);
   GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,COLOR_BORDER );
 
   x = GrMaxX()/2;
   y = GrMaxY()/2;
    joshlog("II %s %d %d\n",message, x, y);

   GrDrawString( message,strlen( message ),x,y-10,&grt );
   GrDrawString( "(Y)es",5,x-80,y+20,&grt );
   GrDrawString( "(N)o",5,x+80,y+20,&grt );

   GrKeyType key;
   for(;;) {
     key = GrKeyRead();
     if (key >= 'a' && key <= 'z') {
        key = key + ('A' - 'a');
     }
     if (key == 'Y' || key == 'N') {
        break;
     }
   }

   grt.txo_fgcolor.v = GrBlack();
   grt.txo_bgcolor.v = GrWhite();
   if (key == 'Y') GrDrawString( "(Y)es",5,x-80,y+20,&grt );
   else GrDrawString( "(N)o",5,x+80,y+20,&grt );
   usleep(100000);
   pContext->result = (key == 'Y') ? 1 : 0;

}

static void message_message_handler(joshem_modal_context *pContext);

void joshem_modal_message(const char *pPrompt) {
    void * c = (void *)(pPrompt ? pPrompt : "<NULL>");
    joshem_do_modal(message_message_handler, c);
}


static void message_message_handler(joshem_modal_context *pContext) {
   char *message;
   int x,y;
   int insety = 50;
   int insetx = 80;
   message =  (char *)(pContext->input);
   GrTextOption grt;
 
      GrClearScreen(GrBlack());


   // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
   // so I don't have to use the text measurement functions.
   grt.txo_font = &GrFont_PC8x14;
   grt.txo_fgcolor.v = COLOR_PRIMARY;
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;


   GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
   GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,COLOR_BORDER );
   GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,COLOR_BORDER );
 
   x = GrMaxX()/2;
   y = GrMaxY()/2;
    joshlog("MSG %s %d %d\n",message, x, y);

   GrDrawString( message,strlen( message ),x,y-10,&grt );
   GrDrawString( "Press (Enter)",13,x,y+20,&grt );

   GrKeyType key;
   for(;;) {
     key = GrKeyRead();
     if (key == GrKey_Return) {
        break;
     }
   }

   pContext->result = 0;

}




static int ask_question(const char *prompt, char *dest, int maxLen) {
   int x,y;
   int insety = 50;
   int insetx = 80;
    int retVal = 0;

   char *buf = maxLen > 0 ? malloc(maxLen + 2) : 0;
    if (!buf) {
        joshlog("ERR: ask_question alloc failed\n");
        return 0;
    }
    buf[0] = 0;
    int cur_len = 0;

   GrTextOption grt;
 
   GrClearScreen(GrBlack());


   // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
   // so I don't have to use the text measurement functions.
   grt.txo_font = &GrFont_PC8x14;
   grt.txo_fgcolor.v = COLOR_PRIMARY;
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;


   GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
   GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,COLOR_BORDER );
   GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,COLOR_BORDER );

   for(;;) {
       x = GrMaxX()/2;
       y = GrMaxY()/2;
       GrFilledBox(x-60,y-10,x+60,y+10,GrBlack());
       GrBox(x-60,y-10,x+60,y+10,GrWhite());

       grt.txo_xalign = GR_ALIGN_LEFT;
       buf[cur_len]=0xB0;
       buf[cur_len+1] = 0;
       GrDrawString(buf,cur_len+1, x-50, y, &grt);
       grt.txo_xalign = GR_ALIGN_CENTER;


       GrDrawString( (void*)prompt,strlen( prompt ),x,y-30,&grt );
       GrDrawString( "(Enter) to accept",17,x-150,y+30,&grt );
       GrDrawString( "(Esc) to cancel",15,x+150,y+30,&grt );

       GrKeyType key;
       for(;;) {
         key = GrKeyRead();
         if (key >= 'a' && key <= 'z') {
            key = key + ('A' - 'a');
         }
         if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key=='_'
            || key == GrKey_BackSpace || key == GrKey_Return || key == GrKey_Escape)
            break;
       }
       if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key=='_') {
         if (cur_len < maxLen) {
             buf[cur_len++] = key;
         }
       } else if (key == GrKey_BackSpace) {
         if (cur_len > 0)
            cur_len --;
       } else if (key == GrKey_Return && cur_len > 0) {
           retVal = 1;
         break;
       } else if (key == GrKey_Escape) {
           retVal = 0;
           break;
       }

   }
    if (retVal) {
        memcpy(dest, buf, cur_len);
        dest[cur_len] = 0;
    }
    if (buf)
        free(buf);
    return retVal;
}




static void cassette_control_handler(joshem_modal_context *pContext);

void joshem_cassette_control(joshem_cassette_control_args *pArgs) {
    joshem_cassette_control_args dmy;

    if (!pArgs) { 
        memset(&dmy, 0, sizeof(dmy));
        strcpy(dmy.cassette_filename,"Hello");
        pArgs = &dmy;
    }

    
    joshem_do_modal(cassette_control_handler, (void *)pArgs);
}




static void draw_cassette(joshem_cassette_control_args *pArgs) {
   int midx, midy;
   midx = GrMaxX() / 2;
   midy = GrMaxY() / 2;
   int ry = midy - 50;
   int rx = midx - 150;
   GrFilledBox( midx - rx, midy - ry, midx + rx, midy + ry,GrBlack() );
   GrBox(  midx - rx, midy - ry, midx + rx, midy + ry,GrWhite() );
   GrEllipse( midx - rx / 2, midy - ry/5, rx / 7, ry / 4 , GrWhite());
   GrEllipse( midx + rx / 2, midy - ry/5, rx / 7, ry / 4 , GrWhite());


   int bx1 = rx * 3 / 4;
   int bx2 = bx1 - rx / 10;
   int by1 = ry;
   int by2 = by1  - ry *  5 / 10;
   int poly[][2] = {
    {midx - bx1, midy + by1}
    ,{midx - bx2, midy + by2}
    ,{midx + bx2, midy + by2}
    ,{midx + bx1, midy + by1}
   };
   GrPolygon(4, poly, GrWhite());
   //GrLine(poly[0][0], poly[0][1], poly[1][0], poly[1][1], GrWhite());

   GrTextOption grt;
   grt.txo_font = &GrFont_PC8x14;
   grt.txo_fgcolor.v = GrWhite();
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;
   GrDrawString( pArgs->cassette_filename,min(10,strlen( pArgs->cassette_filename )),midx,midy-30,&grt );


}

static void cassette_control_handler(joshem_modal_context *pContext) {
   joshem_cassette_control_args *pArgs = pContext->input;
    cassette_entry_fn e;
    if (!pArgs) {
        return;
    }
    if (pArgs ->write_requested) {
        if (ask_question("Save to which cassette?", e, 8)) {
            sprintf(pArgs->cassette_filename, "%s.CAS", e);
        } else {
            strcpy(pArgs->cassette_filename, "_NOCAS.DMP");
        }
        pArgs ->cassette_format = 1;
        pArgs->cassette_position = get_file_length(pArgs->cassette_filename);
        pArgs ->cassette_writable = 1;

        return;
    }
   //ask_question_message_handler(0);
   if (choose_cassette(e)) {
    strcpy(pArgs->cassette_filename, e);
    pArgs->cassette_position = 0;
    pArgs->cassette_format = 1;
    pArgs->cassette_writable = 0;
    pArgs->initial_selection = 0;
   }
   //draw_cassette(pArgs);

   //GrKeyRead();
}

