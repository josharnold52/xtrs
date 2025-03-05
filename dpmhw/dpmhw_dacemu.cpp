//
// Created by arnold on 3/1/25.
//

#include "dpmhw_dacemu.h"
#include "dpmhw_impl.h"

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
    if (!dev.isActive()) {
        dpmhw_log("Cannot create RTSound because HDA device could not be activated\n");
        return {};
    }
    return {&dev, dev.getNumberOfInputStreamsSupported(), 1};
}

static hda::codec_info loadCodecInfo(HdaDevice &dev) {
    if (!dev.isValid()) {
        return {};
    }
    hda::codec_info info{};
    info.loadFrom(dev, 0);
    return info;
}


EmulatedDac::EmulatedDac()
: hdaPciFunction(getHdaPciFunction())
, busMasterEnabler(PciFunctionBusMasterEnabler(hdaPciFunction))
, deviceMemory(getHdaRegs(hdaPciFunction, busMasterEnabler))
, device(createHdaDevice(hdaPciFunction, deviceMemory))
, hdaDeviceRunning(device.activate())
, rtSound(createRtSound(device))
, valid(device.isValid() && device.isActive() && rtSound.isValid() && hdaDeviceRunning)
{
    if (valid) {
        dpmhw_log("Emulated DAC construction succeeded\n");
    } else {
        dpmhw_log("Emulated DAC construction failed\n");
    }
}

EmulatedDac::~EmulatedDac() {
    dpmhw_log("Destroying EmulatedDac\n");
    if (running) {
        dpmhw_log("...Stopping EmulatedDac\n");
        stop();
    }
    if (active) {
        dpmhw_log("...Deactivating EmulatedDac\n");
        deactivate();
    }
    dpmhw_log("...Destroying components of EmulatedDac\n");
}

