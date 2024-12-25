//
// Created by arnold on 12/22/24.
//
#include <go32.h>
#include <dpmi.h>
#include <cstring>

#include "dpmhw_pci.h"
#include "dpmhw_impl.h"

using namespace dpmhw;

/** Returns the usable remaining space in the djgpp transfer buffer.
 * (Not sure we ever need to transfer memory for pci bios calls, but
 * we can use the transfer buffer if needed.  The ds, and es segment
 * registers are set so offset 0 is the start of the usable buffer.
 *
 * If more space is needed, use djgpp/dpmi calls to allocate our own
 * dos memory buffer.
 *
 * This should be used with "__dpmi_simulate_real_mode_interrupt" because
 * we manage the real-mode stack.
 *
 * See the "PCI BIOS SPECIFICATION Revision 2.1" for the basics of using
 * BIOS to access PCI.  It's SLOW, but we don't need it to be fast, since
 * we only call it during initialization.
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

PciBusInfo dpmhw::detectPci() {
    volatile static int pci_init_flag = 0;
    volatile static int pci_present = 0;
    volatile static unsigned char pci_hardware_mechanism = 0;
    volatile static unsigned char pci_ver_major = 0;
    volatile static unsigned char pci_ver_minor = 0;
    volatile static unsigned char pci_last_bus_no = 0;


    __dpmi_regs regs;
    if (!pci_init_flag) {
        prepare_pci_bios(&regs, PCI_BIOS_PRESENT);

        dpmhw_debug("PCICHECK PRE %x,%x,%x,%x,%x,%x\n",
                regs.d.eax,regs.d.ebx, regs.d.ecx, regs.d.edx, regs.d.esi, regs.d.edi
        );

        //int rmi = __dpmi_int(PCI_BIOS_INT, &regs);
        int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
        dpmhw_debug("PCICHECK RMI=%d\n",rmi);
        dpmhw_debug("PCICHECK POST %x,%x,%x,%x,%x,%x\n",
                regs.d.eax,regs.d.ebx, regs.d.ecx, regs.d.edx, regs.d.esi, regs.d.edi
        );
        if (regs.h.ah == 0 && regs.d.edx == 0x20494350 && (regs.x.flags & 1) == 0) {
            pci_present = 1;
            pci_hardware_mechanism = regs.h.al;
            pci_ver_major = regs.h.bh;
            pci_ver_minor = regs.h.bl;
            pci_last_bus_no = regs.h.cl;
            dpmhw_log("PCI Present %x,%x,%x,%x\n", pci_hardware_mechanism, pci_ver_major, pci_ver_minor, pci_last_bus_no);
        } else {
            pci_present = 0;
            dpmhw_log("PCI Not Present %u,%x,%u\n", regs.h.ah, regs.d.edx, regs.x.flags);
        }
        pci_init_flag = 1;
    }
    return PciBusInfo{
        pci_present != 0, pci_hardware_mechanism, pci_ver_minor, pci_ver_minor, pci_last_bus_no
    };
}

static bool test_for_pci() {
    return detectPci().busIsPresent;
}

uint32_t dpmhw::PciFunction::getConfig32(uint8_t address) {
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
    return (uint32_t)regs.d.ecx;
}

uint16_t dpmhw::PciFunction::getConfig16(uint8_t address) {
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

void dpmhw::PciFunction::setConfig16(uint8_t address, uint16_t value) {
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
}

option<PciFunction> dpmhw::PciFunction::findHdaFunction() {
    if (!test_for_pci()) {
        return option<PciFunction>(false, PciFunction::invalid());
    }
    __dpmi_regs regs;
    prepare_pci_bios(&regs, FIND_PCI_CLASS_CODE);
    regs.d.ecx = 0x040300; //Class Code 04 -> Multimedia Device, 03 -> Audio Device, 00 -> HDA Audio
    regs.x.si = 0;
    int rmi = __dpmi_simulate_real_mode_interrupt(PCI_BIOS_INT, &regs);
    if (rmi != 0) {
        dpmhw_log("ERROR: PCIBIOS(FIND_PCI_CLASS_CODE) - failed to call BIOS\n");
        return option<PciFunction>(false, PciFunction::invalid());
    }
    if ((regs.x.flags & 1) == 1) {
        //Carry flag set if BIOS flagged an error
        dpmhw_log("ERROR: PCIBIOS(FIND_PCI_CLASS_CODE) - error was returned\n");
        return option<PciFunction>(false, PciFunction::invalid());
    }
    if (regs.h.ah != PCI_SUCCESSFUL) {
        dpmhw_debug("PCIBIOS(FIND_PCI_CLASS_CODE) - class not found %x\n", regs.h.ah);
        return option<PciFunction>(false, PciFunction::invalid());
    }
    dpmhw_log("Found candidate at %x\n", regs.x.bx);

    return option<PciFunction>(PciFunction (regs.x.bx));
}
