
#include <go32.h>
#include <dpmi.h>
#include <cstring>
#include <cstdlib>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <unistd.h>
#include <cstdarg>
#include <cmath>

extern "C" {
#include "z80.h"
}
#include "dpmhw/dpmhw_pci.h"
#include "dpmhw/dpmhw_memory.h"
#include "dpmhw/dpmhw_hdadev.h"
#include "dpmhw/dpmhw_hdastream.h"
#include "dpmhw/dpmhw_hdacodec.h"
#include "dpmhw/dpmhw_rtsound.h"

/*
 * TODO:  The MASTER LIST
 *
 * I've been super inconsistent with numeric types, but a major issue has
 * been with HDA node numbers.   The spec mentions 7-bit "short from" or 15-bit
 * "long form" but the 15-bit version depend on some yet-to-be specified indirect
 * addressing scheme.  Probably I should just forget about 15-bit support and go
 * with 7-bit for now.   I've been using unsigned types for node nums, but it's tempting
 * to use signed types because they fit and because negative numbers are convenient
 * invalid sentinel values.
 *
 * I also need to fix connection list parsing - it currently reads the values as
 * 8 bit or 16 bit (depending om short/long form).  But the high bit is actually
 * a flag indicating whether the value is standalone or the top end of a range.
 *
*/



using dpmhw::option;
using dpmhw::SelectorMem;
using dpmhw::DmaRegion;
using dpmhw::HdaDevice;
using dpmhw::HdaOutputStream;
using dpmhw::rtsound::HdaRealTimeSound;

using namespace dpmhw::hda;

