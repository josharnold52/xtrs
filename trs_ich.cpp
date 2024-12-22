
#include <go32.h>
#include <dpmi.h>
#include <cstring>
#include <cstdlib>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <unistd.h>
#include <cstdarg>

extern "C" {
#include "z80.h"
}

#define PCI_BIOS_INT 0x1A
#define PCI_FUNCTION_ID 0xB1
#define PCI_BIOS_PRESENT 0x01
#define FIND_PCI_CLASS_CODE 0x03
#define PCI_READ_CONFIG_BYTE 0x08
#define PCI_READ_CONFIG_WORD 0x09
#define PCI_READ_CONFIG_DWORD 0x0A
#define PCI_WRITE_CONFIG_BYTE 0x0B
#define PCI_WRITE_CONFIG_WORD 0x0C
#define PCI_WEITE_CONFIG_DWORD 0x0D

#define PCI_SUCCESSFUL 0
#define PCI_DEVICE_NOT_FOUND 0x86


#define INLINE_PAUSE  { __asm__ __volatile__ ("pause"); }


static const int NODE_PARAM_DEVICE_ID = 0x0;
static const int NODE_PARAM_REVISION_ID = 0x2;
static const int NODE_PARAM_SUBORDINATE_NODES = 0x4;
static const int NODE_PARAM_FUNCTION_GROUP_TYPE = 0x5;
static const int NODE_PARAM_AUDIO_FUNCTION_GROUP_CAPABILITIES = 0x8;
static const int NODE_PARAM_AUDIO_WIDGET_CAPABILITIES = 0x9;
static const int NODE_PARAM_SUPPORTED_PMC_RATES = 0xA;
static const int NODE_PARAM_SUPPORTED_STREAM_FORMATS = 0xB;
static const int NODE_PARAM_PIN_CAPABILITIES = 0xC;
static const int NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES = 0xD;
static const int NODE_PARAM_CONNECTION_LIST_LENGTH = 0xE;
static const int NODE_PARAM_SUPPORTED_POWER_STATES = 0xF;
static const int NODE_PARAM_PROCESSING_CAPABILITIES = 0x10;
static const int NODE_PARAM_GPIO_COUNT = 0x11;
static const int NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES = 0x12;
static const int NODE_PARAM_VOLUME_KNOB_CAPABILITIES = 0x13;





static const int NODE_TYPE_AUDIO_FUNCTION_GROUP = 1;

static const int WIDGET_TYPE_AUDIO_OUT = 0x0;
static const int WIDGET_TYPE_AUDIO_IN = 0x1;
static const int WIDGET_TYPE_AUDIO_MIXER = 0x2;
static const int WIDGET_TYPE_AUDIO_SELECTOR = 0x3;
static const int WIDGET_TYPE_PIN_COMPLEX = 0x4;
static const int WIDGET_TYPE_POWER = 0x5;
static const int WIDGET_TYPE_VOLUME_KNOB = 0x6;
static const int WIDGET_TYPE_BEEP = 0x7;


static const unsigned int DMA_MEM_ALIGN = 0x1000;


/*
 * TODO:  The MASTER LIST
 *
 * I've been super inconsistent with numeric types, but a major issue has
 * been with HDA node numbers.   The spec mentions 7-bit "short from" or 15-bit
 * "long form" but the 15-bit version depend on some yet-to-be specified indirect
 * addressing scheme.  Probably I should just forget about 15-bit support and go
 * with 7-bit for now.   I've been using unsigned types for node nums, but it's tempting
 * to use signed types because they fit and because negative numbers are convenient
 * invalid sentinel values.
 *
 * I also need to fix connection list parsing - it currently reads the values as
 * 8 bit or 16 bit (depending om short/long form).  But the high bit is actually
 * a flag indicating whether the value is standalone or the top end of a range.
 *
*/

/*
 * OTHER HW NOTES:
 * - If I want to play with the GPIO (I2C) PINS on the VGA of the Asus - Look at
 *   the GMA500 driver in the linux tree.  The GPIO regs are at 0x5010, 0x5014, etc...
 *   and the intel_i2c.c file shows how to use them.  I believe these registers are
 *   relative to the base of the MMIO given by the PCI function for the display.  This
 *   is pretty easy to confirm by tracing back the get_clock, get_data, etc. functions
 *   in the i2c file.  I can possibly figure out which GPIO is used by playing with the
 *   I2C stuff in linux.  That should get to the point where I can do it from DOS probably
 */

static void joshdebug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
//    joshlogv(msg, args);
    va_end(args);
}

static int pci_init_flag = 0;
static int pci_present = 0;
static unsigned char pci_hardware_mechanism;
static unsigned char pci_ver_major;
static unsigned char pci_ver_minor;
static unsigned char pci_last_bus_no;

template<class T, int (*cf)(const T &t1, const T &t2)> class tqsort {
private:
    static int vcf(const void *e1, const void *e2) {
        const T *pt1 = *(static_cast<const T * const *>(e1));
        const T *pt2 = *(static_cast<const T * const *>(e2));
        if (!(pt1 && pt2)) {
            return pt1 ? -1 : (pt2 ? 1 : 0);
        }
        return cf(*pt1, *pt2);
    }
public:
    tqsort() = delete;
    static void run(const T **base, size_t numelem) {
        qsort(base, numelem, sizeof(T*), vcf);
    }
};

/**
 * Kind of like option in java/scala but we need a placeholder value
 * even in the invalid case.
 * @tparam T
 */
template <class T> class option {
private:
    const bool valid;
    T value;
public:
    explicit option(const T &value) : valid(true), value(value) {}
    explicit option(bool valid, const T &value) : valid(valid), value(value) {}

    bool exists() const { return valid; }
    T * operator->() { return valid ? &value : static_cast<T*>(0); }
    T * operator->() const { return valid ? &value : static_cast<T*>(0); }

    T & get() { return value; }
    const T & get() const { return value; }
};


class nodeset {
private:
    unsigned int bits[8]{};
public:
    void add(unsigned char x) {
        bits[(x>>5) & 7] |= 1u << (x & 31);
    }
    void addAll(const nodeset &rhs) {
        for(int i=0;i<8;i++) {
            bits[i] |= rhs.bits[i];
        }
    }
    bool contains(unsigned char x) const {
        return (bits[(x>>5) & 7] & (1u << (x & 31))) != 0;
    }
    int findNext(int startAt) const {
        if (startAt < 0)
            startAt = 0;
        //Can optimize this...
        for(int i=startAt; i < 256; i++) {
            if (contains(i)) {
                return i;
            }
        }
        return -1;
    }
};


class SelectorMem {
public:
    unsigned short selector;
    explicit SelectorMem(unsigned short sel) : selector(sel) {}
    unsigned char peek8 (unsigned long offset) const { return  _farpeekb(selector, offset); }
    unsigned short peek16 (unsigned long offset) const { return  _farpeekw(selector, offset); }
    unsigned long peek32 (unsigned long offset) const { return  _farpeekl(selector, offset); }
    void poke8 (unsigned long offset, unsigned char v) const {  _farpokeb(selector, offset, v); }
    void poke16 (unsigned long offset, unsigned short v) const {  _farpokew(selector, offset, v); }
    void poke32 (unsigned long offset, unsigned long v) const {  _farpokel(selector, offset, v); }
    /*
     * NOTE: If accessing in a tight loop, better to load the descriptor into a segment
     * reg and access relative that reg many times.  DJGPP has macros for this (see the other _far
     * macros).  Or can do it ourselves with inline assembly
     */

    void flushLine(unsigned long offset) const {
        unsigned short sv = _fargetsel();
        _farsetsel(selector);
        __asm__ __volatile__ ("clflush %%fs:(%%eax)" : /*none*/ : "a" (offset));
        _farsetsel(sv);
    }

    static SelectorMem invalid() { return SelectorMem(0) ; }

    static option<SelectorMem> mapDevice(unsigned long addr, unsigned long size) {
        if (size >= 0x100000) {
            joshlog("Segments > 1M not supported (because I have to be smarter about granularity bit");
            return option<SelectorMem>(false, SelectorMem::invalid());
        }
        __dpmi_meminfo mi;
        mi.size=size;
        mi.address = addr;
        mi.handle = 0;
        if (__dpmi_physical_address_mapping(&mi)!=0) {
            joshlog("DPMI map of %x(%u) failed\n", addr,size);
            return option<SelectorMem>(false, SelectorMem::invalid());
        }
        int sel = __dpmi_allocate_ldt_descriptors(1);
        if (sel  == -1) {
            joshlog("Unable to allocate descriptor\n");
            return option<SelectorMem>(false, SelectorMem::invalid());
        }
        //Access rights - Data, RW, Ring 3, size in bytes
        if (__dpmi_set_segment_base_address(sel, addr) |
            __dpmi_set_segment_limit(sel, size - 1) |
            __dpmi_set_descriptor_access_rights(sel, 0x4F3) ) {
            joshlog("Unable to set descriptor params\n");
            return option<SelectorMem>(false, SelectorMem::invalid());
        }
        return option<SelectorMem>(SelectorMem(sel));
    }

