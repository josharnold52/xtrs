//
// Created by arnold on 3/1/25.
//

#include "dpmhw_dacemu.h"


using namespace dpmhw;

static dpmhw::PciFunction getHdaPciFunction() {
    PciBusInfo pci = detectPci();
    if (!pci.busIsPresent) {
        dpmhw_log("pci bios not found\n");
        return PciFunction::invalid();
    }

    option<dpmhw::PciFunction> hdaFunction = dpmhw::PciFunction::findHdaFunction();
    if (!hdaFunction.exists()) {
        dpmhw_log("No HDA found\n");
        return PciFunction::invalid();
    }
    return hdaFunction.get();
}

static dpmhw::SelectorMem getHdaRegs(PciFunction &f, PciFunctionBusMasterEnabler &busEnabler) {
    if (!f.isValid() || busEnabler.didFail()) {
        return SelectorMem::invalid();
    }
    auto deviceMemAddr = f.getConfig32(4 * 4);
    if (f.hadErrors()) {
        dpmhw_log("The HDA PCI function had errors\n");
        return SelectorMem::invalid();;
    }
    option<SelectorMem> devMem = SelectorMem::mapDevice(deviceMemAddr & 0xFFFFFFF0u, 4096 * 3);

    if (!devMem.exists() || devMem.get().isNull()) {
        dpmhw_log("Could not map HDA device memory\n");
        return SelectorMem::invalid();;
    }
    return devMem.get();
}

static HdaDevice createHdaDevice(PciFunction &f, SelectorMem &regs) {
    if (f.hadErrors() || regs.isNull()) {
        return {};
    }
    return {f, regs};
}

static rtsound::HdaRealTimeSound createRtSound(HdaDevice &dev) {
    if (!dev.isValid()) {
        dpmhw_log("Cannot create RTSound because HDA device could not be created\n");
        return {};
    }
    return {&dev, dev.getNumberOfInputStreamsSupported(), 1};
}

EmulatedDac::EmulatedDac()
: hdaPciFunction(getHdaPciFunction())
, busMasterEnabler(PciFunctionBusMasterEnabler(hdaPciFunction))
, deviceMemory(getHdaRegs(hdaPciFunction, busMasterEnabler))
, device(createHdaDevice(hdaPciFunction, deviceMemory))
, rtSound(createRtSound(device))
, codecInfo()
, valid(false)
{
    if (device.isValid() && rtSound.isValid()) {
        codecInfo.loadFrom(device, 0);
        valid = true;
        dpmhw_log("Emulated DAC construction succeeded\n");
    } else {
        dpmhw_log("Emulated DAC construction failed\n");
    }
}

