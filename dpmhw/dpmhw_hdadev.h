//
// Created by arnold on 12/24/24.
//

#ifndef XTRS_DPMHW_HDADEV_H
#define XTRS_DPMHW_HDADEV_H
#include "dpmhw_memory.h"
#include "dpmhw_pci.h"

namespace dpmhw {
    class HdaDevice {
    private:
        const dpmhw::PciFunction pciFunction;
        const SelectorMem regs;

        SelectorMem::ref16 GCAP = regs.r16(0x00);
        SelectorMem::ref8 VMIN = regs.r8(0x02);
        SelectorMem::ref8 VMAJ = regs.r8(0x03);
        SelectorMem::ref16 OUTPAY = regs.r16(0x04);
        SelectorMem::ref16 INPAY = regs.r16(0x06);
        SelectorMem::ref32 GCTL = regs.r32(0x08);
        SelectorMem::ref16 WAKEEN = regs.r16(0x0C);
        SelectorMem::ref16 STATESTS = regs.r16(0x0E);
        SelectorMem::ref16 GSTS = regs.r16(0x10);
        SelectorMem::ref16 OUTSTRMPAY = regs.r16(0x18);
        SelectorMem::ref16 INSTRMPAY = regs.r16(0x1A);

        SelectorMem::ref32 INTCTL = regs.r32(0x20);
        SelectorMem::ref32 INTSTS = regs.r32(0x24);

        SelectorMem::ref32 WallClockCounter = regs.r32(0x30);
        SelectorMem::ref32 SSYNC = regs.r32(0x38);

        SelectorMem::ref32 CORB = regs.r32(0x40);
        SelectorMem::ref32 CORBUBASE = regs.r32(0x44);
        SelectorMem::ref16 CORBWP = regs.r16(0x48);
        SelectorMem::ref16 CORBRP = regs.r16(0x4A);
        SelectorMem::ref8 CORBCTL = regs.r8(0x4C);
        SelectorMem::ref8 CORBSTATUS = regs.r8(0x4D);
        SelectorMem::ref8 CORBSIZE = regs.r8(0x4E);


        SelectorMem::ref32 RIRBLBASE = regs.r32(0x50);
        SelectorMem::ref32 RIRBUBASE = regs.r32(0x54);
        SelectorMem::ref16 RIRBWP = regs.r16(0x58);
        SelectorMem::ref16 RINTCNT = regs.r16(0x5A);
        SelectorMem::ref8 RIRBCTL = regs.r8(0x5C);
        SelectorMem::ref8 RIRBSTS = regs.r8(0x5D);
        SelectorMem::ref8 RIRBSIZE = regs.r8(0x5E);

        SelectorMem::ref32 ICW = regs.r32(0x60);
        SelectorMem::ref32 IRR = regs.r32(0x64);
        SelectorMem::ref32 ICS = regs.r32(0x68);

        SelectorMem::ref32 DPLBASE = regs.r32(0x70);
        SelectorMem::ref32 DPUBASE = regs.r32(0x74);

        DmaRegion * const pDmaRegion;
        const DmaRegion::DmaBlock corbDma;
        const DmaRegion::DmaBlock rirbDma;
        const DmaRegion::DmaBlock dmaPosDma;
        const bool allocationSucceeded;

        unsigned short corbSizeInCommands = 0;
        unsigned short rirbSizeInCommands = 0;
        unsigned short rirbReadPointer = 0;
        bool corbRirbSystemsActive = false;


    public:
        HdaDevice(dpmhw::PciFunction &p, SelectorMem &r) :
                pciFunction(p),
                regs(r),
                // Worst case 256 CORB entries (256 * 4), 256 RIRB entries (256 * 8), 64 DMAPOS entries (64 * 8)
                pDmaRegion(DmaRegion::allocate(256 * 4 + 256 * 4 + 256 * 4 + 64 * 8, 128)),
                corbDma(DmaRegion::reserveBlock(pDmaRegion, 256 * 4)),
                rirbDma(DmaRegion::reserveBlock(pDmaRegion, 256 * 8)),
                dmaPosDma(DmaRegion::reserveBlock(pDmaRegion, 64 * 8)),
                allocationSucceeded(!corbDma.isError() && !rirbDma.isError() && !dmaPosDma.isError())
                {
            dpmhw_log("Allocated HDADevice success=%d\n", allocationSucceeded ? 1 : 0);
        }

