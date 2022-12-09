
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
  
//#include <dpmi.h>

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
 
  

   grt.txo_font = &GrDefaultFont;
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
     if (key >= 'a' || key <= 'z') {
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


}
