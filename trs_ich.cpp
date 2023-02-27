
#include <go32.h>
#include <dpmi.h>
#include <string.h>
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


static int pci_init_flag = 0;
static int pci_present = 0;
static unsigned char pci_hardware_mechanism;
static unsigned char pci_ver_major;
static unsigned char pci_ver_minor;
static unsigned char pci_last_bus_no;

/**
 * Kind of like option in java/scala but we need a placeholder value
 * even in the invalid case
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

    static option<SelectorMem> mapDevice(unsigned long addr, unsigned long size) {
        option<SelectorMem> invalid(false, SelectorMem(0));
        if (size >= 0x100000) {
            joshlog("Segments > 1M not supported (because I have to be smarter about granularity bit");
            return invalid;
        }
        __dpmi_meminfo mi;
        mi.size=size;
        mi.address = addr;
        mi.handle = 0;
        if (__dpmi_physical_address_mapping(&mi)!=0) {
            joshlog("DPMI map of %x(%u) failed\n", addr,size);
            return invalid;
        }
        int sel = __dpmi_allocate_ldt_descriptors(1);
        if (sel  == -1) {
            joshlog("Unable to allocate descriptor\n");
            return invalid;
        }
        //Access rights - Data, RW, Ring 3, size in bytes
        if (__dpmi_set_segment_base_address(sel, addr) |
            __dpmi_set_segment_limit(sel, size - 1) |
            __dpmi_set_descriptor_access_rights(sel, 0x4F3) ) {
            joshlog("Unable to set descriptor params\n");
            return invalid;
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

    static option<PciFunction> find_hga() {
        option<PciFunction> invalid = option<PciFunction>(false, PciFunction(-1));
        if (!test_for_pci()) {
            return invalid;
        }
        __dpmi_regs regs;
        prepare_pci_bios(&regs, FIND_PCI_CLASS_CODE);
        regs.d.ecx = 0x040300; //TODO
        regs.x.si = 0;
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        if (rmi != 0) {
            joshlog("rmi FIND_PCI_CLASS_CODE failed\n");
            return invalid;
        }
        if ((regs.x.flags & 1) == 1) {
            joshlog("rmi FIND_PCI_CLASS_CODE error\n");
            return invalid;
        }
        if (regs.h.ah != PCI_SUCCESSFUL) {
            joshlog("rmi FIND_PCI_CLASS_CODE not found %x\n", regs.h.ah);
            return invalid;
        }
        joshlog("Found candidate at %x\n", regs.x.bx);

        return option<PciFunction>(PciFunction (regs.x.bx));
    }
};


static void setup_hda() {
    option<PciFunction> hdaFunction = PciFunction::find_hga();
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
    unsigned long peeks[32];
    for(int line=0;line<4;line++) {
        unsigned int *chunks = allchunks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            peeks[chunk] = devMem->peek32(4*(line*8+chunk));
        }
        joshlog("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                chunks[0],chunks[1],chunks[2],chunks[3],
                chunks[4],chunks[5],chunks[6],chunks[7]);
    }
    joshlog("Woo C++ v3\n");

}


extern "C" void trs_ich_setup() {
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