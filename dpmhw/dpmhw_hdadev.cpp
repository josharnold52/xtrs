#include <unistd.h>
#include "dpmhw_hdadev.h"
#include "dpmhw_impl.h"

using dpmhw::HdaDevice;

dpmhw::HdaDeviceType dpmhw::hdaGetDeviceType(dpmhw::PciFunction &pciFunction) {
    if (pciFunction.getConfig16(0x02) == 0x811B) {
        return dpmhw::HdaDeviceType::intelSch;
    }
    return dpmhw::HdaDeviceType::other;
}

bool HdaDevice::activate() {
    if (!allocationSucceeded.get()) {
        dpmhw_log("ERROR: Failed to activate HDA because allocation was not fuccessful\n");
        return false;
    }
    force_reset();
    dpmhw_debug("Bringing HDA device out of reset...\n");
    WAKEEN.poke(0);
    GCTL.poke(1);
    dpmhw_debug("Waiting for device...\n");
    usleep(600);
    while((GCTL.peek() & 0x1) == 0) INLINE_PAUSE;
    dpmhw_log("HDA Device is out of reset\n");

    //TODO: Enable PCI bus mastering here or before hand??
    dpmhw_log("HDA DMAPOS dmaOffset=0x%x, phys=0x%08X\n",
              dmaPosDma.selectorAddress, dmaPosDma.physicalAddress);
    for(int zb=0;zb<dmaPosDma.size;zb+=4) {
        dmaPosDma.selector.poke32(dmaPosDma.selectorAddress + zb, 0xBADF00D);
    }
    DPUBASE.poke(0);
    DPLBASE.poke(dmaPosDma.physicalAddress | 1);


    unsigned char corbSizeCap = CORBSIZE.peek();
    if (corbSizeCap & 0x40) {
        corbSizeInCommands = 256;
        if ((corbSizeCap & 3) != 2)
            CORBSIZE.poke((corbSizeCap & 0xFC ) | 2);
    } else if (corbSizeCap & 0x20) {
        corbSizeInCommands = 16;
        if ((corbSizeCap & 3) != 2)
            CORBSIZE.poke((corbSizeCap & 0xFC ) | 1);
    } else if (corbSizeCap == 0x10) {
        corbSizeInCommands = 2;
        if ((corbSizeCap & 3) != 2)
            CORBSIZE.poke((corbSizeCap & 0xFC ) | 0);
    } else {
        dpmhw_log("Unable to determine CORB size capability\n");
        corbSizeInCommands = 256;
        CORBSIZE.poke(2);
    }

    dpmhw_log("HDA CORB sizeInCommands=%u,CORBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
              corbSizeInCommands, CORBSIZE.peek(), corbDma.selectorAddress, corbDma.physicalAddress);
    CORB.poke(corbDma.physicalAddress);
    CORBUBASE.poke(0);
    CORBRP.poke(0x8000);
    while(!(CORBRP.peek() & 0x8000)) INLINE_PAUSE;
    CORBRP.poke(0);
    while(CORBRP.peek()) INLINE_PAUSE;
    CORBWP.poke(0);
    dpmhw_debug("CORBWP=%u,CORBRP=%u\n", CORBWP.peek(), CORBRP.peek());
    if (CORBWP.peek() | CORBRP.peek()) {
        dpmhw_log("CORB pointers in unexpected position\n");
        return false;
    }
    dpmhw_debug("Starting CORB DMA Engine...");
    CORBCTL.poke(0x2);
    while(!(CORBCTL.peek() & 0x2)) INLINE_PAUSE;
    dpmhw_debug("CORBWP=%u,CORBRP=%u\n", CORBWP.peek(), CORBRP.peek());
    if (CORBWP.peek() | CORBRP.peek()) {
        dpmhw_debug("CORB pointers in unexpected position\n");
        return false;
    }
    dpmhw_log("CORB appears to be running!\n");

    unsigned char rirbSizeCap = RIRBSIZE.peek();
    if (rirbSizeCap & 0x40) {
        rirbSizeInCommands = 256;
        if ((rirbSizeCap & 3) != 2)
            RIRBSIZE.poke((rirbSizeCap & 0xFC ) | 2);
    } else if (rirbSizeCap & 0x20) {
        rirbSizeInCommands = 16;
        if ((rirbSizeCap & 3) != 1)
            RIRBSIZE.poke((rirbSizeCap & 0xFC ) | 1);
    } else if (rirbSizeCap == 0x10) {
        rirbSizeInCommands = 2;
        if ((rirbSizeCap & 3) != 0)
            RIRBSIZE.poke((rirbSizeCap & 0xFC ) | 0);
    } else {
        dpmhw_log("Unable to determine RIRB size capability\n");
        rirbSizeInCommands = 256;
        RIRBSIZE.poke(2);
    }
    RINTCNT.poke(210); //TODO - Not sure why I did this

    dpmhw_log("HDA RIRB sizeInCommands=%u,RIRBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
              rirbSizeInCommands, RIRBSIZE.peek(), rirbDma.selectorAddress, rirbDma.physicalAddress);
    RIRBLBASE.poke(rirbDma.physicalAddress);
    RIRBUBASE.poke(0);
    RIRBWP.poke(0x8000);
    rirbReadPointer = RIRBWP.peek();
    dpmhw_debug("RIRBWP=%u,rirbReadPointer=%u\n", RIRBWP.peek(), rirbReadPointer);
    if (RIRBWP.peek() | rirbReadPointer) {
        dpmhw_log("RIRB pointers in unexpected position\n");
        return false;
    }
    dpmhw_debug("Starting RIRB DMA Engine...\n");
    RIRBCTL.poke(0x2);
    while(!(RIRBCTL.peek() & 0x2)) INLINE_PAUSE;
    dpmhw_debug("RIRBWP=%u,rirbReadPointer=%u\n", RIRBWP.peek(), rirbReadPointer);
    if (RIRBWP.peek() | rirbReadPointer) {
        dpmhw_log("RIRB pointers in unexpected position\n");
        return false;
    }
    dpmhw_log("RIRB appears to be running!\n");

    corbRirbSystemsActive = true;


    return true;
}


