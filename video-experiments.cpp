//
// Created by arnold on 11/6/23.
//

#include <cstdio>
#include <dpmi.h>
#include <cstring>
#include <go32.h>
#include <sys/farptr.h>
#include <unistd.h>


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

void setCGAHighRes() {
    __dpmi_regs regs;
    prepare_bios_regs(&regs);
    regs.h.ah  = 0;
    regs.h.al = 6;
    __dpmi_simulate_real_mode_interrupt(0x10, &regs);
}


int main(int argc, char **argv) {
    setCGAHighRes();
    for(int i=0;i<0x2000;i++) {
        _farpokeb(_dos_ds, 0xB8000 + i, 0xFF & i);
    }
    sleep(10);

    printf("Done\n");
    return 0;
}