static void try_it_out(HdaDevice &dev, const codec_info &codec) {

    //HdaOutputStream myStream(&dev, 4096, 2, dev.getNumberOfInputStreamsSupported(), 1);
    HdaRealTimeSound rtSound(&dev, dev.getNumberOfInputStreamsSupported(), 1);

    if (!rtSound.isValid()) {
        joshlog("ERROR: rtsound allocation failed");
        return;
    }

    joshlog("streamDescriptorNumber=%d streamNumber=%d\n", rtSound.getDescriptorNumber(), rtSound.getStreamNumber());
    HdaDevice::Codec codecControl(dev, codec.codecNumber);
    const audio_function_group_info &afg = codec.audioFunctionGroups[0];

    //TODO: This is needed to make virtualbox work on the second run (Without this, the first run works
    // but not subsequent runs without hard-resetting the VM
    joshlog("Root Reset %u\n", 0);
    codecControl.nodeVerb(0, 0x7FF, 0);


    dpmhw::dpmhw_debug("AFG Function RESET\n");
    codecControl.nodeVerb(afg.nodeNumber, 0x7ff, 0);

    const widget_info * speaker = afg.findSpeaker();
    if (!speaker) {
        joshlog("Cannot find speaker\n");
        return;
    }
    unsigned char chain[8];
    unsigned int chainLen = afg.findPathToDac(speaker->nodeNumber, chain, 8);
    if (!chainLen) {
        joshlog("Cannot find DAC\n");
        return;
    }
    unsigned char vkbuf[1];
    const widget_info * volumeKnob;
    if (afg.findAssociatedVolumeKnobs(chain, chainLen, vkbuf, 1)) {
        volumeKnob = afg.lookupNode(vkbuf[0]);
    } else {
        volumeKnob = nullptr;
    }
    const widget_info *dac = afg.lookupNode(chain[chainLen - 1]);

    joshlog("Powering up...");
    //TODO - Hack! power up FG and other stuff
    joshlog("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    joshlog("Powering Down %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 3);
    usleep(100 * 1000);
    joshlog("Powering Up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);
    usleep(100 * 1000);
    joshlog("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    joshlog("Resetting %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x7FF, 0);
    joshlog("Power state of AFG is 0x%x\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    joshlog("Powering up %u\n", afg.nodeNumber);
    codecControl.nodeVerb(afg.nodeNumber, 0x705, 0);
    joshlog("Power state of AFG is %ul\n", codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));

    for(unsigned int i = 0; i < chainLen; i++) {
        joshlog("Powering up %u\n", i);
        codecControl.nodeVerb(chain[i], 0x705, 0);
        joshlog("Power state of %d is %ul\n", (int)chain[i], codecControl.nodeVerb(afg.nodeNumber, 0xf05, 0));
    }
    if (volumeKnob) {
        joshlog("Powering up %u\n", volumeKnob->nodeNumber);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x705, 0);
        joshlog("Power state of knob is %ul\n", codecControl.nodeVerb(volumeKnob->nodeNumber, 0xf05, 0));
    }

    for(unsigned int i=0;i<chainLen;i++) {
        const widget_info *cur = afg.lookupNode(chain[chainLen - i - 1]);
        joshlog("Setting up widget %u\n", cur->nodeNumber);
        int prevNode = i > 0 ? chain[chainLen - i] : -1;
        int inputIndex = prevNode >= 0 ?cur->getOffsetOfInputConnection(prevNode) : -1;
        if (cur->widgetCaps.hasInputAmp()) {
            amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                    cur->inputAmpCaps : afg.inputAmpCaps;
            unsigned int payload = 0x7000 |
                ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                (acap.getOffset() & 0x7F);
            joshlog("Setting input amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            if (cur->getWidgetType() == WIDGET_TYPE_AUDIO_MIXER) {
                for(int ii=0;ii<cur->numConns;ii++) {
                    if (ii != inputIndex) {
                        unsigned int pl = 0x7000 |
                                               ((ii & 0xF) << 8) |
                                               0x80;
                        joshlog("muting unused input amp of node %u - PL=%04X\n", cur->nodeNumber, pl);
                        codecControl.nodeVerb(cur->nodeNumber, 0x3, pl);
                    }
                }
            }
        }

        if (cur->widgetCaps.hasOutputAmp()) {
            joshlog("Tweaking output amp of node %u\n", cur->nodeNumber);
            joshlog("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            joshlog("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));

            amp_capabilities acap = cur->widgetCaps.hasAmpOverride() ?
                                    cur->outputAmpCaps : afg.outputAmpCaps;
            unsigned int payloadReset = 0xB000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                                   0;
            joshlog("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payloadReset);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payloadReset);
            // I  used to crank down the volume here, but now I'm not
            unsigned int payload = 0xB000 |
                                   ((inputIndex >= 0 ? (inputIndex & 0xF) : 0) << 8) |
                    ((acap.getOffset() & 0x7F) );
            joshlog("Setting output amp of node %u - PL=%04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x3, payload);

            joshlog("OutAmp 0x8000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0x8000));
            joshlog("OutAmp 0xC000 value is 0x%x\n", codecControl.nodeVerb(cur->nodeNumber, 0xB, 0xC000));

        }
        if (cur->numConns > 1 && prevNode >= 0 && cur->getWidgetType() != WIDGET_TYPE_AUDIO_MIXER) {
            unsigned int payload = cur->getOffsetOfInputConnection(prevNode) & 0xFF;
            joshlog("Setting active connector of node %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x701,payload);
        }
        if (cur->getWidgetType() == WIDGET_TYPE_PIN_COMPLEX) {
            joshlog("Tweaking pin control of node %u\n", cur->nodeNumber);
            joshlog("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
            unsigned int payload = 0x40;
            joshlog("Set pin control of %u to %04X\n", cur->nodeNumber, payload);
            codecControl.nodeVerb(cur->nodeNumber, 0x707, payload);
            joshlog("Pin control of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF07, 0));
        }
        joshlog("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));
        joshlog("Setting EAPD of %u\n", cur->nodeNumber);
        codecControl.nodeVerb(cur->nodeNumber, 0x70C, 0x2);
        joshlog("EAPD of %u is 0x%x\n", cur->nodeNumber, codecControl.nodeVerb(cur->nodeNumber, 0xF0C, 0));

    }
    if (volumeKnob) {
        unsigned int steps = volumeKnob->volumeKnobCaps.getNumSteps();
        unsigned int payload = 0x80 | (steps & 0x7F);
        joshlog("Setting volume knob %u to %04X\n", volumeKnob->nodeNumber, payload);
        codecControl.nodeVerb(volumeKnob->nodeNumber, 0x70F, payload);

    }

    codecControl.nodeVerb(dac->nodeNumber, 0x2, rtSound.getStreamFormat()); //Set format
    codecControl.nodeVerb(dac->nodeNumber, 0x706, (rtSound.getStreamNumber() << 4) + 0);
    //TODO - Set power states
    // TODO - EAPD/BTL ?

    // TODO - Stripe Control ??
    codecControl.nodeVerb(dac->nodeNumber, 0x72D, 1);  // 2 channels
    dev.dumpRegs();
    dev.dumpVendorRegs();
    dev.dumpExtendedRegs();
    dev.dumpDmaBuf();

    const double secPerWall = 1.0 / 24e6;
    joshlog("Timing rdtsc\n");
    const auto calib_run = (int32_t) (1 * 24e6);
    int32_t calib_start = dev.getWallClockCount();
    int64_t rdt_start = dpmhw::dpmhw_rdtsc();
    while( (dev.getWallClockCount() - calib_start) < calib_run) {
        dpmhw::dpmhw_x86pause();
    }
    int64_t rdt_end = dpmhw::dpmhw_rdtsc();
    int64_t rdt_diff = rdt_end - rdt_start;
    joshlog("Elapsed after 10 %llu\n", rdt_diff);
    joshlog("Elapsed after 10 %lu\n", (int32_t)(rdt_diff));

    //24959999078
    const double tscFactor = 10.0 / 24959999078.0;

    //myStream.dumpBufferDescriptorList();
    double afreq = 300 * 2  * PI;
    double bfreq = 4 * 2  * PI;

    rtSound.start();
    rtSound.resetBuffer(0x8000);
    //Note - our calculations will overflow if we time longer than 89 seconds
    const auto started = dpmhw::dpmhw_rdtsc();
    double elapsed = 0;
    int32_t counter = 0;
    int32_t writesCounter = 0;
    while(elapsed < 30) {
        double tdiff = (double)(dpmhw::dpmhw_rdtsc() - started);
        elapsed = tdiff * tscFactor;
        auto x = (uint16_t )lround(0x8000 + 0x4000 * sin(afreq * (elapsed + 0.02 * sin(bfreq * (elapsed + 0.1 * elapsed * elapsed)))));
        //auto x = (tdiff & 512) ? 0x9999 : 0x7777;
        auto sc = rtSound.soundOut(x);
        counter += sc;
        writesCounter++;
    }
    joshlog("Sent %u samples (%u)\n", counter, writesCounter);
    rtSound.stop();
    //myStream.stop();
    //usleep(3000000);


    //codecControl.nodeVerb(dac->nodeNumber, )

}
/*
class CodecResponse {
public:
    const unsigned long value;

    CodecResponse(unsigned long v) : value(v) {}

};

class NodeContoller {
private:
    HdaDevice::Codec &codec;
    unsigned int nodeNum;
public:
    NodeContoller(HdaDevice::Codec &codec, unsigned int nodeNum)
    : codec(codec)
    , nodeNum(nodeNum) {}


    unsigned long rawVerb(unsigned int verb, unsigned int params) {
        return codec.nodeVerb(nodeNum, verb, params);
    }

    unsigned long getParameter(unsigned char paramId) {
        return rawVerb(0xF00, paramId);
    }

    class ResponseVendorId : CodecResponse {
    public:
        unsigned short vendorId()  { return 0xFFFF & (value >> 16); }
        unsigned short deviceId()  { return 0xFFFF & value; }
        ResponseVendorId(unsigned long v) : CodecResponse(v) {}
    };

    ResponseVendorId getVendorId() {
        unsigned long r = getParameter(0);
        return {r};
    }


    unsigned char getConnectionSelect() {
        return rawVerb(0xF01, 0) & 0xFF;
    }

    void setConnectionSelect(unsigned char value) {
        rawVerb(0x701, value);
    }

};

*/
static void setup_hda() {
    option<dpmhw::PciFunction> hdaFunction = dpmhw::PciFunction::findHdaFunction();
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
        dpmhw::dpmhw_debug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                chunks[0],chunks[1],chunks[2],chunks[3],
                chunks[4],chunks[5],chunks[6],chunks[7]);
    }
    dpmhw::dpmhw_debug("CFG ERROR State=%u\n", hdaFunction->errorDiagnosticBits());
    if (hdaFunction->hadErrors()) {
        return;
    }
    unsigned short pciCommand = hdaFunction->getConfig16(0x4);
    dpmhw::dpmhw_debug("HDA PCI COMMAND=%02hx\n", pciCommand);
    if (!(pciCommand & 0x4)) {
        joshlog("HDA Bus Mastering not enabled...fixing!\n");
        hdaFunction->setConfig16(0x4, pciCommand | 0x4);
        pciCommand = hdaFunction->getConfig16(0x4);
        joshlog("NEW HDA PCI COMMAND=%02hx\n", pciCommand);
    }

    option<SelectorMem> devMem = SelectorMem::mapDevice(allchunks[4] & 0xFFFFFFF0u, 4096 * 3);
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
        dpmhw::dpmhw_debug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }

    HdaDevice myDev(hdaFunction.get(), devMem.get());
    joshlog("GCAP: os=%d,is=%d,bs=%d,sdo=%d,a64=%d\n", myDev.getNumberOfOutputStreamsSupported(),
           myDev.getNumberOfInputStreamsSupported(),
           myDev.getNumberOfBidirectionalStreamsSupported(),
           myDev.getNumberOfSerialDataOutSignals(),
           myDev.get64BitAddressSupported());

    myDev.activate();
    joshlog("Post-activate...\n");
    joshlog("Codec bitmap = %u\n", myDev.getCodecBitMap());


    joshlog("TODO: PICK CODEC\n");
    codec_info cinfo{};
    cinfo.loadFrom(myDev, 0);
    cinfo.logReport();
    try_it_out(myDev, cinfo);

    for(int line=0;line<4;line++) {
        unsigned long *peeks = allpeeks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            peeks[chunk] = devMem->peek32(4*(line*8+chunk));
        }
        dpmhw::dpmhw_debug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }
    myDev.force_reset();
    //devMem->peek8(4096); //Force a GPF

    unsigned short pciCommand2 = hdaFunction->getConfig16(0x4);
    dpmhw::dpmhw_debug("HDA PCI COMMAND=%02hx\n", pciCommand2);
    if (pciCommand2 & 0x4) {
        joshlog("HDA Bus Mastering enabled...enabling!\n");
        hdaFunction->setConfig16(0x4, pciCommand2 & ~0x4);
        pciCommand2 = hdaFunction->getConfig16(0x4);
        joshlog("NEW HDA PCI COMMAND=%02hx\n", pciCommand2);
    }
}


extern "C" void trs_ich_setup() {

    joshlog("Woo C++ v8\n");
    joshlog("In ich setup\n");
    dpmhw::PciBusInfo pci = dpmhw::detectPci();
    if (!pci.busIsPresent) {
        joshlog("pci bios not found\n");
        return;
    } else {
        dpmhw::dpmhw_debug("PCI found: hw=%02X,maj=%u,min=%u,lb=%u\n",
                pci.hardwareMechanism, pci.versionMajor, pci.versionMinor, pci.lastBusNumber);
    }
    setup_hda();

}