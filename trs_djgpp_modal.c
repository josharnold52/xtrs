
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

#include <dirent.h>  
#include <assert.h>
//#include <dpmi.h>

static int min(int x, int y) {
    return x < y ? x : y;
}


typedef char cassette_instructions[256];
typedef char cassette_description[256];
typedef char cassette_entry_fn[sizeof(cassette_filename_buffer) - 16];

typedef struct cassette_entry {
    cassette_entry_fn filename;
    int metadata_state;
    cassette_instructions instructions;
    cassette_description description;
} cassette_entry;

typedef struct cassette_table {
    unsigned int size;
    cassette_entry pEntries[];
} cassette_table;


static int is_cassette_file(struct dirent *dir) {
    if (!dir) {
        return 0;
    }
    if (dir->d_type != DT_REG) {
        return 0;
    }
    if (dir -> d_namlen < 4) {
        return 0;
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

    return 1;

}

static void init_entry(cassette_entry *entry, struct dirent *de) {
    assert(entry != 0);
    memset(entry, 0, sizeof(cassette_entry));
    int elen;
    elen = min(strlen(de->d_name), sizeof(cassette_entry_fn) - 1);
    memcpy(entry->filename, de->d_name, elen);
    entry->filename[elen-4] = 0;
    for(int i=0;i<elen;i++) {
        entry->filename[i] = toupper(entry->filename[i]);
    }
}

static cassette_table * load_cassette_table() {
   struct dirent *de;
   DIR *d;
   int tableSize = 0;
   cassette_table *pRes;

   d = opendir(".");
   if (!d) {
    joshlog("opendir failed\n");
    return 0;
   }
   while ((de = readdir(d))) {
      if (is_cassette_file(de)) {
        tableSize ++;
      }
   }
   closedir(d);

   pRes = (cassette_table *)malloc(sizeof(cassette_table) + tableSize * sizeof(cassette_entry));
   if (!pRes) {
    return 0;
   }

   int ts2 = 0;
   d = opendir(".");
   if (!d) {
    joshlog("opendir failed 2\n");
    free(pRes);
    return 0;
   }
   while ((de = readdir(d)) && ts2 < tableSize) {
      if (is_cassette_file(de)) {
        init_entry(pRes->pEntries + ts2, de);
        ts2 += 1;
      }
   }
   closedir(d);
   pRes->size = ts2;





   return pRes;
}



static int choose_cassette(cassette_entry *pDest) {
   int x,y;
   int insety = 50;
   int insetx = 80;
   int return_value = 0;
   cassette_table *pTable = 0;
   typedef const char *ccc;


   static const ccc choices[]={"(B)ack", "(S)elect", "(C)ancel", "(N)ext"};
   static const char *chooseMsg = "Choose Cassette";
   pTable = load_cassette_table();
   if (!pTable) {
    joshlog("ERR: Failed to load_cassette_table\n");
    goto done;
   }
   if (!pTable->size) {
    joshlog("ERR: No cassettes found\n");
    goto done;
   }
    GrClearScreen(GrBlack());
   GrTextOption grt;
   int currentEntry = 0;
   for(;;) {
       // Use the GrFont_PC8x14 - presumably i can assume 8 pixels wide and 14 pixels high
       // so I don't have to use the text measurement functions.
       grt.txo_font = &GrFont_PC8x14;
       grt.txo_fgcolor.v = GrWhite();
       grt.txo_bgcolor.v = GrBlack();
       grt.txo_direct = GR_TEXT_RIGHT;
       grt.txo_xalign = GR_ALIGN_CENTER;
       grt.txo_yalign = GR_ALIGN_CENTER;
       grt.txo_chrtype = GR_BYTE_TEXT;


       GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
       GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrWhite() );
       GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,GrWhite() );
     
       x = GrMaxX()/2;
       y = GrMaxY()/2;

       GrDrawString( (void*)chooseMsg,strlen( chooseMsg),x,y-30,&grt );
       GrDrawString( pTable->pEntries[currentEntry].filename,strlen( pTable->pEntries[currentEntry].filename ),x,y-10,&grt );
       for(int ii=0;ii<4;ii++) {
           GrDrawString( (void*)choices[ii],strlen(choices[ii]),x-180 + ii * 120,y+35,&grt );
       }
       //GrDrawString( "(B)ack",6,x-180,y+35,&grt );
       //GrDrawString( "(N)ext",6,x+180,y+35,&grt );
       //GrDrawString( "(S)elect",8,x-60,y+35,&grt );
       //GrDrawString( "(C)ancel",8,x+60,y+35,&grt );

       int choice = -1;
       for(;choice < 0;) {
         GrKeyType key;
         key = GrKeyRead();
         joshlog("KEY %u\n", key);
         if (key >= 'a' && key <= 'z') {
            key = key + ('A' - 'a');
         }
         switch(key) {
            case GrKey_Left:
            case 'B': choice = 0; break;
            case GrKey_Right:
            case 'N': choice = 3; break;
            case GrKey_Return:
            case 'S': choice = 1; break;
            case GrKey_Escape:
            case 'C': choice = 2; break;
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
         if (pDest) {
            memcpy(pDest, pTable->pEntries+currentEntry, sizeof(cassette_entry));
         }
         return_value = 1;
         //select
         break;
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
   if (pTable) 
    free(pTable);
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
   grt.txo_fgcolor.v = GrWhite();
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;


   GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
   GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrWhite() );
   GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,GrWhite() );
 
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
   grt.txo_fgcolor.v = GrWhite();
   grt.txo_bgcolor.v = GrBlack();
   grt.txo_direct = GR_TEXT_RIGHT;
   grt.txo_xalign = GR_ALIGN_CENTER;
   grt.txo_yalign = GR_ALIGN_CENTER;
   grt.txo_chrtype = GR_BYTE_TEXT;


   GrFilledBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrBlack() );
   GrBox( insetx,insety,GrMaxX() - insetx,GrMaxY() - insety,GrWhite() );
   GrBox( insetx + 4,insety + 4,GrMaxX()-insetx-4,GrMaxY()-insety-4,GrWhite() );

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
       GrDrawString( "(Y)es",5,x-80,y+20,&grt );
       GrDrawString( "(N)o",5,x+80,y+20,&grt );

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
   cassette_entry e;
    if (!pArgs) {
        return;
    }
    if (pArgs ->write_requested) {
        if (ask_question("Save to which cassette?", e.filename, 8)) {
            sprintf(pArgs->cassette_filename, "%s.CAS", e.filename);
            pArgs ->cassette_format = 1;
            pArgs -> cassette_position = 0;
            pArgs ->cassette_writable = 1;
        } else {
            pArgs ->cassette_writable = 0;
        }
        return;
    }
   //ask_question_message_handler(0);
   if (choose_cassette(&e)) {
    sprintf(pArgs->cassette_filename, "%s.CAS", e.filename);
    pArgs->cassette_position = 0;
    pArgs->cassette_format = 1;
    pArgs->cassette_writable = 0;
   }
   //draw_cassette(pArgs);

   //GrKeyRead();
}