void HdaDevice::force_reset() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to force-reset HDA because allocation was not successful\n");
        return;
    }
    dpmhw_log("Resetting the HDA...\n");
    INTCTL.poke(0);

    dpmhw_log("Resetting all streams...\n");
    unsigned short gcap = GCAP.peek();
    unsigned int scnt = ((gcap >> 12) & 0xF) + ((gcap >> 8) & 0xF) + ((gcap >> 3) & 0x1F);
    for(unsigned int i=0; i<scnt && i <30; i++) {
        unsigned ctlreg = 0x80 + i * 0x20;
        unsigned int x = regs.peek32(ctlreg);
        x &= 0xFFFFFF; // oddly, only 3 byte register
        if (x & 0x2) {
            dpmhw_log("Stopping stream %u...\n",i);
            x &= (~2); //Don't run
            regs.poke32(ctlreg, x);
            while (regs.peek32(ctlreg) & 0x2) INLINE_PAUSE;
        }
        dpmhw_log("Resetting stream %u...\n",i);
        x = regs.peek32(ctlreg) & 0xFFFFFF;
        x |= 1;
        regs.poke32(ctlreg, x);
        while (!(regs.peek32(ctlreg) & 0x1)) INLINE_PAUSE;
        // TODO - should we leave it in reset instead?
        x = regs.peek32(ctlreg) & 0xFFFFFF;
        x &= ~1;
        regs.poke32(ctlreg, x);
        while (regs.peek32(ctlreg) & 0x1) INLINE_PAUSE;
    }

    dpmhw_log("Stopping DPL...\n");
    DPLBASE.poke(DPLBASE.peek() & ~1);
    while (DPLBASE.peek() & 1) INLINE_PAUSE;

    dpmhw_log("Stopping response dma...\n");
    RIRBCTL.poke(0);
    while((RIRBCTL.peek() & 0x2) != 0) INLINE_PAUSE;
    dpmhw_log("Stopping command dma...\n");
    CORBCTL.poke(0);
    while((CORBCTL.peek() & 0x2) != 0) INLINE_PAUSE;
    dpmhw_log("Resetting device...\n");
    GCTL.poke(0);
    corbRirbSystemsActive = false;
    while((GCTL.peek() & 0x1) != 0) INLINE_PAUSE;

    GCTL.poke(1);

    dpmhw_log("HDA has been reset\n");

}


bool HdaDevice::singleCommand(unsigned long command,  unsigned long &response) {
    if (!allocationSucceeded.get()) {
        dpmhw_log("ERROR: Failed to singleCommand HDA because allocation was not successful\n");
        return false;
    }
    dpmhw_debug("Sending=%08x\n",command);
    if (!corbRirbSystemsActive) {
        dpmhw_log("Error: CORB/RIRB not active\n");
        return false;
    }
    if (getAcceptsUnsolicitedResponse()) {
        dpmhw_log("Error: Unsolicited responses not supported\n");
        return false;
    }
    if (CORBRP.peek() != CORBWP.peek()) {
        dpmhw_log("Error: command already in progress\n");
        return false;
    }
    while (RIRBWP.peek() != rirbReadPointer) {
        dpmhw_log("Error: Unread responses exist\n");
        rirbReadPointer = RIRBWP.peek();
        //return false;
    }

    //TODO - Need to setup timeouts and maybe kill pending reads if they fail
    unsigned short nextWritePtr = ((CORBWP.peek() & 0xFF) + 1) & (corbSizeInCommands - 1);
    dpmhw_debug("nextWritePtr=%u\n", nextWritePtr);

    corbDma.selector.poke32(corbDma.selectorAddress + 4 * nextWritePtr, command);
    corbDma.selector.flushLine(corbDma.selectorAddress + 4 * nextWritePtr);

    dpmhw_debug("Poked %08X at DMA 0x%x\n", command, corbDma.selectorAddress + 4 * nextWritePtr);
    CORBWP.poke(nextWritePtr);

    dpmhw_debug("Waiting for command acceptance...\n");
    dbgCommandState();
    //TODO - spin then timeout
    while(CORBWP.peek() != CORBRP.peek()) {
        INLINE_PAUSE;
        dbgCommandState();
    }

    dpmhw_debug("Waiting for response...\n");
    dbgCommandState();
    //TODO - spin then timeout
    while(RIRBWP.peek() == rirbReadPointer) {
        INLINE_PAUSE;
        dbgCommandState();
    };
    rirbReadPointer = RIRBWP.peek();
    //Not really sure if fencing is needed or which side of the flush it should go on...
    INLINE_MFENCE;  // Intel manual suggests flush isn't ordered with respect to reads.  It is ordered with respect to fences
    rirbDma.selector.flushLine(rirbDma.selectorAddress + 8 * rirbReadPointer);
    INLINE_MFENCE;  // Intel manual suggests flush isn't ordered with respect to reads.  It is ordered with respect to fences
    unsigned long v1 = rirbDma.selector.peek32(rirbDma.selectorAddress + 8 * rirbReadPointer);
    unsigned long v2 = rirbDma.selector.peek32(rirbDma.selectorAddress + 8 * rirbReadPointer + 4);
    dpmhw_debug("Received %u/%u\n", v1, v2);
    //TODO: Validated codec # and unsolicited flag in v2
    response = v1;
    return true;

}


