//
// Created by arnold on 12/25/24.
//

#include "dpmhw_hdastream.h"
#include "dpmhw_impl.h"

using dpmhw::HdaOutputStream;


HdaOutputStream::HdaOutputStream(HdaDevice *d, unsigned int bsize, unsigned char bcount, unsigned char descNo, unsigned char streamNo)
: dev(d)
, descriptorNumber(descNo)
, SDCTL(d->regs, 0x80 + descNo * 0x20)
, SDSTS(d->regs, 0x83 + descNo * 0x20)
, SDLPIB(d->regs, 0x84 + descNo * 0x20)
, SDCBL(d->regs, 0x88 + descNo * 0x20)
, SDLVI(d->regs, 0x8C + descNo * 0x20)
, SDFIFOS(d->regs, 0x90 + descNo * 0x20)
, SDFMT(d->regs, 0x92 + descNo * 0x20)
, SDBDPL(d->regs, 0x98 + descNo * 0x20)
, SDBDPU(d->regs, 0x9C + descNo * 0x20)
, streamNumber(streamNo)
, bufferCount(bcount)
, singleBufferSize(bsize)
, totalBufferSize(bsize * bcount)
, pDmaRegion(DmaRegion::allocate(256 * 16 + totalBufferSize, 128))
, dmaBdl(DmaRegion::reserveBlock(pDmaRegion, 256 * 16))
, dmaBuffers(DmaRegion::reserveBlock(pDmaRegion, totalBufferSize))
, allocationSucceeded(pDmaRegion && !dmaBdl.isError() && !dmaBuffers.isError() && isPowerOfTwo(singleBufferSize) && singleBufferSize >= 128)
{
    //TODO - Check that buffer size is a power of 2 and is at least 128 bytes?
    if (!allocationSucceeded) {
        dpmhw_log("ERROR Failed to allocate HDA Stream Buffers bc=%u,sbs=%u\n", bcount, bsize);
        return;
    }
    dpmhw_log("HDA Setting up stream desc %u\n", descNo);
    const unsigned int ctlUpperBits = ((streamNumber & 0xF) << 20);
    //| ( 1 << 19)
    //| (1 << 18);

    //Putting stream in reset
    dpmhw_log("HDA Resetting the stream...\n");
    SDCTL.poke(ctlUpperBits | 1);
    while(!(SDCTL.peek() & 1)) INLINE_PAUSE;
    SDCTL.poke(ctlUpperBits);
    while((SDCTL.peek() & 1)) INLINE_PAUSE;
    dpmhw_log("HDA Stream is reset\n");


    for(unsigned int i=0; i < totalBufferSize; i+=2) {
        dmaBuffers.selector.poke16(dmaBuffers.selectorAddress + i, 0);
    }
    dmaBuffers.flushFromCache();

    for(unsigned int i=0; i < bufferCount; i++) {
        const uint32_t entry = dmaBdl.selectorAddress + i * 0x10;
        dmaBdl.selector.poke32(entry + 0, dmaBuffers.physicalAddress + i * singleBufferSize);
        dmaBdl.selector.poke32(entry + 0x4, 0);
        dmaBdl.selector.poke32(entry + 0x8, singleBufferSize);
        dmaBdl.selector.poke32(entry + 0xC, singleBufferSize);

        dpmhw_log("BDL @ %08x (%08x): %08x %08x %08x\n",
                dmaBdl.physicalAddress + i * 0x10,
                dmaBdl.selectorAddress + i * 0x10,
                dmaBdl.selector.peek32(entry + 0),
                dmaBdl.selector.peek32(entry + 0x4),
                dmaBdl.selector.peek32(entry + 0x8),
                dmaBdl.selector.peek32(entry + 0xC)
        );
    }
    dmaBdl.flushFromCache();

    SDCTL.poke(ctlUpperBits);
    if (dev->get64BitAddressSupported())
        SDBDPU.poke(0);
    SDBDPL.poke(dmaBdl.physicalAddress);
    dpmhw_log("HDA SDBDPL(0x%x) is 0x%x\n", SDBDPL.offset,  SDBDPL.peek());

    SDCBL.poke(totalBufferSize); //Pretty sure this is in bytes but at some point I wondered if it was samples
    SDLVI.poke(bufferCount - 1);
    //TODO - This is setting up a stereo stream - but would be easier to do a mono stream
    // TODO - I just switched this to 48Khz (was 44.1) - make sure this works
    SDFMT.poke(
            (0 << 14)   // 48 KHz
            | ( 1 << 4)
            | 1   // 48 KHz,  16 Bits,  2 channels
    );

}

HdaOutputStream::~HdaOutputStream() {
    if (allocationSucceeded) {
        stop();


        dpmhw_log("Resetting stream %d\n", streamNumber);
        SDCTL.poke(SDCTL.peek() | 1);
        while (!(SDCTL.peek() & 1)) {
            INLINE_PAUSE;
        }
        dpmhw_log("Unresetting stream %d\n", streamNumber);
        SDCTL.poke(SDCTL.peek() & ~1);
        while (SDCTL.peek() & 1) {
            INLINE_PAUSE;
        }
        dpmhw_log("SDCTL=0x%x\n", SDCTL.peek());
        dpmhw_log("SDBDPL=0x%x\n", SDBDPL.peek());
    }
    DmaRegion::deallocate(pDmaRegion);
}


void HdaOutputStream::run() {
    if (!allocationSucceeded) {
        dpmhw_log("HDA ERROR - Cannot run stream because allocation failed\n");
        return;
    }
    SDCTL.poke(SDCTL.peek() | 2);
    dpmhw_log("Started stream SDCTL=0x%x SDBDPL=0x%x\n", SDCTL.peek(), SDBDPL.peek());
}

void HdaOutputStream::stop() {
    if (!allocationSucceeded) {
        return;
    }
    dpmhw_log("Stopping stream %d\n", streamNumber);
    SDCTL.poke(SDCTL.peek() & ~2);
    while(SDCTL.peek() & 2) {
        INLINE_PAUSE ;
    }
    dpmhw_log("SDCTL=0x%x\n", SDCTL.peek());
}

void HdaOutputStream::dumpBufferDescriptorList() {
    for(int i =0 ; i < bufferCount; i++) {
        unsigned int p[4];
        for(int j=0; j<4;j++) {
            p[j] = dmaBdl.selector.peek32(dmaBdl.selectorAddress + i * 16 + j * 4);
        }
        dpmhw_log("BDL %02X %08X %08X %08X %08X\n",
                i , p[0],p[1],p[2],p[3]
        );
    }
}

void HdaOutputStream::fillBufferWithTestTone(uint32_t halfPeriodInSamples) {
    uint32_t cnt = 0;
    bool state = false;
    for(uint32_t offset = 0; offset < dmaBuffers.size; offset += 2) {
        dmaBuffers.selector.poke16(dmaBuffers.selectorAddress + offset, state ? 0xF000 : 0x1000);
        cnt += 1;
        if (cnt == halfPeriodInSamples) {
            state = !state;
            cnt = 0;
        }
    }
    dmaBuffers.flushFromCache();
}