    class ref {
    public:
        const unsigned short selector;
        const unsigned long offset;
        ref(unsigned short selector, unsigned long offset) : selector(selector), offset(offset) {}
    };
    class ref8 : public ref {
    public:
        ref8(SelectorMem &mem, unsigned long offset) : ref(mem.selector, offset) {}
        unsigned char peek() const { return  _farpeekb(selector, offset);  }
        void poke(unsigned char v) const { _farpokeb(selector, offset, v);  }
    };
    SelectorMem::ref8 r8(unsigned long offset) { return {*this, offset}; }
    class ref16 : public ref {
    public:
        ref16(SelectorMem &mem, unsigned long offset) : ref(mem.selector, offset) {}
        unsigned short peek() const { return  _farpeekw(selector, offset);  }
        void poke(unsigned short v) const { _farpokew(selector, offset, v);  }
    };
    SelectorMem::ref16 r16(unsigned long offset) { return {*this, offset}; }
    class ref32 : public ref {
    public:
        ref32(SelectorMem &mem, unsigned long offset) : ref(mem.selector, offset) {}
        unsigned long peek() const { return  _farpeekl(selector, offset);  }
        void poke(unsigned long v) const { _farpokel(selector, offset, v);  }
    };
    SelectorMem::ref32 r32(unsigned long offset) { return {*this, offset}; }


};


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
static unsigned long prepare_pci_bios(__dpmi_regs *regs, unsigned char call_no) {
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

    regs->h.ah = PCI_FUNCTION_ID;
    regs->h.al = call_no;

    //Redundant, but keeping it here because _dpmi says we should do this or else
    //must set a valid? flags register...
    regs->x.flags = 0;

    //Return the usable space starting at offset 0 from the ds/es sergments.
    //We need to reserve 1K of stack (from the dpmi spec).  Also
    //subtract 16 because the ds/es are 1 above ss.  Finally subtract another
    //16 in case I did something wrong...
    return regs->x.sp - 1024 - 32;
}


static int test_for_pci() {
    __dpmi_regs regs;
    if (!pci_init_flag) {
        prepare_pci_bios(&regs, PCI_BIOS_PRESENT);

        joshlog("PRE %x,%x,%x,%x,%x,%x\n",
                regs.d.eax,regs.d.ebx, regs.d.ecx, regs.d.edx, regs.d.esi, regs.d.edi
        );

        //int rmi = __dpmi_int(PCI_BIOS_INT, &regs);
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        joshlog("RMI=%d\n",rmi);
        joshlog("POST %x,%x,%x,%x,%x,%x\n",
                regs.d.eax,regs.d.ebx, regs.d.ecx, regs.d.edx, regs.d.esi, regs.d.edi
        );
        if (regs.h.ah == 0 && regs.d.edx == 0x20494350 && (regs.x.flags & 1) == 0) {
            pci_present = 1;
            pci_hardware_mechanism = regs.h.al;
            pci_ver_major = regs.h.bh;
            pci_ver_minor = regs.h.bl;
            pci_last_bus_no = regs.h.cl;
        } else {
            pci_present = 0;
            joshlog("NO! %u,%x,%u\n", regs.h.ah, regs.d.edx, regs.x.flags);
        }
        pci_init_flag = 1;
    }
    return pci_present;
}



class PciFunction {
private:
    const unsigned short pci_address;
    int had_errors;
    static const unsigned int invalid_read_value = 0xFFFFFFFFu;
public:
    explicit PciFunction(unsigned short pci_address) : pci_address(pci_address), had_errors(0) {}

    int hadErrors() const {
        return had_errors != 0;
    }

    int errorDiagnosticBits() const {
        return had_errors;
    }

    int clearErrors() {
        int res = hadErrors();
        had_errors = 0;
        return res;
    }

    unsigned int getConfig32(unsigned char address) {
        if (!test_for_pci()) {
            had_errors |= 1;
            return invalid_read_value;
        }
        address &= 0xFC;
        __dpmi_regs regs;
        prepare_pci_bios(&regs, PCI_READ_CONFIG_DWORD);
        regs.x.bx = pci_address;
        regs.x.di = 0xFF & address;
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        if (rmi != 0) {
            had_errors |= 2;
            return invalid_read_value;
        }
        if ((regs.x.flags & 1) == 1) {
            had_errors |= 4;
            return invalid_read_value;
        }
        if (regs.h.ah != PCI_SUCCESSFUL) {
            had_errors |= 8;
            return invalid_read_value;
        }
        return (unsigned int)regs.d.ecx;
    }
    unsigned short getConfig16(unsigned char address) {
        if (!test_for_pci()) {
            had_errors |= 1;
            return (unsigned short)invalid_read_value;
        }
        address &= 0xFE;
        __dpmi_regs regs;
        prepare_pci_bios(&regs, PCI_READ_CONFIG_WORD);
        regs.x.bx = pci_address;
        regs.x.di = 0xFF & address;
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        if (rmi != 0) {
            had_errors |= 2;
            return (unsigned short)invalid_read_value;;
        }
        if ((regs.x.flags & 1) == 1) {
            had_errors |= 4;
            return (unsigned short)invalid_read_value;;
        }
        if (regs.h.ah != PCI_SUCCESSFUL) {
            had_errors |= 8;
            return (unsigned short)invalid_read_value;;
        }
        return (unsigned int)regs.x.cx;
    }
    void setConfig16(unsigned char address, unsigned short value) {
        if (!test_for_pci()) {
            had_errors |= 1;
            return;
        }
        address &= 0xFE;
        __dpmi_regs regs;
        prepare_pci_bios(&regs, PCI_WRITE_CONFIG_WORD);
        regs.x.bx = pci_address;
        regs.x.di = 0xFF & address;
        regs.x.cx = value;
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        if (rmi != 0) {
            had_errors |= 2;
            return;
        }
        if ((regs.x.flags & 1) == 1) {
            had_errors |= 4;
            return;
        }
        if (regs.h.ah != PCI_SUCCESSFUL) {
            had_errors |= 8;
            return;
        }
        return;
    }

    static PciFunction invalid() { return PciFunction(-1); }

    static option<PciFunction> find_hda_function() {
        if (!test_for_pci()) {
            return option<PciFunction>(false, PciFunction::invalid());
        }
        __dpmi_regs regs;
        prepare_pci_bios(&regs, FIND_PCI_CLASS_CODE);
        regs.d.ecx = 0x040300; //TODO
        regs.x.si = 0;
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        if (rmi != 0) {
            joshlog("rmi FIND_PCI_CLASS_CODE failed\n");
            return option<PciFunction>(false, PciFunction::invalid());
        }
        if ((regs.x.flags & 1) == 1) {
            joshlog("rmi FIND_PCI_CLASS_CODE error\n");
            return option<PciFunction>(false, PciFunction::invalid());
        }
        if (regs.h.ah != PCI_SUCCESSFUL) {
            joshlog("rmi FIND_PCI_CLASS_CODE not found %x\n", regs.h.ah);
            return option<PciFunction>(false, PciFunction::invalid());
        }
        joshlog("Found candidate at %x\n", regs.x.bx);

        return option<PciFunction>(PciFunction (regs.x.bx));
    }


};

class HdaStreamBuffer;

class HdaDevice {
    friend class HdaStreamBuffer;
private:
    PciFunction pciFunction;
    SelectorMem regs;
    unsigned long dmaMemSize = 0x11000;
    unsigned long dmaMemUsed = 0x0;
    unsigned long dmaPhysicalAddress = 0;
    SelectorMem dmaSelector = SelectorMem::invalid();

    unsigned long corbDmaOffset = 0;
    unsigned short corbSizeInCommands = 0;
    bool corbMemoryInitialzed = false;

    unsigned long rirbDmaOffset = 0;
    unsigned short rirbSizeInCommands = 0;
    unsigned short rirbReadPointer = 0;
    bool rirbMemoryInitialzed = false;

    bool corbRirbSystemsActive = false;

