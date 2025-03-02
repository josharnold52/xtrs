//
// Created by arnold on 12/22/24.
//

#ifndef XTRS_DPMHW_PCI_H
#define XTRS_DPMHW_PCI_H

#include <cstdint>
#include "dpmhw.h"



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
        static const unsigned short invalid_pci_address = -1;

        [[nodiscard]] bool isValidPciAddress() const {
            return pci_address != invalid_pci_address;
        }
    public:
        explicit PciFunction(unsigned short pci_address) : pci_address(pci_address), had_errors(0) {}
        PciFunction() : pci_address(PciFunction::invalid_pci_address), had_errors(0) {}  //Default constructor is same as "invalid" below

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

        static PciFunction invalid() { return PciFunction(PciFunction::invalid_pci_address); }

        /** Attempts to find the HD Audio PCI Function.   If mote than one exists, the first one is returned
         *
         * @return the function or an empty option if not found
         */
        static option<PciFunction> findHdaFunction();
    };


};


#endif //XTRS_DPMHW_PCI_H
