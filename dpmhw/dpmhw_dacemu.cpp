//
// Created by arnold on 3/1/25.
//

#include "dpmhw.h"
#include "dpmhw_dacemu.h"
#include "dpmhw_pci.h"
#include "dpmhw_memory.h"
#include "dpmhw_hdadev.h"
#include "dpmhw_rtsound.h"
#include "dpmhw_hdacodec.h"


using namespace dpmhw;

/**
 * Note - will be using placement new for these doohickies
 *
 * Best way to clean up is to explicitly call destructor e,g, (&(d->device))->~HdaDevice();
 */


// Note:  The idea here is that we're going to allocate certain
struct init_data {
    option<dpmhw::PciFunction> hdaPciFunction;
    bool busMasteringNeededEnabling;
    option<SelectorMem> deviceMemory;

    HdaDevice device;
    bool deviceConstructed;

    hda::codec_info info;

    rtsound::HdaRealTimeSound rtSound;
    bool rtSoundConstructed;

public:
    init_data() = delete;
    ~init_data() = delete;
    init_data(init_data &) = delete;
    init_data(const init_data &) = delete;
};


void * init() {

    PciBusInfo pci = detectPci();
    if (!pci.busIsPresent) {
        dpmhw_log("pci bios not found\n");
        return nullptr;
    }

    option<dpmhw::PciFunction> hdaFunction = dpmhw::PciFunction::findHdaFunction();
    if (!hdaFunction.exists()) {
        dpmhw_log("No HDA found\n");
        return nullptr;
    }



    bool busMasteringNeededEnabling=false;
    unsigned short pciCommand = hdaFunction->getConfig16(0x4);
    if (!(pciCommand & 0x4)) {
        busMasteringNeededEnabling = true;
        dpmhw_log("HDA Bus Mastering not enabled...fixing! %02hx\n", pciCommand);
        hdaFunction->setConfig16(0x4, pciCommand | 0x4);
        pciCommand = hdaFunction->getConfig16(0x4);
        if (!(pciCommand & 0x4)) {
            dpmhw_log("Failed to enable bus mastering %02hx\n", pciCommand);
            return nullptr;
        }
    }

    auto deviceMem = hdaFunction->getConfig32(4*4);
    if (hdaFunction->hadErrors()) {
        dpmhw_log("pci hda errors");
        return nullptr;
    }
    option<SelectorMem> devMem = SelectorMem::mapDevice(deviceMem & 0xFFFFFFF0u, 4096 * 3);
    if (!devMem.exists()) {
        dpmhw_log("Could not map device memory");
        return nullptr;
    }

    HdaDevice myDev(hdaFunction.get(), devMem.get());
    myDev.activate();

    rtsound::HdaRealTimeSound rtSound(&myDev, myDev.getNumberOfInputStreamsSupported(), 1);




    return nullptr;

}