    unsigned long dmaPosOffset = 0;



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

public:
    HdaDevice(PciFunction &p, SelectorMem &r) :
            pciFunction(p), regs(r) {
        int sel;
        int paras = (int)(dmaMemSize>>4);
        joshdebug("Requesting %d paras\n", paras);
        int i = __dpmi_allocate_dos_memory(paras, &sel);
        if (i == -1) {
            dmaPhysicalAddress = 0;
            dmaSelector = SelectorMem::invalid();
            joshlog("Failed to allocate dos memory for hda dma\n");
        }
        dmaPhysicalAddress = ((unsigned int)i) << 4;
        dmaSelector = SelectorMem(sel);
        joshdebug("Allocated dos memory at %x\n", dmaPhysicalAddress);
        if (dmaPhysicalAddress & (DMA_MEM_ALIGN - 1)) {
            dmaMemUsed += DMA_MEM_ALIGN - (dmaPhysicalAddress & (DMA_MEM_ALIGN - 1)); //256-byte aligned
        }
        joshlog("Usable HDA dma memory is at %x (%u bytes)\n",
                dmaPhysicalAddress + dmaMemUsed, dmaMemSize - dmaMemUsed);

        joshlog("Created HdaDevice object\n");
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

    /** If success, sets offset to the offset from the start of DMA */
    bool reserveDMA(unsigned long size, unsigned long &offset) {
        if (!size)
            return false;
        if (!dmaPhysicalAddress)
            return false;
        if (size > (dmaMemSize - dmaMemUsed)) {
            return false;
        }
        offset = dmaMemUsed;
        joshlog("Reserved %u bytes at %x\n", size, offset);
        dmaMemUsed += size;
        if (size & (DMA_MEM_ALIGN - 1)) {
            dmaMemUsed += DMA_MEM_ALIGN - (size & (DMA_MEM_ALIGN - 1));
        }
        return true;
    }


    bool activate() {
        if (!dmaPhysicalAddress) {
            joshlog("Failed to activate because no mem\n");
            return false;
        }
        force_reset();
        joshdebug("Bringing HDA device out of reset...\n");
        WAKEEN.poke(0);
        GCTL.poke(1);
        joshdebug("Waiting for device...\n");
        usleep(600);
        while((GCTL.peek() & 0x1) == 0) INLINE_PAUSE;
        joshlog("HDA Device is out of reset\n");
        //TODO: Enable PCI bus mastering

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
            joshlog("Unable to determine CORB size capability\n");
            corbSizeInCommands = 256;
            CORBSIZE.poke(2);
        }
        if (!reserveDMA(corbSizeInCommands * 4, corbDmaOffset)) {
            joshlog("Failed to reserve CORB DMA memory\n");
            return false;
        }
        corbMemoryInitialzed = true;
        joshdebug("CORB sizeInCommands=%u,CORBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
                corbSizeInCommands, CORBSIZE.peek(), corbDmaOffset, dmaPhysicalAddress + corbDmaOffset);
        CORB.poke(dmaPhysicalAddress + corbDmaOffset);
        CORBUBASE.poke(0);
        CORBRP.poke(0x8000);
        while(!(CORBRP.peek() & 0x8000)) INLINE_PAUSE;
        CORBRP.poke(0);
        while(CORBRP.peek()) INLINE_PAUSE;
        CORBWP.poke(0);
        joshdebug("CORBWP=%u,CORBRP=%u\n", CORBWP.peek(), CORBRP.peek());
        if (CORBWP.peek() | CORBRP.peek()) {
            joshlog("CORB pointers in unexpected position\n");
            return false;
        }
        joshdebug("Starting CORB DMA Engine...");
        CORBCTL.poke(0x2);
        while(!(CORBCTL.peek() & 0x2)) INLINE_PAUSE;
        joshdebug("CORBWP=%u,CORBRP=%u\n", CORBWP.peek(), CORBRP.peek());
        if (CORBWP.peek() | CORBRP.peek()) {
            joshlog("CORB pointers in unexpected position\n");
            return false;
        }
        joshlog("CORB appears to be running!\n");

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
            joshlog("Unable to determine RIRB size capability\n");
            rirbSizeInCommands = 256;
            RIRBSIZE.poke(2);
        }
        RINTCNT.poke(210);
        if (!reserveDMA(rirbSizeInCommands * 8, rirbDmaOffset)) {
            joshlog("Failed to reserve RIRB DMA memory\n");
            return false;
        }
        rirbMemoryInitialzed = true;
        joshdebug("RIRB sizeInCommands=%u,RIRBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
                rirbSizeInCommands, RIRBSIZE.peek(), rirbDmaOffset, dmaPhysicalAddress + rirbDmaOffset);
        RIRBLBASE.poke(dmaPhysicalAddress + rirbDmaOffset);
        RIRBUBASE.poke(0);
        RIRBWP.poke(0x8000);
        rirbReadPointer = RIRBWP.peek();
        //joshlog("RIRBWP=%u,rirbReadPointer=%u\n", RIRBWP.peek(), rirbReadPointer);
        if (RIRBWP.peek() | rirbReadPointer) {
            joshlog("RIRB pointers in unexpected position\n");
            return false;
        }
        joshdebug("RIRB sizeInCommands=%u,RIRBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
                rirbSizeInCommands, RIRBSIZE.peek(), rirbDmaOffset, dmaPhysicalAddress + rirbDmaOffset);
        joshdebug("Starting RIRB DMA Engine...\n");
        RIRBCTL.poke(0x2);
        while(!(RIRBCTL.peek() & 0x2)) INLINE_PAUSE;
        joshdebug("RIRBWP=%u,rirbReadPointer=%u\n", RIRBWP.peek(), rirbReadPointer);
        if (RIRBWP.peek() | rirbReadPointer) {
            joshlog("RIRB pointers in unexpected position\n");
            return false;
        }
        joshdebug("RIRB sizeInCommands=%u,RIRBSIZE=0x%02x,dmaOffset=0x%x, phys=0x%08X\n",
                rirbSizeInCommands, RIRBSIZE.peek(), rirbDmaOffset, dmaPhysicalAddress + rirbDmaOffset);
        joshlog("RIRB appears to be running!\n");

        corbRirbSystemsActive = true;

        if (!reserveDMA(128, dmaPosOffset)) {
            joshlog("Failed to reserve dma space for the DmaPos table");
        }
        for(int zb=0;zb<128;zb+=4) {
            dmaSelector.poke32(dmaPosOffset + zb, 0xBADF00D);
        }
        DPUBASE.poke(0);
        DPLBASE.poke((dmaPhysicalAddress + dmaPosOffset) | 1);

        return true;
    }
    void dumpDmaBuf() {
        for(int i =0 ; i < 4; i++) {
            unsigned int p[8];
            for(int j=0; j<8;j++) {
                p[j] = dmaSelector.peek32(dmaPosOffset + i*32 + j * 4);
            }
            joshlog("DMA %02X %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
                    );
        }
    }

    void dumpRegs() {
        for(int i =0 ; i < 16; i++) {
            unsigned int p[8];
            for(int j=0; j<8;j++) {
                SelectorMem::ref32 x = regs.r32(i*32 + j * 4);
                p[j] = x.peek();
            }
            joshlog("REGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
            );
        }
    }
    void dumpVendorRegs() {
        for(int i =0 ; i < 2; i++) {
            unsigned int p[8];
            for(int j=0; j<8;j++) {
                SelectorMem::ref32 x = regs.r32(i*32 + j * 4 + 0x1000);
                p[j] = x.peek();
            }
            joshlog("VREGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
            );
        }
    }
    void dumpExtendedRegs() {
        for(int i =0 ; i < 2; i++) {
            unsigned int p[8];
            for(int j=0; j<8;j++) {
                SelectorMem::ref32 x = regs.r32(i*32 + j * 4 + 0x2000);
                p[j] = x.peek();
            }
            joshlog("EREGS %02X - %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    i * 32, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]
            );
        }
    }

    void force_reset() {
        joshlog("Resetting the HDA...\n");
        INTCTL.poke(0);

        joshdebug("Stopping all streams...\n");
        unsigned short gcap = GCAP.peek();
        unsigned int scnt = ((gcap >> 12) & 0xF) + ((gcap >> 8) & 0xF) + ((gcap >> 3) & 0x1F);
        for(unsigned int i=0; i<scnt && i <30; i++) {
            joshdebug("Stopping streams %u...\n",i);
            unsigned int x = regs.peek32(0x80 + i * 0x20);
            x &= 0xFFFFFF; // oddly, only 3 byte register
            x &= (~2); //Don't run
            regs.poke32(0x80 + i * 0x20, x);
        }

        joshdebug("Stopping DPL...\n");
        DPLBASE.poke(DPLBASE.peek() & ~1);


        joshdebug("Stopping response dma...\n");
        RIRBCTL.poke(0);
        while((RIRBCTL.peek() & 0x2) != 0) INLINE_PAUSE;
        joshdebug("Stopping command dma...\n");
        CORBCTL.poke(0);
        while((CORBCTL.peek() & 0x2) != 0) INLINE_PAUSE;
        joshdebug("Resetting device...\n");
        GCTL.poke(0);
        while((GCTL.peek() & 0x1) != 0) INLINE_PAUSE;

        GCTL.poke(1);

        joshlog("HDA has been reset\n");

    }


    unsigned short getGlobalCapabilities() { return GCAP.peek(); }
    unsigned int getNumberOfOutputStreamsSupported() { return (getGlobalCapabilities() >> 12) & 0xF; }
    unsigned int getNumberOfInputStreamsSupported() { return (getGlobalCapabilities() >> 8) & 0xF; }
    unsigned int getNumberOfBidirectionalStreamsSupported() { return (getGlobalCapabilities() >> 3) & 0x1F; }
    unsigned int getNumberOfSerialDataOutSignals() { return (getGlobalCapabilities() >> 1) & 0x3; }
    unsigned int get64BitAddressSupported() { return getGlobalCapabilities() & 0x1; }

    unsigned short getCodecBitMap() { return STATESTS.peek() & 0x7FFF; }

    bool getAcceptsUnsolicitedResponse() { return (GCTL.peek() & 0x100) != 0; }


    bool singleCommand(unsigned long command,  unsigned long &response) {
        joshdebug("Sending=%08x\n",command);
        if (!corbRirbSystemsActive) {
            joshlog("Error: CORB/RIRB not active\n");
            return false;
        }
        if (getAcceptsUnsolicitedResponse()) {
            joshlog("Error: Unsolicited responses not supported\n");
            return false;
        }
        if (CORBRP.peek() != CORBWP.peek()) {
            joshlog("Error: command already in progress\n");
            return false;
        }
        while (RIRBWP.peek() != rirbReadPointer) {
            joshlog("Error: Unread responses exist\n");
            rirbReadPointer = RIRBWP.peek();
            //return false;
        }

        //TODO - Need to setup timeouts and maybe kill pending reads if they fail
        unsigned short nextWritePtr = ((CORBWP.peek() & 0xFF) + 1) & (corbSizeInCommands - 1);
        joshdebug("nextWritePtr=%u\n", nextWritePtr);
        dmaSelector.poke32(corbDmaOffset + 4 * nextWritePtr, command);
        joshdebug("Poked %08X at DMA 0x%x\n", command, corbDmaOffset + 4 * nextWritePtr);
        CORBWP.poke(nextWritePtr);

        joshdebug("Waiting for command acceptance...\n");
        dbgCommandState();
        //TODO - spin then timeout
        while(CORBWP.peek() != CORBRP.peek()) {
            INLINE_PAUSE;
            dbgCommandState();
        }

        joshdebug("Waiting for response...\n");
        dbgCommandState();
        //TODO - spin then timeout
        while(RIRBWP.peek() == rirbReadPointer) {
            INLINE_PAUSE;
            dbgCommandState();
        };
        rirbReadPointer = RIRBWP.peek();
        unsigned long v1 = dmaSelector.peek32(rirbDmaOffset + 8 * rirbReadPointer);
        unsigned long v2 = dmaSelector.peek32(rirbDmaOffset + 8 * rirbReadPointer + 4);
        joshdebug("Received %u/%u\n", v1, v2);
        //TODO: Validated codec # and unsolicited flag in v2
        response = v1;
        return true;

    }

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
                joshlog("COMMAND: 0x%08x => 0x%08x\n",cmd, r);

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

    void dbgCommandState() {
        joshdebug("GCTL=%lu\n", GCTL.peek());
        dbgCorb();
        dbgRirb();
    }

    void dbgCorb() {
        joshdebug("CORB L/U=%lx/%lx W/R=%hu/%hu C/S/Z=%hhu/%hhu/%hhu %8.8x %8.8x\n",
                CORB.peek(), CORBUBASE.peek(),
                CORBWP.peek(), CORBRP.peek(),
                CORBCTL.peek(), CORBSTATUS.peek(), CORBSIZE.peek(),
                dmaPhysicalAddress ? dmaSelector.peek32(corbDmaOffset) : 0,
                dmaPhysicalAddress ? dmaSelector.peek32(corbDmaOffset+4) : 0
                );
    }
    void dbgRirb() {
        joshdebug("RIRB L/U=%lx/%lx W/R=%hu/%hu C/S/Z=%hhu/%hhu/%hhu %8.8x %8.8x %8.8x %8.8x\n",
                RIRBLBASE.peek(), RIRBUBASE.peek(),
                RIRBWP.peek(), rirbReadPointer,
                RIRBCTL.peek(), RIRBSTS.peek(), RIRBSIZE.peek(),
                dmaPhysicalAddress ? dmaSelector.peek32(rirbDmaOffset) : 0,
                dmaPhysicalAddress ? dmaSelector.peek32(rirbDmaOffset+4) : 0,
                dmaPhysicalAddress ? dmaSelector.peek32(rirbDmaOffset + 8) : 0,
                dmaPhysicalAddress ? dmaSelector.peek32(rirbDmaOffset+12) : 0
        );
    }


};

