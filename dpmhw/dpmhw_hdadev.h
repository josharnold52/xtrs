//
// Created by arnold on 12/24/24.
//

#ifndef XTRS_DPMHW_HDADEV_H
#define XTRS_DPMHW_HDADEV_H
#include "dpmhw_memory.h"
#include "dpmhw_pci.h"

namespace dpmhw {
    enum HdaDeviceType {
        intelSch,
        other
    };

    HdaDeviceType hdaGetDeviceType(dpmhw::PciFunction &pciFunction);


    class HdaDevice {
    public:
        const dpmhw::PciFunction pciFunction;
        const HdaDeviceType deviceType;
        const SelectorMem regs;

        const SelectorMem::ref16 GCAP = regs.r16(0x00);
        const SelectorMem::ref8 VMIN = regs.r8(0x02);
        const SelectorMem::ref8 VMAJ = regs.r8(0x03);
        const SelectorMem::ref16 OUTPAY = regs.r16(0x04);
        const SelectorMem::ref16 INPAY = regs.r16(0x06);
        const SelectorMem::ref32 GCTL = regs.r32(0x08);
        const SelectorMem::ref16 WAKEEN = regs.r16(0x0C);
        const SelectorMem::ref16 STATESTS = regs.r16(0x0E);
        const SelectorMem::ref16 GSTS = regs.r16(0x10);
        const SelectorMem::ref16 OUTSTRMPAY = regs.r16(0x18);
        const SelectorMem::ref16 INSTRMPAY = regs.r16(0x1A);

        const SelectorMem::ref32 INTCTL = regs.r32(0x20);
        const SelectorMem::ref32 INTSTS = regs.r32(0x24);

        const SelectorMem::ref32 WallClockCounter = regs.r32(0x30);
        const SelectorMem::ref32 SSYNC = regs.r32(0x38);

        const SelectorMem::ref32 CORB = regs.r32(0x40);
        const SelectorMem::ref32 CORBUBASE = regs.r32(0x44);
        const SelectorMem::ref16 CORBWP = regs.r16(0x48);
        const SelectorMem::ref16 CORBRP = regs.r16(0x4A);
        const SelectorMem::ref8 CORBCTL = regs.r8(0x4C);
        const SelectorMem::ref8 CORBSTATUS = regs.r8(0x4D);
        const SelectorMem::ref8 CORBSIZE = regs.r8(0x4E);


        const SelectorMem::ref32 RIRBLBASE = regs.r32(0x50);
        const SelectorMem::ref32 RIRBUBASE = regs.r32(0x54);
        const SelectorMem::ref16 RIRBWP = regs.r16(0x58);
        const SelectorMem::ref16 RINTCNT = regs.r16(0x5A);
        const SelectorMem::ref8 RIRBCTL = regs.r8(0x5C);
        const SelectorMem::ref8 RIRBSTS = regs.r8(0x5D);
        const SelectorMem::ref8 RIRBSIZE = regs.r8(0x5E);

        const SelectorMem::ref32 ICW = regs.r32(0x60);
        const SelectorMem::ref32 IRR = regs.r32(0x64);
        const SelectorMem::ref32 ICS = regs.r32(0x68);

        const SelectorMem::ref32 DPLBASE = regs.r32(0x70);
        const SelectorMem::ref32 DPUBASE = regs.r32(0x74);
    private:

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
                deviceType(hdaGetDeviceType(p)),
                regs(r),
                // Worst case 256 CORB entries (256 * 4), 256 RIRB entries (256 * 8), 64 DMAPOS entries (64 * 8)
                pDmaRegion(DmaRegion::allocate(256 * 4 + 256 * 4 + 256 * 4 + 64 * 8, 128)),
                corbDma(DmaRegion::reserveBlock(pDmaRegion, 256 * 4)),
                rirbDma(DmaRegion::reserveBlock(pDmaRegion, 256 * 8)),
                dmaPosDma(DmaRegion::reserveBlock(pDmaRegion, 64 * 8)),
                allocationSucceeded(!corbDma.isError() && !rirbDma.isError() && !dmaPosDma.isError() && !p.hadErrors())
                {
            if (p.hadErrors()) {
                dpmhw_log("HDA ERROR: PCI Interface had errors!");
            }
            dpmhw_log("Allocated HDADevice success=%d\n", allocationSucceeded ? 1 : 0);
        }

        HdaDevice(const HdaDevice&) = delete; //Remove this compiler generated doohickeys
        void operator=(const HdaDevice&) = delete;
        //TODO: Should I have a destructor?  This forces __gxx_personality_v0
        //  Maybe can avoid it by disabling exceptions/RTTI
        //  See https://stackoverflow.com/questions/329059/what-is-gxx-personality-v0-for


        ~HdaDevice() {
            force_reset();
            DmaRegion::deallocate(pDmaRegion);
        }

        bool activate();
        void force_reset();

        int32_t getWallClockCount() { return (int32_t)WallClockCounter.peek(); };
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

        uint32_t getDmaPos(int descriptorNo) {
            if (descriptorNo < 0 || descriptorNo > 63) {
                return 0xFFFFFFFFu;
            }
            //Intel SCH (on the ASUS NB) puts the DMA Position info of its output streams at a different index
            if (deviceType == HdaDeviceType::intelSch) {
                if (descriptorNo == 2 || descriptorNo == 3) {
                    descriptorNo += 2;
                }
            }

            return dmaPosDma.selector.peek32(dmaPosDma.selectorAddress + (descriptorNo * 8));
        }
    };

};

#endif //XTRS_DPMHW_HDADEV_H
