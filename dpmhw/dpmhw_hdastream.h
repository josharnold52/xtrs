//
// Created by arnold on 12/25/24.
//

#ifndef XTRS_DPMHW_HDASTREAM_H
#define XTRS_DPMHW_HDASTREAM_H

#include "dpmhw_memory.h"
#include "dpmhw_hdadev.h"

namespace dpmhw {


    class HdaOutputStream {
    public:
        HdaDevice * const dev;
        const unsigned char descriptorNumber;

        const SelectorMem::ref32 SDCTL;
        const SelectorMem::ref8 SDSTS;
        const SelectorMem::ref32 SDLPIB;
        const SelectorMem::ref32 SDCBL;
        const SelectorMem::ref16 SDLVI;
        const SelectorMem::ref16 SDFIFOS;
        const SelectorMem::ref16 SDFMT;
        const SelectorMem::ref32 SDBDPL;
        const SelectorMem::ref32 SDBDPU;

        const unsigned char streamNumber;
        const unsigned char bufferCount;
        const unsigned int singleBufferSize;
        const unsigned int totalBufferSize;
    private:
        DmaRegion * const pDmaRegion;
        const DmaRegion::DmaBlock dmaBdl;
    public:
        const DmaRegion::DmaBlock dmaBuffers;
        const bool allocationSucceeded;


    public:
        HdaOutputStream(HdaDevice *d, unsigned int bsize, unsigned char bcount, unsigned char descNo, unsigned char streamNo);
        ~HdaOutputStream();

        void run();
        void stop();

        unsigned long getDmaPos() {
            return dev->getDmaPos(descriptorNumber);
        }
        unsigned long getLinkPos() {
            return SDLPIB.peek();
        }
        unsigned short getFifoSize() {
            return SDFIFOS.peek();
        }
        unsigned char getStatus() {
            return SDSTS.peek();
        }
        unsigned short getFormat() {
            return SDFMT.peek();
        }
        unsigned char getStreamNumber() {
            return streamNumber;
        }
        unsigned char getDescriptorNumber() {
            return descriptorNumber;
        }
        void dumpBufferDescriptorList();

        /** Should be a power of 2 to avoid glitching */
        void fillBufferWithTestTone(uint32_t halfPeriodInSamples);

    };

}


#endif //XTRS_DPMHW_HDASTREAM_H