class HdaStreamBuffer {
private:
    HdaDevice *dev;
    unsigned char descriptorNumber;
    unsigned char streamNumber;
    unsigned char bufferCount;
    unsigned int singleBufferSize;
    unsigned int totalBufferSize;
    unsigned long offsetBufferDescriptorList = 0;
    unsigned long offsetBuffers = 0;

    SelectorMem::ref32 SDCTL;
    SelectorMem::ref8 SDSTS;
    SelectorMem::ref32 SDLPIB;
    SelectorMem::ref32 SDCBL;
    SelectorMem::ref16 SDLVI;
    SelectorMem::ref16 SDFIFOS;
    SelectorMem::ref16 SDFMT;
    SelectorMem::ref32 SDBDPL;
    SelectorMem::ref32 SDBDPU;

public:
    HdaStreamBuffer(HdaDevice *d, unsigned int bsize, unsigned char bcount, unsigned char descNo, unsigned char streamNo)
        : dev(d)
        , descriptorNumber(descNo)
        , streamNumber(streamNo)
        , bufferCount(bcount)
        , singleBufferSize(bsize)
        , totalBufferSize(bsize * bcount)
        , SDCTL(d->regs, 0x80 + descNo * 0x20)
        , SDSTS(d->regs, 0x83 + descNo * 0x20)
        , SDLPIB(d->regs, 0x84 + descNo * 0x20)
        , SDCBL(d->regs, 0x88 + descNo * 0x20)
        , SDLVI(d->regs, 0x8C + descNo * 0x20)
        , SDFIFOS(d->regs, 0x90 + descNo * 0x20)
        , SDFMT(d->regs, 0x92 + descNo * 0x20)
        , SDBDPL(d->regs, 0x98 + descNo * 0x20)
        , SDBDPU(d->regs, 0x9C + descNo * 0x20)
        {
        joshlog("Setting up stream desc %u\n", descNo);
        const unsigned int ctlUpperBits = ((streamNumber & 0xF) << 20)
                                          //| ( 1 << 19)
                                          //| (1 << 18);
                                          ;
        //Putting stream in reset
        joshlog("Resetting the stream...\n");
        SDCTL.poke(ctlUpperBits | 1);
        while(!(SDCTL.peek() & 1)) INLINE_PAUSE;
        SDCTL.poke(ctlUpperBits);
        while((SDCTL.peek() & 1)) INLINE_PAUSE;
        joshlog("Stream is reset\n");

        if (!dev->reserveDMA(128, offsetBufferDescriptorList)) {
            joshlog("Failed to reserve BDL");
            return;
        }
        if (!dev->reserveDMA(totalBufferSize, offsetBuffers)) {
            joshlog("Failed to reserve buffers");
            return;
        }
        for(unsigned int i=0; i < totalBufferSize; i+=2) {
            dev->dmaSelector.poke16(offsetBuffers + i, (i & 0x200) ? 0x1000 : 0xF000 );
        }
        for(unsigned int i=0; i < totalBufferSize; i+=64) {
            dev->dmaSelector.flushLine(offsetBuffers + i);
        }
        for(unsigned int i=0; i < bufferCount; i++) {
            dev->dmaSelector.poke32(offsetBufferDescriptorList + i * 0x10,
                                    dev->dmaPhysicalAddress + offsetBuffers
                                        + i * singleBufferSize
                                    );
            dev->dmaSelector.poke32(offsetBufferDescriptorList + i * 0x10 + 0x4, 0);
            dev->dmaSelector.poke32(offsetBufferDescriptorList + i * 0x10 + 0x8, singleBufferSize);
            dev->dmaSelector.poke32(offsetBufferDescriptorList + i * 0x10 + 0xC, 0);

            joshlog("BDL @ %08x (%08x): %08x %08x %08x\n",
                    dev->dmaPhysicalAddress + offsetBufferDescriptorList + i * 0x10,
                    offsetBufferDescriptorList + i * 0x10,
                    dev->dmaSelector.peek32(offsetBufferDescriptorList + i * 0x10 + 0),
                    dev->dmaSelector.peek32(offsetBufferDescriptorList + i * 0x10 + 0x4),
                    dev->dmaSelector.peek32(offsetBufferDescriptorList + i * 0x10 + 0x8),
                    dev->dmaSelector.peek32(offsetBufferDescriptorList + i * 0x10 + 0xC)
                    );
            dev->dmaSelector.flushLine(offsetBufferDescriptorList + i * 0x10);
        }
        SDCTL.poke(ctlUpperBits);
        if (dev->get64BitAddressSupported())
            SDBDPU.poke(0);
        SDBDPL.poke(dev->dmaPhysicalAddress + offsetBufferDescriptorList);
        joshlog("SDBDPL(0x%x) is 0x%x\n", SDBDPL.offset,  SDBDPL.peek());

        SDCBL.poke(totalBufferSize); //TODO: This might be in samples!
        SDLVI.poke(bufferCount - 1);
        SDFMT.poke(
                (1 << 14)
                | ( 1 << 4)
                | 1
                );

    }

    ~HdaStreamBuffer() {
        stop();

        joshlog("Resetting stream %d\n", streamNumber);
        SDCTL.poke(SDCTL.peek() | 1);
        while(!(SDCTL.peek() & 1)) {
            INLINE_PAUSE ;
        }
        joshlog("Unresetting stream %d\n", streamNumber);
        SDCTL.poke(SDCTL.peek() & ~1);
        while(SDCTL.peek() & 1) {
            INLINE_PAUSE ;
        }
        joshlog("SDCTL=0x%x\n", SDCTL.peek());
        joshlog("SDBDPL=0x%x\n", SDBDPL.peek());
    }

    void run() {
        SDCTL.poke(SDCTL.peek() | 2);
        joshlog("SDCTL=0x%x\n", SDCTL.peek());
        joshlog("SDBDPL=0x%x\n", SDBDPL.peek());
    }
    void stop() {
        joshlog("Stopping stream %d\n", streamNumber);
        SDCTL.poke(SDCTL.peek() & ~2);
        while(SDCTL.peek() & 2) {
            INLINE_PAUSE ;
        }
        joshlog("SDCTL=0x%x\n", SDCTL.peek());
    }

    unsigned long getDmaPos() {
        //TODO - The SCH HDA device on my Asus netbook puts this at a wierd place
        //return dev->dmaSelector.peek32(dev->dmaPosOffset + 8 * descriptorNumber);

        return dev->dmaSelector.peek32(dev->dmaPosOffset + 8 * 4);
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
    void dumpBufferDescriptorList() {
        for(int i =0 ; i < bufferCount; i++) {
            unsigned int p[4];
            for(int j=0; j<4;j++) {
                p[j] = dev->dmaSelector.peek32(offsetBufferDescriptorList + i*16 + j * 4);
            }
            joshlog("BDL %02X %08X %08X %08X %08X\n",
                    i , p[0],p[1],p[2],p[3]
            );
        }
    }

    void testWriteToBuffer(unsigned int mask) {
        for(unsigned int i=0; i < totalBufferSize; i+=2) {
            dev->dmaSelector.poke16(offsetBuffers + i, (i & mask) ? 0x1000 : 0xF000 );
        }
        for(unsigned int i=0; i<totalBufferSize; i += 64) {
            dev->dmaSelector.flushLine(offsetBuffers + i);
        }
        dev->dmaSelector.flushLine(offsetBuffers + totalBufferSize - 1);
    }
    //TODODMP
};


static void lookupLogCode(unsigned int singleCodeLength, const char *allCodes, unsigned int value, char *result ) {
    result[0] = 0;
    if (singleCodeLength == 0 || !allCodes) {
        return;
    }
    unsigned int numCodes = strlen(allCodes) / singleCodeLength;
    if (numCodes == 0) {
        return;
    }
    if (value >= numCodes) {
        return;
    }
    memcpy(result, allCodes + (singleCodeLength * value), singleCodeLength);
    result[singleCodeLength] = 0;
}




static const int CON_LIST_BUF_LEN = 16;

struct amp_capabilities {
    unsigned long caps;

