//
// Created by arnold on 12/25/24.
//

#ifndef XTRS_DPMHW_HDASTREAM_H
#define XTRS_DPMHW_HDASTREAM_H

#include "dpmhw_memory.h"
#include "dpmhw_hdadev.h"

namespace dpmhw {


    class HdaOutputStream {
    private:
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
        std::unique_ptr<DmaRegion, decltype(&DmaRegion::deallocate)>  pDmaRegion;
        const DmaRegion::DmaBlock dmaBdl;
        const DmaRegion::DmaBlock dmaBuffers;
        reset_on_move<bool> allocationSucceeded;
        //TODO: maybe combine ownsStream and allocationSucceeded into a single flag
        reset_on_move<bool> ownsStream;



    public:
        HdaOutputStream(HdaDevice *d, unsigned int bsize, unsigned char bcount, unsigned char descNo, unsigned char streamNo);
        ~HdaOutputStream();

        /*
         * This constructs an "invalid" stream where "allocationSucceeded" is false.  Can use move assignment to
         * later give it a valid value...except move assignment isn't defined here :(
         */
        HdaOutputStream();

        //Don't allow copies
        HdaOutputStream(const HdaOutputStream &rhs) = delete;
        HdaOutputStream &operator=(const HdaOutputStream &rhs) = delete;
        //Allow move construction - source stream becomes non-owner with allocationSucceeded as false (basically invalid)
        HdaOutputStream(HdaOutputStream &&rhs) = default;
        // Not allowing move assignment because we would need to be able to deallocate the current stream
        HdaOutputStream &operator=(HdaOutputStream &&rhs) = delete;

        void run();
        void stop();

        [[nodiscard]] bool isValid() const { return allocationSucceeded.get(); }

        [[nodiscard]] const DmaRegion::DmaBlock & getDmaBuffers() {
            return dmaBuffers;
        }

        [[nodiscard]] unsigned long getDmaPos() const {
            return dev->getDmaPos(descriptorNumber);
        }
        [[nodiscard]] unsigned long getLinkPos() const {
            return SDLPIB.peek();
        }
        [[nodiscard]] unsigned short getFifoSize() const {
            return SDFIFOS.peek();
        }
        [[nodiscard]] unsigned char getStatus() const {
            return SDSTS.peek();
        }
        [[nodiscard]] unsigned short getFormat() const {
            return SDFMT.peek();
        }
        [[nodiscard]] unsigned char getStreamNumber() const {
            return streamNumber;
        }
        [[nodiscard]] unsigned char getDescriptorNumber() const {
            return descriptorNumber;
        }
        void dumpBufferDescriptorList();

        /** Should be a power of 2 to avoid glitching */
        void fillBufferWithTestTone(uint32_t halfPeriodInSamples);

    };

}


#endif //XTRS_DPMHW_HDASTREAM_H