bool EmulatedDac::setupCodecs() {
    //TODO - this sets up codecs, but we may need to acitvate/reset the HDA too!
    if (!codecInfoLoaded) {
        codecInfo.loadFrom(device, 0);
        codecInfoLoaded = true;
    }

    dpmhw_log("Setting up codecs: streamDescriptorNumber=%d streamNumber=%d\n", rtSound.getDescriptorNumber(), rtSound.getStreamNumber());
    HdaDevice::Codec codecControl(device, codecInfo.codecNumber);
    const hda::audio_function_group_info &afg = codecInfo.audioFunctionGroups[0];

    //TODO: This is needed to make virtualbox work on the second run (Without this, the first run works
    // but not subsequent runs without hard-resetting the VM
    dpmhw_log("Root Reset %u\n", 0);
    codecControl.nodeVerb(0, 0x7FF, 0);


    dpmhw_debug("AFG Function RESET\n");
    codecControl.nodeVerb(afg.nodeNumber, 0x7ff, 0);

    // Add a slight delay here by executing a get command on the afg
    codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0);  //xf05 == get power state

    const hda::widget_info * speaker = afg.findSpeaker();
    if (!speaker) {
        dpmhw_log("Cannot find speaker\n");
        return false;
    }
    unsigned char chain[8];
    unsigned int chainLen = afg.findPathToDac(speaker->nodeNumber, chain, 8);
    if (!chainLen) {
        dpmhw_log("Cannot find DAC\n");
        return false;
    }
    unsigned char vkbuf[1];
    const hda::widget_info * volumeKnob;
    if (afg.findAssociatedVolumeKnobs(chain, chainLen, vkbuf, 1)) {
        volumeKnob = afg.lookupNode(vkbuf[0]);
    } else {
        volumeKnob = nullptr;
    }
    const hda::widget_info *dac = afg.lookupNode(chain[chainLen - 1]);

    dpmhw_log("Powering up hda widgets...\n");
    //TODO - Hack! power up FG and other stuff
    dpmhw_log("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    //TODO - At some point I had tried power-cycling by powering down the widget before powering up,
    //  but I suspect it is no longer necessary so I'm commenting that stuff out.   Once I confirm it
    //  works I can remove these lines (or restore them if they don't work)
    //dpmhw_log("Powering Down %u\n", afg.nodeNumber);
    //codecControl.nodeVerb(afg.nodeNumber, 0x705, 3);
    //usleep(100 * 1000);

    //TODO - I'm not sure that this powerup sequence works if the afg was in the coldest powerdown
    //  state because in that case the nodes won't respond to commands.  See the HDA PowerState info.
    //  Basically, I think I need to also tie this to a "link reset" of the HDA controller and/or
    //  be able to send a "double reset", which I'll need special support for since the first reset
    //  doesn't get a response
    dpmhw_log("Powering Up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);

    //Wait for the node to report power up.  Each command is sent in its own frame, and the response is
    //received in the subsequent frame.  Each frame takes about 20us (48,000 frames per sec), so we
    //expect approx 40 us per call to nodeVerb() (not including controller overhead).  So 25000 loops
    //should correspond to a wait of about 1 second, which should be enough time for the power up
    for(int i=0;i<25000;i++) {
        if ((codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0) & 0xF0) != 0) {
            break;
        }
        INLINE_PAUSE;
    }
    if ((codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0) & 0xF0) != 0) {
        dpmhw_log("Unable to power up codec\n");
        return false;
    }
    /*
    TODO - I think all of this was trying to get VirtualBox to properly initialize HDA after it had already
     been used - I think the real solution was sending a reset command to the codec root node above (which isn't
     really part of the HDA spec but it did the trick

    dpmhw_log("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    dpmhw_log("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    dpmhw_log("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    dpmhw_log("Powering up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);
    */
    dpmhw_log("Power state of AFG is %ul\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));

    for(unsigned int i = 0; i < chainLen; i++) {
        dpmhw_log("Powering up %u\n", i);
        codecControl.nodeVerb(chain[i], 0x705, 0);
        dpmhw_log("Power state of %d is %ul\n", (int)chain[i], codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    }
    if (volumeKnob) {
        dpmhw_log("Powering up %u\n", volumeKnob->nodeNumber);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x705, 0);
        dpmhw_log("Power state of knob is %ul\n", codecControl.nodeVerb(volumeKnob->nodeNumber, 0xf05, 0));
    }

    for(unsigned int i=0;i<chainLen;i++) {
        const hda::widget_info *cur = afg.lookupNode(chain[chainLen - i - 1]);
        dpmhw_log("Setting up widget %u\n", cur->nodeNumber);
        int prevNode = i > 0 ? chain[chainLen - i] : -1;
        int inputIndex = prevNode >= 0 ?cur->getOffsetOfInputConnection(prevNode) : -1;
        if (cur->widgetCaps.hasInputAmp()) {
            hda::amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                                    cur->inputAmpCaps : afg.inputAmpCaps;
            unsigned int payload = 0x7000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                                   (acap.getOffset() & 0x7F);
            dpmhw_log("Setting input amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            if (cur->getWidgetType() == hda::WIDGET_TYPE_AUDIO_MIXER) {
                for(int ii=0;ii<cur->numConns;ii++) {
                    if (ii != inputIndex) {
                        unsigned int pl = 0x7000 |
                                          ((ii & 0xF) << 8) |
                                          0x80;
                        dpmhw_log("muting unused input amp of node %u - PL=%04X\n", cur->nodeNumber, pl);
                        codecControl.nodeVerb(cur->nodeNumber, 0x3, pl);
                    }
                }
            }
        }

        if (cur->widgetCaps.hasOutputAmp()) {
            dpmhw_log("Tweaking output amp of node %u\n", cur->nodeNumber);
            dpmhw_log("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            dpmhw_log("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));

            hda::amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                                    cur->outputAmpCaps : afg.outputAmpCaps;
            unsigned int payloadReset = 0xB000 |
                                        ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                                        0;
            dpmhw_log("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payloadReset);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payloadReset);
            // I  used to crank down the volume here, but now I'm not
            unsigned int payload = 0xB000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                                   ((acap.getOffset() & 0x7F) );
            dpmhw_log("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            dpmhw_log("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            dpmhw_log("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));
        }
        if (cur->numConns > 1 && prevNode >= 0 && cur->getWidgetType() != hda::WIDGET_TYPE_AUDIO_MIXER) {
            unsigned int payload = cur->getOffsetOfInputConnection(prevNode) & 0xFF;
            dpmhw_log("Setting active connector of node %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x701,payload);
        }
        if (cur->getWidgetType() == hda::WIDGET_TYPE_PIN_COMPLEX) {
            dpmhw_log("Tweaking pin control of node %u\n", cur->nodeNumber);
            dpmhw_log("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
            unsigned int payload = 0x40;
            dpmhw_log("Set pin control of %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x707, payload);
            dpmhw_log("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
        }
        dpmhw_log("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));
        dpmhw_log("Setting EAPD of %u\n", cur->nodeNumber);
        codecControl.nodeVerb(cur->nodeNumber, 0x70C, 0x2);
        dpmhw_log("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));

    }
    if (volumeKnob) {
        unsigned int steps = volumeKnob->volumeKnobCaps.getNumSteps();
        unsigned int payload = 0x80 | (steps & 0x7F);
        dpmhw_log("Setting volume knob %u to %04X\n", volumeKnob->nodeNumber, payload);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x70F, payload);

    }

    codecControl.nodeVerb(dac->nodeNumber, 0x2, rtSound.getStreamFormat()); //Set format
    codecControl.nodeVerb(dac->nodeNumber, 0x706, (rtSound.getStreamNumber() << 4) + 0);
    //TODO - Set power states
    // TODO - EAPD/BTL ?
    codecControl.nodeVerb(dac->nodeNumber, 0x72D, 1);  // 2 channels

    return true;
}
