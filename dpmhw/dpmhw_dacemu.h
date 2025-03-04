//
// Created by arnold on 3/1/25.
//

#ifndef XTRS_DPMHW_DACEMU_H
#define XTRS_DPMHW_DACEMU_H

#include "dpmhw_pci.h"
#include "dpmhw_memory.h"
#include "dpmhw_hdadev.h"
#include "dpmhw_rtsound.h"
#include "dpmhw_hdacodec.h"


namespace dpmhw {


    //High level interface to an emulated audio DAC
    class EmulatedDac {
    private:
        dpmhw::PciFunction hdaPciFunction;
        dpmhw::PciFunctionBusMasterEnabler busMasterEnabler;
        SelectorMem deviceMemory;
        HdaDevice device;
        rtsound::HdaRealTimeSound rtSound;
        hda::codec_info codecInfo;
        bool valid;

    public:
        EmulatedDac();


    };


}



#endif //XTRS_DPMHW_DACEMU_H