    unsigned int getStepSize() const { return (caps >> 16) & 0x7F ; }
    unsigned int getNumSteps() const { return (caps >> 8) & 0x7F ; }
    unsigned int getOffset() const { return (caps) & 0x7F ; }
    bool getMuteCapable() const { return (caps & 0x80000000u) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        joshlog("%sM=%d,SS=%u,NS=%u,O=%u\n",
                prefix,getMuteCapable() ? 1: 0, getStepSize(), getNumSteps(), getOffset());
    }
};

struct widget_capabilities {
    unsigned long caps;
    unsigned int getWidgetType() const { return (caps >> 20) & 0xF; }
    unsigned int getDelay() const { return (caps >> 16) & 0xF; }
    unsigned int getChannelCount() const { return 1 + (((caps >> 12) & 0xE) | (caps & 0x1)); }
    bool hasCpCaps() const { return (caps & 0x1000) != 0; }
    bool hasLrSwap() const { return (caps & 0x800) != 0; }
    bool hasPowerControl() const { return (caps & 0x400) != 0; }
    bool isDigital() const { return (caps & 0x200) != 0; }
    bool hasConnectionList() const { return (caps & 0x100) != 0; }
    bool isUnsolCapable() const { return (caps & 0x80) != 0; }
    bool isProcWidget() const { return (caps & 0x40) != 0; }
    bool isStripeSupported() const { return (caps & 0x20) != 0; }
    bool hasFormatOverride() const { return (caps & 0x10) != 0; }
    bool hasAmpOverride() const { return (caps & 0x8) != 0; }
    bool hasOutputAmp() const { return (caps & 0x4) != 0; }
    bool hasInputAmp() const { return (caps & 0x2) != 0; }
    bool isStereo() const { return (caps & 0x1) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        unsigned long widType = (caps >> 20) & 0xF;
        static const char flagNames[] = "_X*DLUP%FAOIS";
        static const char typeNames[] = "AOAIAMASPCPWVKBG08090A0B0C0D0E0F";
        char flags[sizeof(flagNames)];
        memcpy(flags, flagNames, sizeof(flagNames));
        const char flagCount = sizeof(flagNames) - 1;

        for (int fi = 0; fi < flagCount; fi++) {
            if (!((caps >> (flagCount - 1 - fi)) & 0x1)) {
                flags[fi] = ' ';
            }
        }
        char typeCode[3];
        typeCode[0] = typeNames[widType * 2];
        typeCode[1] = typeNames[widType * 2 + 1];
        typeCode[2] = 0;

        joshlog("%s%s,flags=%s,delay=%u,channels=%u\n",
                prefix, typeCode, flags, getDelay(),getChannelCount());

    }
};

struct pin_capabilities {
    unsigned long caps;

    bool canHighBitRate() const { return (caps & 0x8000000) != 0; }
    bool canDisplayPort() const { return (caps & 0x1000000) != 0; }
    bool canEapd() const { return (caps & 0x10000) != 0; }
    unsigned char getVrefControlBits() const { return (caps >> 8) & 0xFF; }
    bool canHdmi() const { return (caps & 0x80) != 0; }
    bool hasBalancedPins() const { return (caps & 0x40) != 0; }
    bool isInputCapable() const { return (caps & 0x20) != 0; }
    bool isOutputCapable() const { return (caps & 0x10) != 0; }
    bool canHeadphoneDrive() const { return (caps & 0x8) != 0; }
    bool canPresenceDetect() const { return (caps & 0x4) != 0; }
    bool isTriggerRequiredForImpedanceSense() const { return (caps & 0x2) != 0; }
    bool isImpedanceSenseCapable() const { return (caps & 0x1) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        unsigned long eapd = canEapd() ? 1 : 0;
        unsigned long vref = getVrefControlBits() & 0xFF;
        static const char pinFlagNames[] = "DBIOHPTZ";
        char pinFlags[sizeof(pinFlagNames)];
        memcpy(pinFlags, pinFlagNames, sizeof(pinFlagNames));
        const char pinFlagCount = sizeof(pinFlagNames) - 1;
        for(int fi=0;fi<pinFlagCount;fi++) {
            if (!((caps >> (pinFlagCount - 1 - fi)) & 0x1)) {
                pinFlags[fi] = ' ';
            }
        }
        joshlog("%seapd=%u,vref=%02x,flags=%s\n",
                prefix, eapd,vref, pinFlags);
    }
};

struct volume_knob_capabilities {
    unsigned long caps;

    bool isPresent() const { return caps != 0; }

    bool isDelta() const { return (caps & 0x80) != 0; }

    unsigned int getNumSteps() const { return caps & 0x7f; }

    void log(const char *prefix) const {
        joshlog("%sdelta=%u,numsteps=%u\n",
                prefix, isDelta() ? 1 : 0, getNumSteps());
    }
};

struct supported_pcm_caps {
    unsigned long caps;
    bool isPresent() const { return caps != 0; }
    unsigned int getDepthBits() const { return ( caps >> 16) & 0x1F; }
    unsigned int getRateBits() const { return caps & 0xFFF; }

    void log(const char *prefix) const {
        char depth[100];
        int dlen = 0;
        static const char depthNames[] = " 816202432";
        for(int i = 0; i < 5; i++) {
            if ((getDepthBits() >> i) & 0x1) {
                if (dlen)
                    depth[dlen++] = ',';
                depth[dlen++] = depthNames[i*2];
                depth[dlen++] = depthNames[i*2+1];
            }
        }
        depth[dlen] = 0;
        char rate[200];
        int rlen = 0;
        static const char rateNames[] = "  8 11 16 22 32 44 48 88 96176192384";
        for(int i = 0; i < 12; i++) {
            if ((getRateBits() >> i) & 0x1) {
                if (rlen)
                    rate[rlen++] = ',';
                rate[rlen++] = rateNames[i*3];
                rate[rlen++] = rateNames[i*3+1];
                rate[rlen++] = rateNames[i*3+2];
            }
        }
        rate[rlen] = 0;
        joshlog("%sdepths=%s,rates=%s\n",prefix, depth, rate);
    }
};

struct supported_stream_format_caps {
    unsigned long caps;
    bool isPresent() const { return caps != 0; }
    bool canAc3() const { return (caps & 0x4) != 0; }
    bool canFloat32() const { return (caps & 0x2) != 0; }
    bool canPcm() const { return (caps & 0x1) != 0; }
    void log(const char *prefix) const {
        joshlog("%spcm=%u,float32=%u,ac3=%u\n",
                prefix, canPcm() ? 1: 0, canFloat32() ? 1 : 0, canAc3() ? 1: 0);
    }
};

struct config_default {
    unsigned long dflt;

    unsigned int getPortConnectivityBits() const { return (dflt >> 30) & 0x3; }
    unsigned int getGrossLocationBits() const { return (dflt >> 28) & 0x3; }
    unsigned int getGeometricLocationBits() const { return (dflt >> 24) & 0xf; }
    unsigned int getDefaultDeviceBits() const { return (dflt >> 20) & 0xf; }
    unsigned int getConnectionTypeBits() const { return (dflt >> 16) & 0xf; }
    unsigned int getColorBits() const { return (dflt >> 12) & 0xf; }
    bool getJackDetectOverrideToIncapable() const { return (dflt & 0x100) != 0; }
    unsigned int getDefaultAssociation() const { return (dflt >> 4) & 0xf; }
    unsigned int getSequence() const { return dflt & 0xf; }
    bool isPresent() const { return dflt != 0; }

    void log(const char *prefix) const {
        char portConn[3];
        lookupLogCode(2, "JANOFIBO",
                      getPortConnectivityBits(), portConn);
        char grossLoc[3];
        lookupLogCode(2, "EXINSEOT",
                      getGrossLocationBits(), grossLoc);
        char geomLoc[3];
        lookupLogCode(2, "NAREFRLERITOBO0708090A0B0C0D0E0F",
                      getGeometricLocationBits(), geomLoc);
        char dfltDev[3];
        lookupLogCode(2, "LOSPHPCDSPDOMLMHLIAUMCTESPDO0EOT",
                      getDefaultDeviceBits(), dfltDev);
        char connTyp[3];
        lookupLogCode(2, "UN/8/4ATRCOPODOADIXLRJCO0C0D0EOT",
                      getConnectionTypeBits(), connTyp);
        char color[3];
        lookupLogCode(2, "UNBKGRBUGRREORYEPUPI0A0B0C0DWHOT",
                      getColorBits(), color);
        unsigned int jdoFlag = getJackDetectOverrideToIncapable() ? 1 : 0;
        joshlog("%sLoc=%s-%s,Conn=%s-%s,Dev=%s,Col=%s,Jdo=%u,Assoc=%u/%u\n"
                  ,prefix,grossLoc,geomLoc,portConn,connTyp,dfltDev,color
                  ,jdoFlag,getDefaultAssociation(),getSequence());
    };
};

struct widget_info {
    unsigned short nodeNumber;
    widget_capabilities widgetCaps;
    pin_capabilities pinCaps;
    amp_capabilities inputAmpCaps;
    amp_capabilities outputAmpCaps;
    unsigned long connectionListCaps;
    volume_knob_capabilities volumeKnobCaps;
    unsigned short numConns;
    unsigned short connList[CON_LIST_BUF_LEN];
    config_default configDefault;
    supported_pcm_caps pcmCaps;
    supported_stream_format_caps streamFormatCaps;

