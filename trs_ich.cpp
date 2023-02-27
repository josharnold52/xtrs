
#include <go32.h>
#include <dpmi.h>
#include <cstring>
#include <cstdlib>
#include <sys/nearptr.h>
#include <sys/farptr.h>

extern "C" {
#include "z80.h"
}

#define PCI_BIOS_INT 0x1A
#define PCI_FUNCTION_ID 0xB1
#define PCI_BIOS_PRESENT 0x01
#define FIND_PCI_CLASS_CODE 0x03
#define PCI_READ_CONFIG_DWORD 0x0A

#define PCI_SUCCESSFUL 0
#define PCI_DEVICE_NOT_FOUND 0x86


#define INLINE_PAUSE  { __asm__ __volatile__ ("pause"); }


static int pci_init_flag = 0;
static int pci_present = 0;
static unsigned char pci_hardware_mechanism;
static unsigned char pci_ver_major;
static unsigned char pci_ver_minor;
static unsigned char pci_last_bus_no;

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


class SelectorMem {
public:
    const unsigned short selector;
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

class HdaDevice {
private:
    PciFunction pciFunction;
    SelectorMem regs;
    bool active = false;
    unsigned long dmaMemSize = 0x10000;
    unsigned long dmaMemUsed = 0x0;
    unsigned char *dmaMem = 0;




public:
    static const unsigned long GCAP = 0;
    static const unsigned long INTCTL = 0x20;
    static const unsigned long CORBCTL = 0x4C;
    static const unsigned long RIRBCTL = 0x5C;
    static const unsigned long GCTL = 0x08;
    HdaDevice(PciFunction &p, SelectorMem &r) :
            pciFunction(p), regs(r) {
        dmaMem = static_cast<unsigned char *>(malloc(dmaMemSize));
        if (_go32_dpmi_lock_data(dmaMem, dmaMemSize) != 0) {
            joshlog("Failed to lock dma memory\n");
            return;
        }
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

    void activate() {
        //TODO
        if (dmaMem) {
            active = true;
        } else {
            joshlog("Failed to activate because no mem\n");
        }
    }


    void reset() {
        if (!active) {
            return;
        }
        joshlog("Resetting the HDA...\n");
        regs.poke32(INTCTL, 0); //Disable all interrupts

        //TODO: Keep track of any streams the we started and shut them down
        joshlog("Stopping response dma...\n");
        regs.poke8(RIRBCTL, 0);
        while((regs.peek8(RIRBCTL) & 0x2) != 0) INLINE_PAUSE;
        joshlog("Stopping command dma...\n");
        regs.poke8(CORBCTL, 0);
        while((regs.peek8(CORBCTL) & 0x2) != 0) INLINE_PAUSE;
        joshlog("Resetting device...\n");
        regs.poke32(GCTL, 0);
        while((regs.peek8(CORBCTL) & 0x1) != 0) INLINE_PAUSE;

        active = false;
        joshlog("HDA has been reset\n");

    }

    unsigned short getGlobalCapabilities() { return regs.peek16(GCAP); }
    unsigned int getNumberOfOutputStreamsSupported() { return (getGlobalCapabilities() >> 12) & 0xF; }
    unsigned int getNumberOfInputStreamsSupported() { return (getGlobalCapabilities() >> 8) & 0xF; }
    unsigned int getNumberOfBidirectionalStreamsSupported() { return (getGlobalCapabilities() >> 3) & 0x1F; }
    unsigned int getNumberOfSerialDataOutSignals() { return (getGlobalCapabilities() >> 1) & 0x3; }
    unsigned int get64BitAddressSupported() { return getGlobalCapabilities() & 0x1; }
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
        joshlog("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                chunks[0],chunks[1],chunks[2],chunks[3],
                chunks[4],chunks[5],chunks[6],chunks[7]);
    }
    joshlog("CFG ERROR=%u\n", hdaFunction->errorDiagnosticBits());
    if (hdaFunction->hadErrors()) {
        return;
    }
    option<SelectorMem> devMem = SelectorMem::mapDevice(allchunks[4] & 0xFFFFFFF0u, 4096);
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
        joshlog("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }

    HdaDevice myDev(hdaFunction.get(), devMem.get());
    myDev.activate();
    myDev.reset();
    //devMem->peek8(4096); //Force a GPF

}


extern "C" void trs_ich_setup() {
    joshlog("Woo C++ v5\n");
    joshlog("In ich setup\n");
    if (!test_for_pci()) {
        joshlog("pci bios not found\n");
        return;
    } else {
        joshlog("PCI found: hw=%02X,maj=%u,min=%u,lb=%u\n",
                pci_hardware_mechanism, pci_ver_major, pci_ver_minor, pci_last_bus_no);
    }
    setup_hda();

}