        HdaDevice(const HdaDevice&) = delete; //Remove this compiler generated doohickeys
        void operator=(const HdaDevice&) = delete;
        //TODO: Should I have a destructor?  This forces __gxx_personality_v0
        //  Maybe can avoid it by disabling exceptions/RTTI
        //  See https://stackoverflow.com/questions/329059/what-is-gxx-personality-v0-for

        /*
        ~HdaDevice() {
            if (active) {
                reset();
            }
            if (!active) {
                //TODO - Should unlock the memory in question?
                if (dmaMem) {
                    free(dmaMem);
                }
            }
            joshlog("Destroyed HdaDevice object\n");

        }
         */


        bool activate();


        void force_reset();

        unsigned short getGlobalCapabilities() { return GCAP.peek(); }
        unsigned int getNumberOfOutputStreamsSupported() { return (getGlobalCapabilities() >> 12) & 0xF; }
        unsigned int getNumberOfInputStreamsSupported() { return (getGlobalCapabilities() >> 8) & 0xF; }
        unsigned int getNumberOfBidirectionalStreamsSupported() { return (getGlobalCapabilities() >> 3) & 0x1F; }
        unsigned int getNumberOfSerialDataOutSignals() { return (getGlobalCapabilities() >> 1) & 0x3; }
        unsigned int get64BitAddressSupported() { return getGlobalCapabilities() & 0x1; }

        unsigned short getCodecBitMap() { return STATESTS.peek() & 0x7FFF; }

        bool getAcceptsUnsolicitedResponse() { return (GCTL.peek() & 0x100) != 0; }


        bool singleCommand(unsigned long command,  unsigned long &response);

        class Codec {
        public:
            HdaDevice & device;
            const unsigned int codec;
            Codec(HdaDevice &d, unsigned int c) : device(d), codec(c) {}

            unsigned long getNodeParam( unsigned int node, unsigned int param, bool *recvOk) {
                unsigned long r;
                bool e = device.singleCommand(
                        makeCommand(codec, node, 0xf00, param), r);
                if (recvOk)
                    *recvOk = e;
                return e ? r : 0;
            }
            unsigned long getNodeParam( unsigned int node, unsigned int param) {
                return getNodeParam(node, param, 0);
            }

            unsigned long nodeVerb(unsigned int node, unsigned int verb, unsigned int payload) {
                unsigned long r;
                unsigned long cmd = makeCommand(codec, node, verb, payload);
                bool e = device.singleCommand(
                        cmd, r);
                r = e ? r : 0;

                if (verb < 0x800 && (verb < 0x8 || verb >= 0x10))
                    dpmhw_debug("HDA COMMAND: 0x%08x => 0x%08x\n",cmd, r);

                return r;
            }
        };



        static unsigned int makeCommand(unsigned int codec, unsigned int node, unsigned int command, unsigned int data) {
            if (command >= 0x10) {
                return ((codec & 0xFu) << 28) |
                       ((node & 0xFFu) << 20) |
                       ((command & 0xFFFu) << 8) |
                       (data & 0xFFu);
            } else {
                return ((codec & 0xFu) << 28) |
                       ((node & 0xFFu) << 20) |
                       ((command & 0xFu) << 16) |
                       (data & 0xFFFFu);
            }
        }

        void dbgCommandState();
        void dbgCorb();
        void dbgRirb();
        void dumpDmaBuf();
        void dumpRegs();
        void dumpVendorRegs();
        void dumpExtendedRegs();
    };

};

#endif //XTRS_DPMHW_HDADEV_H