    void load(HdaDevice &dev, unsigned int codecNo, unsigned int nodeNo) {
        HdaDevice::Codec codec(dev, codecNo);
        nodeNumber = nodeNo;
        widgetCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_AUDIO_WIDGET_CAPABILITIES);
        pinCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_PIN_CAPABILITIES);
        inputAmpCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES);
        outputAmpCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES);
        //Workaround for Virtualbox emulator bug - output amp caps and input amp caps are switched
        //Check if widget indicates it has amplifier overrides
        if (widgetCaps.hasAmpOverride()) {
            bool outa = widgetCaps.hasOutputAmp();
            bool ina = widgetCaps.hasInputAmp();
            if (
                    outa != ina
                    && inputAmpCaps.isPresent() != outputAmpCaps.isPresent()
                    && ina != inputAmpCaps.isPresent()) {
                unsigned  long x= inputAmpCaps.caps;
                inputAmpCaps.caps = outputAmpCaps.caps;
                outputAmpCaps.caps = x;
            }
        }

        connectionListCaps = codec.getNodeParam(nodeNo, NODE_PARAM_CONNECTION_LIST_LENGTH);
        volumeKnobCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_VOLUME_KNOB_CAPABILITIES);
        streamFormatCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_SUPPORTED_STREAM_FORMATS);
        pcmCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_SUPPORTED_PMC_RATES);

        //Workaround for Virtualbox
        if (widgetCaps.getWidgetType() == WIDGET_TYPE_VOLUME_KNOB) {
            //VirtualBox seems to have misread the volumeKnobCapabilities param number
            //of 13H as a decimal number and put the configuration in 13 decimal, which
            //is input amp capabilities.
            if (!volumeKnobCaps.isPresent() && inputAmpCaps.isPresent()) {
                volumeKnobCaps.caps = inputAmpCaps.caps;
                inputAmpCaps.caps = 0;
            }
        }

        numConns = connectionListCaps & 0x7F;
        numConns = numConns > CON_LIST_BUF_LEN ? CON_LIST_BUF_LEN : numConns;
        memset(&connList, 0, sizeof(connList));
        for(int ci=0;ci < numConns;) {
            //TODO - This is wrong - need to check for range values and handle
            // them correctly
            unsigned long r= codec.nodeVerb(nodeNo, 0xf02, ci);
            unsigned short mask = connectionListCaps & 0x80 ? 0xFFFF : 0xFF;
            unsigned short shift = connectionListCaps & 0x80 ? 16 : 8;
            int cnt = connectionListCaps & 0x80 ? 2 : 4;
            for(int cj=0; cj < cnt && ci < numConns; cj++, ci++) {
                connList[ci] = r & mask;
                r = r >> shift;
            }
        }
        configDefault.dflt = codec.nodeVerb(nodeNo, 0xf1c, 0);

        //NOTE: Linux seems to set the defaults itself.  Here is what virtualBox uses:
        //https://github.com/torvalds/linux/blob/8ca09d5fa3549d142c2080a72a4c70ce389163cd/sound/pci/hda/patch_sigmatel.c#L3287

    }

    bool hasInputFrom(unsigned short fromNode) const {
        for(unsigned int i=0; i<numConns;i++) {
            if (connList[i] == fromNode)
                return true;
        }
    }

    int getOffsetOfInputConnection(unsigned short fromNode) const {
        for(int i=0; i<numConns;i++) {
            if (connList[i] == fromNode)
                return i;
        }
        return -1;
    }

    unsigned int getWidgetType() const {
        return widgetCaps.getWidgetType();
    }
    void logReport() const {
        {
            char p[20];
            sprintf(p," WIDGET %-4u: ", nodeNumber);
            widgetCaps.log(p);
        }
        if (pinCaps.isPresent()) {
            pinCaps.log("      PINCAP: ");
        }
        if (inputAmpCaps.isPresent()) {
            inputAmpCaps.log("      IN AMP: ");
        }
        if (outputAmpCaps.isPresent()) {
            outputAmpCaps.log("     OUT AMP: ");
        }
        if (numConns > 0) {
            char connl[CON_LIST_BUF_LEN * 6 + 1];  //5 chars for num, 1 for space.
            connl[0] = 0;
            for(unsigned int i = 0; i < numConns; i++) {
                char e[8];
                sprintf(e, " %d", connList[i] & 0xFFFF); //Safety mask
                strcat(connl, e);
            }
            joshlog("   CONN LIST:%s\n",connl);
        }
        if (configDefault.isPresent()) {
            configDefault.log("      CONFIG: ");
        }
        if (volumeKnobCaps.isPresent()) {
            volumeKnobCaps.log("       VKNOB: ");
        }
        if (streamFormatCaps.isPresent()) {
            streamFormatCaps.log("         FMT: ");
        }
        if (pcmCaps.isPresent()) {
            pcmCaps.log("         PCM: ");
        }
    }
};



struct audio_function_group_capabilities {
    unsigned long caps;

    bool isPresent() const { return caps != 0; }
    bool hasBeepGen() const { return (caps & 0x10000) != 0; }
    unsigned int getInputDelay() const { return (caps >> 8) & 0xF; }
    unsigned int getOutputDelay() const { return caps & 0xF; }

    void log(const char *prefix) const {
        joshlog("%sbeep=%u,inputDelay=%u,outputDelay=%u\n",
                prefix, hasBeepGen() ? 1 : 0, getInputDelay(), getOutputDelay());
    }

};

static const unsigned int max_audio_widgets = 256;

struct audio_function_group_info {
    unsigned int nodeNumber;
    unsigned char functionGroupType;
    bool canProduceUnsolicitedMessages;

    audio_function_group_capabilities fgCaps;
    amp_capabilities inputAmpCaps;
    amp_capabilities outputAmpCaps;
    supported_pcm_caps pcmCaps;
    supported_stream_format_caps streamFormatCaps;

    unsigned short widgetCount;
    widget_info widgets[max_audio_widgets];
    unsigned short widgetNodeOffset;

    const widget_info * lookupNode (unsigned char nodeNum) const {
        if (nodeNum < widgetNodeOffset)
            return nullptr;
        unsigned short i = nodeNum - widgetNodeOffset;
        if (i >= widgetCount)
            return nullptr;
        return widgets + i;
    }

    void load(HdaDevice &dev, unsigned int codecNo, unsigned int node) {
        HdaDevice::Codec codec(dev, codecNo);
        memset(this, 0, sizeof(*this));
        this->nodeNumber = node;
        unsigned long fgTypeRes = codec.getNodeParam(node, NODE_PARAM_FUNCTION_GROUP_TYPE);
        this->canProduceUnsolicitedMessages = (fgTypeRes & 0x100) != 0;
        this->functionGroupType = fgTypeRes & 0xFF;


        if (this->functionGroupType == NODE_TYPE_AUDIO_FUNCTION_GROUP) {
            fgCaps.caps = codec.getNodeParam(node, NODE_PARAM_AUDIO_FUNCTION_GROUP_CAPABILITIES);
            inputAmpCaps.caps = codec.getNodeParam(node, NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES);
            outputAmpCaps.caps = codec.getNodeParam(node, NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES);
            streamFormatCaps.caps = codec.getNodeParam(node, NODE_PARAM_SUPPORTED_STREAM_FORMATS);
            pcmCaps.caps = codec.getNodeParam(node, NODE_PARAM_SUPPORTED_PMC_RATES);

            unsigned long subordinates = codec.getNodeParam(node, NODE_PARAM_SUBORDINATE_NODES);
            widgetNodeOffset = (subordinates >> 16) & 0xFF;
            widgetCount = subordinates & 0xFF;
            for (unsigned short i = 0; i < widgetCount; i++) {
                widgets[i].load(dev, codecNo, widgetNodeOffset + i);
            }
        }
    }

    void logReport() const {
        char prefix[20];
        sprintf(prefix, "FG NODE %-4u: ", nodeNumber);
        fgCaps.log(prefix);
        if (inputAmpCaps.isPresent()) {
            inputAmpCaps.log("      IN AMP: ");
        }
        if (outputAmpCaps.isPresent()) {
            outputAmpCaps.log("     OUT AMP: ");
        }
        if (streamFormatCaps.isPresent()) {
            streamFormatCaps.log("         FMT: ");
        }
        if (pcmCaps.isPresent()) {
            pcmCaps.log("         PCM: ");
        }
        for(unsigned char i=0;i<widgetCount;i++) {
            widgets[i].logReport();
        }

        const widget_info *psp = findSpeaker();
        joshlog("PREFERRED_SPEAKER = %d\n", psp ? psp->nodeNumber : -1);
        if (psp) {
            unsigned char path[32];
            unsigned int len = findPathToDac(psp->nodeNumber, path, 32);
            char lsto[32*8];
            unsigned int lstolen = 0;
            for(unsigned int i=0;i<len;i++) {
                sprintf(lsto+lstolen, "%u ", path[i] + 0);
                lstolen += strlen(lsto + lstolen);
            }
            lsto[lstolen] = 0;
            joshlog("PATH TO DAC: %s\n", lsto);

            unsigned char vks[32];
            unsigned int vklen = findAssociatedVolumeKnobs(path, len, vks, 32);
            lstolen = 0;
            for(unsigned int i=0;i<vklen;i++) {
                sprintf(lsto+lstolen, "%u ", vks[i] + 0);
                lstolen += strlen(lsto + lstolen);
            }
            lsto[lstolen] = 0;
            joshlog("ASSOCIATED KNOBS: %s\n", lsto);

        }


    }

