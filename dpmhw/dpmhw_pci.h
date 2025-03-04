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

        [[nodiscard]] bool isValid() const {
            return pci_address != PciFunction::invalid_pci_address;
        }
        [[nodiscard]] bool hadErrors() const {
            return had_errors != 0;
        }
        [[nodiscard]] unsigned short getPciAddress() const {
            return pci_address;
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

    class PciFunctionBusMasterEnabler {
    private:
        PciFunction pciFunction;
        bool enabledBusMaster;
        bool failed;

    public:
        PciFunctionBusMasterEnabler() : pciFunction(PciFunction::invalid()), enabledBusMaster(false), failed(false) {}
        explicit PciFunctionBusMasterEnabler(PciFunction function) : pciFunction(function), enabledBusMaster(false), failed(false) {
            if (function.isValid()) {
                unsigned short pciCommand = function.getConfig16(0x4);
                if (!(pciCommand & 0x4)) {
                    dpmhw_log("Bus Mastering for %hu not enabled...fixing! %02hx\n", function.getPciAddress(), pciCommand);
                    function.setConfig16(0x4, pciCommand | 0x4);
                    pciCommand = function.getConfig16(0x4);
                    if (!(pciCommand & 0x4)) {
                        dpmhw_log("Failed to enable bus mastering %02hx\n", pciCommand);
                        failed = true;
                        return;
                    }
                    enabledBusMaster = true;
                } else {
                    dpmhw_log("Bus Mastering for %hu is already enabled. %02hx\n", function.getPciAddress(), pciCommand);
                }
            }
        }
        ~PciFunctionBusMasterEnabler() {
            if (enabledBusMaster) {
                unsigned short pciCommand = pciFunction.getConfig16(0x4);
                if (pciCommand & 0x4) {
                    dpmhw_log("Disabling Bus Mastering for %hu ... %02hx\n", pciFunction.getPciAddress(), pciCommand);
                    pciFunction.setConfig16(0x4, pciCommand & ~0x4);
                    pciCommand = pciFunction.getConfig16(0x4);
                    if (pciCommand & 0x4) {
                        dpmhw_log("Failed to disable bus mastering %02hx\n", pciCommand);
                        return;
                    }
                } else {
                    dpmhw_log("HDA Bus Mastering for %hu is already disabled. %02hx\n", pciFunction.getPciAddress(), pciCommand);
                }
            }
        }
        [[nodiscard]] bool didFail() const {
            return failed;
        }

    };

};


#endif //XTRS_DPMHW_PCI_H
