//
// Created by arnold on 12/22/24.
//

#ifndef XTRS_DPMHW_PCI_H
#define XTRS_DPMHW_PCI_H

#include <cstdint>
#include "dpmhw.h"

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


namespace dpmhw {

    struct PciBusInfo {
        bool busIsPresent;
        uint8_t hardwareMechanism;
        uint8_t versionMajor;
        uint8_t versionMinor;
        uint8_t lastBusNumber;
    };

    PciBusInfo detectPci();



    class PciFunction {
    private:
        const unsigned short pci_address;
        int had_errors;
        static const unsigned int invalid_read_value = 0xFFFFFFFFu;
    public:
        explicit PciFunction(unsigned short pci_address) : pci_address(pci_address), had_errors(0) {}

        [[nodiscard]] bool hadErrors() const {
            return had_errors != 0;
        }

        [[nodiscard]] int errorDiagnosticBits() const {
            return had_errors;
        }

        int clearErrors() {
            int res = hadErrors();
            had_errors = 0;
            return res;
        }

        uint32_t getConfig32(uint8_t address);
        uint16_t getConfig16(uint8_t address);
        void setConfig16(uint8_t address, uint16_t value);

        static PciFunction invalid() { return PciFunction(-1); }

        /** Attempts to find the HD Audio PCI Function.   If mote than one exists, the first one is returned
         *
         * @return the function or an empty option if not found
         */
        static option<PciFunction> findHdaFunction();
    };


};


#endif //XTRS_DPMHW_PCI_H