    const widget_info * findSpeaker() const {
        if (!widgetCount)
            return nullptr;
        const widget_info * wptrs[max_audio_widgets];
        for(unsigned short i=0;i<widgetCount;i++) {
            wptrs[i] = widgets + i;
        }
        tqsort<widget_info, best_speaker_comparator>::run(wptrs, widgetCount);
        return wptrs[0];
    }

    unsigned int findPathToDac(unsigned char fromNode, unsigned char *res, unsigned int maxLen) const {
        if (!res || !maxLen)
            return 0;
        nodeset next, seen;
        next.add(fromNode);
        return bfsToDac(res, 0, maxLen - 1, next, seen);
    }

    unsigned int findAssociatedVolumeKnobs(const unsigned char *toNodes, unsigned int toNodesLength,
                                           unsigned char *output, unsigned int outputMaxLen) const {
        if (!output || !outputMaxLen || !toNodes || !toNodesLength)
            return 0;
        unsigned int len = 0;
        for(unsigned int i = 0; i < widgetCount; i++) {
            if (widgets[i].getWidgetType() != WIDGET_TYPE_VOLUME_KNOB)
                continue;
            for(unsigned int j = 0; j < widgets[i].numConns; j++) {
                unsigned short chk = widgets[i].connList[j];
                bool fnd = false;
                for(unsigned int k = 0; k < toNodesLength && !fnd; k++) {
                    fnd |= (toNodes[k] == chk);
                }
                if (fnd) {
                    output[len++] = widgets[i].nodeNumber;
                    if (len >= outputMaxLen)
                        return len;
                }
            }
        }
        return len;
    }


private:
    unsigned int bfsToDac(unsigned char *res, unsigned int curDepth, const unsigned int maxDepth,
                         const nodeset &next, nodeset &seen) const {
        //Some of these checks are somewhat redundant since I check them again before calling myself
        //recursively.  But they do check the initial call, so i guess Ill keep them
        if (curDepth > maxDepth)
            return 0;
        bool nextEmpty = true;
        for (int i = next.findNext(-1); i >= 0; i = next.findNext(i+1)) {
            nextEmpty = false;
            const widget_info *node = lookupNode(i);
            if (node && node->getWidgetType() == WIDGET_TYPE_AUDIO_OUT) {
                res[curDepth] = i;
                return curDepth+1;
            }
        }
        if (nextEmpty || curDepth >= maxDepth)
            return 0;

        seen.addAll(next);
        nodeset n2;
        bool n2Empty = true;
        for (int i = next.findNext(-1); i >= 0; i = next.findNext(i+1)) {
            const widget_info *node = lookupNode(i);
            if (!node)
                continue;
            for(int j=0; j<node->numConns;j++) {
                unsigned short x = node->connList[j];
                if (x < 256 && !seen.contains(x)) {
                    n2.add(x);
                    n2Empty = false;
                }
            }
        }
        if (n2Empty)
            return 0;
        const unsigned int r = bfsToDac(res, curDepth+1, maxDepth, n2, seen);
        if (!r)
            return 0;
        const unsigned char nextNode = res[curDepth+1];
        for (int i = next.findNext(0); i >= 0; i = next.findNext(i+1)) {
            const widget_info *node = lookupNode(i);
            if (!node)
                continue;
            for(int j=0; j<node->numConns;j++) {
                unsigned short x = node->connList[j];
                if (x == nextNode) {
                    res[curDepth] = i;
                    return r;
                }
            }
        }
        joshlog("Dont think I should get here");
        return 0;
    }

    static int best_speaker_comparator(const widget_info &w1, const widget_info &w2) {
        int f1 = w1.getWidgetType() == WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;
        int f2 = w2.getWidgetType() == WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;

        f1 |= w1.configDefault.isPresent() ? 0x4000 : 0;
        f2 |= w2.configDefault.isPresent() ? 0x4000 : 0;

        f1 |= w1.configDefault.getDefaultDeviceBits() == 1 ? 0x2000 : 0;
        f2 |= w2.configDefault.getDefaultDeviceBits() == 1 ? 0x2000 : 0;

        f1 |= w1.numConns == 0 ? 0x1000 : 0;
        f2 |= w2.numConns == 0 ? 0x1000 : 0;

        f1 |= w1.configDefault.getPortConnectivityBits() == 2 ? 0x800 : 0;
        f2 |= w2.configDefault.getPortConnectivityBits() == 2 ? 0x800 : 0;

        f1 |= w1.configDefault.getPortConnectivityBits() == 3 ? 0x400 : 0;
        f2 |= w2.configDefault.getPortConnectivityBits() == 3 ? 0x400 : 0;
        //Prefer higher score from above checks
        if (f1 != f2) {
            return f1 > f2 ? -1 : 1;
        }
        //Prefer smaller association
        if (w1.configDefault.getDefaultAssociation() != w2.configDefault.getDefaultAssociation()) {
            return w1.configDefault.getDefaultAssociation() < w2.configDefault.getDefaultAssociation() ? -1 : 1;
        }
        //Prefer smaller sequence
        if (w1.configDefault.getSequence() != w2.configDefault.getSequence()) {
            return w1.configDefault.getSequence() < w2.configDefault.getSequence() ? -1 : 1;
        }
        //If all else fails, prefer smaller node number
        if (w1.nodeNumber  != w2.nodeNumber) {
            return w1.nodeNumber < w2.nodeNumber ? -1 : 1;
        }
        return 0;
    }

};


static const unsigned int max_audio_function_groups = 4;

/**
 * Note: This is a big object (around 77kb) mostly because we used
 * fixed size arrays to avoid the need for destructors
 */
struct codec_info {
    unsigned int codecNumber;
    unsigned short vendorId;
    unsigned short deviceId;
    unsigned long revisionId;
    unsigned short audioFunctionGroupCount;
    audio_function_group_info audioFunctionGroups[max_audio_function_groups];

    void load(HdaDevice &dev, unsigned int codecNo) {
        HdaDevice::Codec codec(dev, codecNo);
        this->codecNumber = codecNo;
        unsigned long venDevId = codec.getNodeParam(0, NODE_PARAM_DEVICE_ID);
        vendorId = (venDevId >> 16) & 0xFFFF;
        deviceId = venDevId & 0xFFFF;
        revisionId = codec.getNodeParam(0, NODE_PARAM_REVISION_ID);
        unsigned long subordinates = codec.getNodeParam(0, NODE_PARAM_SUBORDINATE_NODES);
        unsigned long subStart = (subordinates >> 16) & 0xFF;
        unsigned long subCount = subordinates & 0xFF;

        audioFunctionGroupCount = 0;
        memset(audioFunctionGroups, 0, sizeof(audioFunctionGroups));
        for(unsigned long i = 0; i<subCount; i++) {
            audio_function_group_info gi{};
            gi.load(dev, codecNo, subStart + i);
            if (gi.functionGroupType == NODE_TYPE_AUDIO_FUNCTION_GROUP) {
                audioFunctionGroups[audioFunctionGroupCount++] = gi;
                if (audioFunctionGroupCount >= max_audio_function_groups) {
                    break;
                }
            }
        }

    }
    void logReport() const {
        joshlog("CODEC %u\n", codecNumber);
        joshlog("ROOT NODE   : venId=%04x,devId=%04x,revId=%08x,afgCount=%u\n",
                vendorId, deviceId, revisionId, audioFunctionGroupCount);
        for(unsigned long i = 0; i<audioFunctionGroupCount; i++) {
            audioFunctionGroups[i].logReport();
        }

    }
};


