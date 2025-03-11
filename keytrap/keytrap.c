#include "dos.h"
#include "stdlib.h"
#include "process.h"

#include "scanbuf.h"


static void interrupt (*oldfunc) ();


static volatile struct scan_buffer g_buffer;



/**
 * POC - Hook Int 15H, subfunction 4FH
 *    To leave current character alone, preseve AL do IRET
 *    To change current character, change AL scan code, do IRET
 *    To suppress scan code, clear carrt, and do retf2
 * 
 * Note - assume TINY model, so can use CS as DS in the interrupt hook
 *   
 */


void silly() {
    asm BUFFER EQU (OFFSET g_buffer);
    asm O_OLDFUNC EQU (OFFSET oldfunc)

    asm pushf;            /* Flags are pushed! Safe to change flags! */
    asm cmp ah, 0x4f;
    asm je trap_code;
    asm popf;
    asm jmp dword ptr cs:O_OLDFUNC;


    trap_code:
    asm push bx;           /* BX is pushed!  Save to change BX (and bh and bl) */

    /* Add scan code to the ring buffer */
    asm mov bl, byte ptr cs:BUFFER.next_offset;
    asm mov bh, 0;
    asm mov byte ptr cs:BUFFER.key_ring[bx], al;
    asm inc bl;
    asm mov byte ptr cs:BUFFER.next_offset, bl;

    /* Update the code map */
    asm mov bl, al;
    asm and bl, 0x7f;

    asm test al, 0x80;
    asm jnz set_break_state;
    asm test byte ptr cs:BUFFER.key_states[bx], 0xFF;
    asm jz set_initial_make_state;
    /* On autorepeat, increment but ensure high bit always set */
    set_autorepeat_make_state:
    asm inc byte ptr cs:BUFFER.key_states[bx];
    asm or  byte ptr cs:BUFFER.key_states[bx], 0x80;
    asm jmp suppress_if_flagged;
    set_initial_make_state:
    asm mov byte ptr cs:BUFFER.key_states[bx], 1;
    asm jmp suppress_if_flagged;
    set_break_state:
    asm mov byte ptr cs:BUFFER.key_states[bx], 0;

    suppress_if_flagged:
    asm mov bl, byte ptr cs: BUFFER.suppress_flag;
    asm test bl, bl;
    asm jz cleanup_no_suppress;

    cleanup:
    asm pop bx;
    asm popf;
    asm clc;
    asm retf 2;

    cleanup_no_suppress:
    asm pop bx;
    asm popf;
    asm iret;


/* To leave scan code alone, just do an iret, which will restore flags! */
/* To change scan code, update al before returning with iret */
/* To suppress scan code, clear the carry flag and then do a "retf 2". so that we take the old flags
   off the stack but do not set them in the new flags register */


    messing_around_can_delete:
    asm mov    ah, 88H;
    asm or bl, bl;

    asm mov dx, 1;
    asm mov ax, 0;
    asm ret;

    asm jne failed_xms_call;

    asm db '1234567';
    asm sub ax, 400H;
    asm sbb dx, 0H;
    asm ret;
failed_xms_call:
    asm xor ax, ax;
    asm xor dx, dx;
    asm ret;
}


void wait_for_keybord_ready() {
        printf("Wait ready\n");

    while(inportb(0x64) & 2) {
        /* wait */
    }    
}

void wait_for_keybord_input() {
        printf("Wait input\n");
    while(!(inportb(0x64) & 1)) {
        /* wait */
    }    
}

void setup_keyboard() {
    unsigned char z;
    wait_for_keybord_ready();
    outportb(0x64, 0x20);
    /* wait_for_keybord_input(); */
    z = inportb(0x64);
    printf("%x\n", z);
    z = inportb(0x60);
    printf("%x\n", z);

}

/*  */
void main(int argc, char **argv, char **env) {


    void interrupt (*zz)();
    unsigned char cursor;
    unsigned int scan;
    unsigned int c,i;
    char tbuf[32];

    /*
    printf("ROMPATH=%s\n", getenv("ROMPATH") ? getenv("ROMPATH") : "<null>");
    printf("PATH=%s\n", getenv("PATH") ? getenv("PATH") : "<null>");
    */
    
    if (argc < 2) {
        printf("ERROR: You need to specify a command to spawn\n");
        exit(1);
    }

    zz = silly;
    g_buffer.next_offset = 0;

    /*
    printf("Hello\n");
    setup_keyboard();
    printf("Set Up\n");
    */

    oldfunc = getvect(0x15);
    setvect(0x15, zz); 

    /*
    printf("Here\n");
    printf("%p\n", &g_buffer);
    printf("%Fp\n", (void far *)(&g_buffer));
    */
    sprintf(tbuf, "CHARPEEK=%Fp", (void far *)(&g_buffer));
    /*
    printf("%s\n",tbuf);
    */
    putenv(tbuf);

    
    /*
    g_buffer.suppress_flag = 1;
    cursor = g_buffer.next_offset;
    for(i=0;!g_buffer.key_states[0x35];) {
        if (cursor != g_buffer.next_offset) {
            i++;
            scan = g_buffer.key_ring[cursor];
            cursor++;
            printf("%06d %02x %02X\n", i, scan, g_buffer.key_states[0x1E]);
        }
    }
    g_buffer.suppress_flag = 0;
    */

    spawnvp(P_WAIT, argv[1], argv + 1);

    /* spawnl(P_WAIT, "z:\\command.com", 0); */

    printf("Huh?\n");





    /* If were going to TSR, we'd do this...
    /* TODO: We don't need to keep this much memory! */
    /* keep(0, 4096); *//* Exits here - stuff below won't run */


    /* But we're not a TSR, so remove our hook and get out */
    printf("Hi!");
    setvect(0x15, oldfunc);
    return;
    
}