void HdaDevice::dbgCommandState() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dbgCommandState HDA because allocation was not successful\n");
        return;
    }
    dpmhw_debug("GCTL=%lu\n", GCTL.peek());
    dbgCorb();
    dbgRirb();
}

void HdaDevice::dbgCorb() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dbgCorb HDA because allocation was not successful\n");
        return;
    }
    dpmhw_debug("CORB L/U=%lx/%lx W/R=%hu/%hu C/S/Z=%hhu/%hhu/%hhu %8.8x %8.8x\n",
                CORB.peek(), CORBUBASE.peek(),
                CORBWP.peek(), CORBRP.peek(),
                CORBCTL.peek(), CORBSTATUS.peek(), CORBSIZE.peek(),
                allocationSucceeded.get() ? corbDma.selector.peek32(corbDma.selectorAddress) : 0,
                allocationSucceeded.get() ? corbDma.selector.peek32(corbDma.selectorAddress+4) : 0
    );
}
void HdaDevice::dbgRirb() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dbgRirb HDA because allocation was not successful\n");
        return;
    }
    dpmhw_debug("RIRB L/U=%lx/%lx W/R=%hu/%hu C/S/Z=%hhu/%hhu/%hhu %8.8x %8.8x %8.8x %8.8x\n",
                RIRBLBASE.peek(), RIRBUBASE.peek(),
                RIRBWP.peek(), rirbReadPointer,
                RIRBCTL.peek(), RIRBSTS.peek(), RIRBSIZE.peek(),
                allocationSucceeded.get() ? rirbDma.selector.peek32(rirbDma.selectorAddress) : 0,
                allocationSucceeded.get() ? rirbDma.selector.peek32(rirbDma.selectorAddress+4) : 0,
                allocationSucceeded.get() ? rirbDma.selector.peek32(rirbDma.selectorAddress + 8) : 0,
                allocationSucceeded.get() ? rirbDma.selector.peek32(rirbDma.selectorAddress+12) : 0
    );
}
void HdaDevice::dumpDmaBuf() {
    if (dmaPosDma.isError()) {
        dpmhw_log("ERROR: Failed to dumpDmaBuf HDA because allocation was not successful\n");
        return;
    }
    for(int i =0 ; i < 4; i++) {
        unsigned int p[8];
        for(int j=0; j<8;j++) {
            p[j] = dmaPosDma.selector.peek32(dmaPosDma.selectorAddress + i*32 + j * 4);
        }
        dpmhw_log("DMA %02X %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
        );
    }
}

void HdaDevice::dumpRegs() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dumpRegs HDA because allocation was not successful\n");
        return;
    }
    for(int i =0 ; i < 16; i++) {
        unsigned int p[8];
        for(int j=0; j<8;j++) {
            SelectorMem::ref32 x = regs.r32(i*32 + j * 4);
            p[j] = x.peek();
        }
        dpmhw_log("REGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
        );
    }
}
void HdaDevice::dumpVendorRegs() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dumpVendorRegs HDA because allocation was not successful\n");
        return;
    }
    for(int i =0 ; i < 2; i++) {
        unsigned int p[8];
        for(int j=0; j<8;j++) {
            SelectorMem::ref32 x = regs.r32(i*32 + j * 4 + 0x1000);
            p[j] = x.peek();
        }
        dpmhw_log("VREGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
        );
    }
}
void HdaDevice::dumpExtendedRegs() {
    if (regs.isNull()) {
        dpmhw_log("ERROR: Failed to dumpExtendedRegs HDA because allocation was not successful\n");
        return;
    }
    for(int i =0 ; i < 2; i++) {
        unsigned int p[8];
        for(int j=0; j<8;j++) {
            SelectorMem::ref32 x = regs.r32(i*32 + j * 4 + 0x2000);
            p[j] = x.peek();
        }
        dpmhw_log("EREGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
        );
    }
}