static void try_it_out(HdaDevice &dev, const codec_info &codec) {

    HdaStreamBuffer myStream(&dev, 4096, 2, dev.getNumberOfInputStreamsSupported(), 1);
    joshlog("streamDescriptorNumber=%d streamNumber=%d\n", myStream.getDescriptorNumber(), myStream.getStreamNumber());
    HdaDevice::Codec codecControl(dev, codec.codecNumber);
    const audio_function_group_info &afg = codec.audioFunctionGroups[0];

    //TODO: This is needed to make virtualbox work on the second run (Without this, the first run works
    // but not subsequent runs without hard-resetting the VM
    joshlog("Root Reset %u\n", 0);
    codecControl.nodeVerb(0, 0x7FF, 0);


    joshdebug("AFG Function RESET\n");
    codecControl.nodeVerb(afg.nodeNumber, 0x7ff, 0);

    const widget_info * speaker = afg.findSpeaker();
    if (!speaker) {
        joshlog("Cannot find speaker\n");
        return;
    }
    unsigned char chain[8];
    unsigned int chainLen = afg.findPathToDac(speaker->nodeNumber, chain, 8);
    if (!chainLen) {
        joshlog("Cannot find DAC\n");
        return;
    }
    unsigned char vkbuf[1];
    const widget_info * volumeKnob;
    if (afg.findAssociatedVolumeKnobs(chain, chainLen, vkbuf, 1)) {
        volumeKnob = afg.lookupNode(vkbuf[0]);
    } else {
        volumeKnob = nullptr;
    }
    const widget_info *dac = afg.lookupNode(chain[chainLen - 1]);

    joshlog("Powering up...");
    //TODO - Hack! power up FG and other stuff
    joshlog("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    joshlog("Powering Down %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 3);
    usleep(100 * 1000);
    joshlog("Powering Up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);
    usleep(100 * 1000);
    joshlog("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    joshlog("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    joshlog("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    joshlog("Powering up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);
    joshlog("Power state of AFG is %ul\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));

    for(unsigned int i = 0; i < chainLen; i++) {
        joshlog("Powering up %u\n", i);
        codecControl.nodeVerb(chain[i], 0x705, 0);
        joshlog("Power state of %d is %ul\n", (int)chain[i], codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    }
    if (volumeKnob) {
        joshlog("Powering up %u\n", volumeKnob->nodeNumber);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x705, 0);
        joshlog("Power state of knob is %ul\n", codecControl.nodeVerb(volumeKnob->nodeNumber, 0xf05, 0));
    }

    for(unsigned int i=0;i<chainLen;i++) {
        const widget_info *cur = afg.lookupNode(chain[chainLen - i - 1]);
        joshlog("Setting up widget %u\n", cur->nodeNumber);
        int prevNode = i > 0 ? chain[chainLen - i] : -1;
        int inputIndex = prevNode >= 0 ?cur->getOffsetOfInputConnection(prevNode) : -1;
        if (cur->widgetCaps.hasInputAmp()) {
            amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                    cur->inputAmpCaps : afg.inputAmpCaps;
            unsigned int payload = 0x7000 |
                ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                (acap.getOffset() & 0x7F);
            joshlog("Setting input amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            if (cur->getWidgetType() == WIDGET_TYPE_AUDIO_MIXER) {
                for(int ii=0;ii<cur->numConns;ii++) {
                    if (ii != inputIndex) {
                        unsigned int pl = 0x7000 |
                                               ((ii & 0xF) << 8) |
                                               0x80;
                        joshlog("muting unused input amp of node %u - PL=%04X\n", cur->nodeNumber, pl);
                        codecControl.nodeVerb(cur->nodeNumber, 0x3, pl);
                    }
                }
            }
        }

        if (cur->widgetCaps.hasOutputAmp()) {
            joshlog("Tweaking output amp of node %u\n", cur->nodeNumber);
            joshlog("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            joshlog("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));

            amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                                    cur->outputAmpCaps : afg.outputAmpCaps;
            unsigned int payloadReset = 0xB000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                                   0;
            joshlog("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payloadReset);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payloadReset);
            // I  used to crank down the volume here, but now I'm not
            unsigned int payload = 0xB000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                    ((acap.getOffset() & 0x7F) );
            joshlog("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            joshlog("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            joshlog("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));

        }
        if (cur->numConns > 1 && prevNode >= 0 && cur->getWidgetType() != WIDGET_TYPE_AUDIO_MIXER) {
            unsigned int payload = cur->getOffsetOfInputConnection(prevNode) & 0xFF;
            joshlog("Setting active connector of node %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x701,payload);
        }
        if (cur->getWidgetType() == WIDGET_TYPE_PIN_COMPLEX) {
            joshlog("Tweaking pin control of node %u\n", cur->nodeNumber);
            joshlog("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
            unsigned int payload = 0x40;
            joshlog("Set pin control of %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x707, payload);
            joshlog("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
        }
        joshlog("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));
        joshlog("Setting EAPD of %u\n", cur->nodeNumber);
        codecControl.nodeVerb(cur->nodeNumber, 0x70C, 0x2);
        joshlog("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));

    }
    if (volumeKnob) {
        unsigned int steps = volumeKnob->volumeKnobCaps.getNumSteps();
        unsigned int payload = 0x80 | (steps & 0x7F);
        joshlog("Setting volume knob %u to %04X\n", volumeKnob->nodeNumber, payload);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x70F, payload);

    }

    codecControl.nodeVerb(dac->nodeNumber, 0x2, myStream.getFormat()); //Set format
    codecControl.nodeVerb(dac->nodeNumber, 0x706, (myStream.getStreamNumber() << 4) + 0);
    //TODO - Set power states
    // TODO - EAPD/BTL ?

    // TODO - Stripe Control ??
    codecControl.nodeVerb(dac->nodeNumber, 0x72D, 1);  // 2 channels
    dev.dumpRegs();
    dev.dumpVendorRegs();
    dev.dumpExtendedRegs();
    dev.dumpDmaBuf();
    myStream.dumpBufferDescriptorList();
    myStream.run();
    for(int i=0; i<60; i++) {
        joshlog("%x %x %x\n", myStream.getDmaPos(), myStream.getLinkPos(), myStream.getFifoSize());
        //dev.dumpRegs();
        //dev.dumpVendorRegs();
        //dev.dumpExtendedRegs();
        //dev.dumpDmaBuf();
        //myStream.dumpBufferDescriptorList();
        usleep(100000);
        if (i==20) {
            myStream.testWriteToBuffer(0x100);
        } else if (i == 40) {
            myStream.testWriteToBuffer(0x400);
        }
    }
    myStream.stop();
    //usleep(3000000);


    //codecControl.nodeVerb(dac->nodeNumber, )

}

class CodecResponse {
public:
    const unsigned long value;

    CodecResponse(unsigned long v) : value(v) {}

};

class NodeContoller {
private:
    HdaDevice::Codec &codec;
    unsigned int nodeNum;
public:
    NodeContoller(HdaDevice::Codec &codec, unsigned int nodeNum)
    : codec(codec)
    , nodeNum(nodeNum) {}


    unsigned long rawVerb(unsigned int verb, unsigned int params) {
        return codec.nodeVerb(nodeNum, verb, params);
    }

    unsigned long getParameter(unsigned char paramId) {
        return rawVerb(0xF00, paramId);
    }

    class ResponseVendorId : CodecResponse {
    public:
        unsigned short vendorId()  { return 0xFFFF & (value >> 16); }
        unsigned short deviceId()  { return 0xFFFF & value; }
        ResponseVendorId(unsigned long v) : CodecResponse(v) {}
    };

    ResponseVendorId getVendorId() {
        unsigned long r = getParameter(0);
        return {r};
    }


    unsigned char getConnectionSelect() {
        return rawVerb(0xF01, 0) & 0xFF;
    }

    void setConnectionSelect(unsigned char value) {
        rawVerb(0x701, value);
    }

};


static void setup_hda() {
    option<PciFunction> hdaFunction = PciFunction::find_hda_function();
    if (!hdaFunction.exists()) {
        joshlog("No HDA found\n");
        return;
    }

    unsigned int allchunks[64];
    for(int line=0;line<8;line++) {
        unsigned int *chunks = allchunks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            chunks[chunk] = hdaFunction->getConfig32(4*(line*8 + chunk));
        }
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                chunks[0],chunks[1],chunks[2],chunks[3],
                chunks[4],chunks[5],chunks[6],chunks[7]);
    }
    joshdebug("CFG ERROR State=%u\n", hdaFunction->errorDiagnosticBits());
    if (hdaFunction->hadErrors()) {
        return;
    }
    unsigned short pciCommand = hdaFunction->getConfig16(0x4);
    joshdebug("HDA PCI COMMAND=%02hx\n", pciCommand);
    if (!(pciCommand & 0x4)) {
        joshlog("HDA Bus Mastering not enabled...fixing!\n");
        hdaFunction->setConfig16(0x4, pciCommand | 0x4);
        pciCommand = hdaFunction->getConfig16(0x4);
        joshlog("NEW HDA PCI COMMAND=%02hx\n", pciCommand);
    }

    option<SelectorMem> devMem = SelectorMem::mapDevice(allchunks[4] & 0xFFFFFFF0u, 4096 * 3);
    if (!devMem.exists()) {
        joshlog("Could not map device memory");
        return;
    }
    unsigned long allpeeks[32];
    for(int line=0;line<4;line++) {
        unsigned long *peeks = allpeeks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            peeks[chunk] = devMem->peek32(4*(line*8+chunk));
        }
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }

    HdaDevice myDev(hdaFunction.get(), devMem.get());
    joshlog("GCAP: os=%d,is=%d,bs=%d,sdo=%d,a64=%d\n", myDev.getNumberOfOutputStreamsSupported(),
           myDev.getNumberOfInputStreamsSupported(),
           myDev.getNumberOfBidirectionalStreamsSupported(),
           myDev.getNumberOfSerialDataOutSignals(),
           myDev.get64BitAddressSupported());

    myDev.activate();
    joshlog("Post-activate...\n");
    joshlog("Codec bitmap = %u\n", myDev.getCodecBitMap());


    joshlog("TODO: PICK CODEC\n");
    codec_info cinfo{};
    cinfo.load(myDev, 0);
    cinfo.logReport();
    try_it_out(myDev, cinfo);

    for(int line=0;line<4;line++) {
        unsigned long *peeks = allpeeks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            peeks[chunk] = devMem->peek32(4*(line*8+chunk));
        }
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }
    myDev.force_reset();
    //devMem->peek8(4096); //Force a GPF

    unsigned short pciCommand2 = hdaFunction->getConfig16(0x4);
    joshdebug("HDA PCI COMMAND=%02hx\n", pciCommand2);
    if (pciCommand2 & 0x4) {
        joshlog("HDA Bus Mastering enabled...enabling!\n");
        hdaFunction->setConfig16(0x4, pciCommand2 & ~0x4);
        pciCommand2 = hdaFunction->getConfig16(0x4);
        joshlog("NEW HDA PCI COMMAND=%02hx\n", pciCommand2);
    }
}


extern "C" void trs_ich_setup() {


    joshlog("Woo C++ v8\n");
    joshlog("In ich setup\n");
    if (!test_for_pci()) {
        joshlog("pci bios not found\n");
        return;
    } else {
        joshdebug("PCI found: hw=%02X,maj=%u,min=%u,lb=%u\n",
                pci_hardware_mechanism, pci_ver_major, pci_ver_minor, pci_last_bus_no);
    }
    setup_hda();

}