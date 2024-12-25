
#include <go32.h>
#include <dpmi.h>
#include <cstring>
#include <cstdlib>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <unistd.h>
#include <cstdarg>

extern "C" {
#include "z80.h"
}
#include "dpmhw/dpmhw_pci.h"
#include "dpmhw/dpmhw_memory.h"
#include "dpmhw/dpmhw_hdadev.h"
#include "dpmhw/dpmhw_hdastream.h"


#define INLINE_PAUSE  { __asm__ __volatile__ ("pause"); }


static const int NODE_PARAM_DEVICE_ID = 0x0;
static const int NODE_PARAM_REVISION_ID = 0x2;
static const int NODE_PARAM_SUBORDINATE_NODES = 0x4;
static const int NODE_PARAM_FUNCTION_GROUP_TYPE = 0x5;
static const int NODE_PARAM_AUDIO_FUNCTION_GROUP_CAPABILITIES = 0x8;
static const int NODE_PARAM_AUDIO_WIDGET_CAPABILITIES = 0x9;
static const int NODE_PARAM_SUPPORTED_PMC_RATES = 0xA;
static const int NODE_PARAM_SUPPORTED_STREAM_FORMATS = 0xB;
static const int NODE_PARAM_PIN_CAPABILITIES = 0xC;
static const int NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES = 0xD;
static const int NODE_PARAM_CONNECTION_LIST_LENGTH = 0xE;
static const int NODE_PARAM_SUPPORTED_POWER_STATES = 0xF;
static const int NODE_PARAM_PROCESSING_CAPABILITIES = 0x10;
static const int NODE_PARAM_GPIO_COUNT = 0x11;
static const int NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES = 0x12;
static const int NODE_PARAM_VOLUME_KNOB_CAPABILITIES = 0x13;





static const int NODE_TYPE_AUDIO_FUNCTION_GROUP = 1;

static const int WIDGET_TYPE_AUDIO_OUT = 0x0;
static const int WIDGET_TYPE_AUDIO_IN = 0x1;
static const int WIDGET_TYPE_AUDIO_MIXER = 0x2;
static const int WIDGET_TYPE_AUDIO_SELECTOR = 0x3;
static const int WIDGET_TYPE_PIN_COMPLEX = 0x4;
static const int WIDGET_TYPE_POWER = 0x5;
static const int WIDGET_TYPE_VOLUME_KNOB = 0x6;
static const int WIDGET_TYPE_BEEP = 0x7;


static const unsigned int DMA_MEM_ALIGN = 0x1000;


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

/*
 * OTHER HW NOTES:
 * - If I want to play with the GPIO (I2C) PINS on the VGA of the Asus - Look at
 *   the GMA500 driver in the linux tree.  The GPIO regs are at 0x5010, 0x5014, etc...
 *   and the intel_i2c.c file shows how to use them.  I believe these registers are
 *   relative to the base of the MMIO given by the PCI function for the display.  This
 *   is pretty easy to confirm by tracing back the get_clock, get_data, etc. functions
 *   in the i2c file.  I can possibly figure out which GPIO is used by playing with the
 *   I2C stuff in linux.  That should get to the point where I can do it from DOS probably
 */

static void joshdebug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
//    joshlogv(msg, args);
    va_end(args);
}


template<class T, int (*cf)(const T &t1, const T &t2)> class tqsort {
private:
    static int vcf(const void *e1, const void *e2) {
        const T *pt1 = *(static_cast<const T * const *>(e1));
        const T *pt2 = *(static_cast<const T * const *>(e2));
        if (!(pt1 && pt2)) {
            return pt1 ? -1 : (pt2 ? 1 : 0);
        }
        return cf(*pt1, *pt2);
    }
public:
    tqsort() = delete;
    static void run(const T **base, size_t numelem) {
        qsort(base, numelem, sizeof(T*), vcf);
    }
};

using dpmhw::option;

class nodeset {
private:
    unsigned int bits[8]{};
public:
    void add(unsigned char x) {
        bits[(x>>5) & 7] |= 1u << (x & 31);
    }
    void addAll(const nodeset &rhs) {
        for(int i=0;i<8;i++) {
            bits[i] |= rhs.bits[i];
        }
    }
    bool contains(unsigned char x) const {
        return (bits[(x>>5) & 7] & (1u << (x & 31))) != 0;
    }
    int findNext(int startAt) const {
        if (startAt < 0)
            startAt = 0;
        //Can optimize this...
        for(int i=startAt; i < 256; i++) {
            if (contains(i)) {
                return i;
            }
        }
        return -1;
    }
};

using dpmhw::SelectorMem;
using dpmhw::DmaRegion;
using dpmhw::HdaDevice;
using dpmhw::HdaOutputStream;


static void lookupLogCode(unsigned int singleCodeLength, const char *allCodes, unsigned int value, char *result ) {
    result[0] = 0;
    if (singleCodeLength == 0 || !allCodes) {
        return;
    }
    unsigned int numCodes = strlen(allCodes) / singleCodeLength;
    if (numCodes == 0) {
        return;
    }
    if (value >= numCodes) {
        return;
    }
    memcpy(result, allCodes + (singleCodeLength * value), singleCodeLength);
    result[singleCodeLength] = 0;
}




static const int CON_LIST_BUF_LEN = 16;

struct amp_capabilities {
    unsigned long caps;

    unsigned int getStepSize() const { return (caps >> 16) & 0x7F ; }
    unsigned int getNumSteps() const { return (caps >> 8) & 0x7F ; }
    unsigned int getOffset() const { return (caps) & 0x7F ; }
    bool getMuteCapable() const { return (caps & 0x80000000u) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        joshlog("%sM=%d,SS=%u,NS=%u,O=%u\n",
                prefix,getMuteCapable() ? 1: 0, getStepSize(), getNumSteps(), getOffset());
    }
};

struct widget_capabilities {
    unsigned long caps;
    unsigned int getWidgetType() const { return (caps >> 20) & 0xF; }
    unsigned int getDelay() const { return (caps >> 16) & 0xF; }
    unsigned int getChannelCount() const { return 1 + (((caps >> 12) & 0xE) | (caps & 0x1)); }
    bool hasCpCaps() const { return (caps & 0x1000) != 0; }
    bool hasLrSwap() const { return (caps & 0x800) != 0; }
    bool hasPowerControl() const { return (caps & 0x400) != 0; }
    bool isDigital() const { return (caps & 0x200) != 0; }
    bool hasConnectionList() const { return (caps & 0x100) != 0; }
    bool isUnsolCapable() const { return (caps & 0x80) != 0; }
    bool isProcWidget() const { return (caps & 0x40) != 0; }
    bool isStripeSupported() const { return (caps & 0x20) != 0; }
    bool hasFormatOverride() const { return (caps & 0x10) != 0; }
    bool hasAmpOverride() const { return (caps & 0x8) != 0; }
    bool hasOutputAmp() const { return (caps & 0x4) != 0; }
    bool hasInputAmp() const { return (caps & 0x2) != 0; }
    bool isStereo() const { return (caps & 0x1) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        unsigned long widType = (caps >> 20) & 0xF;
        static const char flagNames[] = "_X*DLUP%FAOIS";
        static const char typeNames[] = "AOAIAMASPCPWVKBG08090A0B0C0D0E0F";
        char flags[sizeof(flagNames)];
        memcpy(flags, flagNames, sizeof(flagNames));
        const char flagCount = sizeof(flagNames) - 1;

        for (int fi = 0; fi < flagCount; fi++) {
            if (!((caps >> (flagCount - 1 - fi)) & 0x1)) {
                flags[fi] = ' ';
            }
        }
        char typeCode[3];
        typeCode[0] = typeNames[widType * 2];
        typeCode[1] = typeNames[widType * 2 + 1];
        typeCode[2] = 0;

        joshlog("%s%s,flags=%s,delay=%u,channels=%u\n",
                prefix, typeCode, flags, getDelay(),getChannelCount());

    }
};

struct pin_capabilities {
    unsigned long caps;

    bool canHighBitRate() const { return (caps & 0x8000000) != 0; }
    bool canDisplayPort() const { return (caps & 0x1000000) != 0; }
    bool canEapd() const { return (caps & 0x10000) != 0; }
    unsigned char getVrefControlBits() const { return (caps >> 8) & 0xFF; }
    bool canHdmi() const { return (caps & 0x80) != 0; }
    bool hasBalancedPins() const { return (caps & 0x40) != 0; }
    bool isInputCapable() const { return (caps & 0x20) != 0; }
    bool isOutputCapable() const { return (caps & 0x10) != 0; }
    bool canHeadphoneDrive() const { return (caps & 0x8) != 0; }
    bool canPresenceDetect() const { return (caps & 0x4) != 0; }
    bool isTriggerRequiredForImpedanceSense() const { return (caps & 0x2) != 0; }
    bool isImpedanceSenseCapable() const { return (caps & 0x1) != 0; }
    bool isPresent() const { return caps != 0; }

    void log(const char *prefix) const {
        unsigned long eapd = canEapd() ? 1 : 0;
        unsigned long vref = getVrefControlBits() & 0xFF;
        static const char pinFlagNames[] = "DBIOHPTZ";
        char pinFlags[sizeof(pinFlagNames)];
        memcpy(pinFlags, pinFlagNames, sizeof(pinFlagNames));
        const char pinFlagCount = sizeof(pinFlagNames) - 1;
        for(int fi=0;fi<pinFlagCount;fi++) {
            if (!((caps >> (pinFlagCount - 1 - fi)) & 0x1)) {
                pinFlags[fi] = ' ';
            }
        }
        joshlog("%seapd=%u,vref=%02x,flags=%s\n",
                prefix, eapd,vref, pinFlags);
    }
};

struct volume_knob_capabilities {
    unsigned long caps;

    bool isPresent() const { return caps != 0; }

    bool isDelta() const { return (caps & 0x80) != 0; }

    unsigned int getNumSteps() const { return caps & 0x7f; }

    void log(const char *prefix) const {
        joshlog("%sdelta=%u,numsteps=%u\n",
                prefix, isDelta() ? 1 : 0, getNumSteps());
    }
};

struct supported_pcm_caps {
    unsigned long caps;
    bool isPresent() const { return caps != 0; }
    unsigned int getDepthBits() const { return ( caps >> 16) & 0x1F; }
    unsigned int getRateBits() const { return caps & 0xFFF; }

    void log(const char *prefix) const {
        char depth[100];
        int dlen = 0;
        static const char depthNames[] = " 816202432";
        for(int i = 0; i < 5; i++) {
            if ((getDepthBits() >> i) & 0x1) {
                if (dlen)
                    depth[dlen++] = ',';
                depth[dlen++] = depthNames[i*2];
                depth[dlen++] = depthNames[i*2+1];
            }
        }
        depth[dlen] = 0;
        char rate[200];
        int rlen = 0;
        static const char rateNames[] = "  8 11 16 22 32 44 48 88 96176192384";
        for(int i = 0; i < 12; i++) {
            if ((getRateBits() >> i) & 0x1) {
                if (rlen)
                    rate[rlen++] = ',';
                rate[rlen++] = rateNames[i*3];
                rate[rlen++] = rateNames[i*3+1];
                rate[rlen++] = rateNames[i*3+2];
            }
        }
        rate[rlen] = 0;
        joshlog("%sdepths=%s,rates=%s\n",prefix, depth, rate);
    }
};

struct supported_stream_format_caps {
    unsigned long caps;
    bool isPresent() const { return caps != 0; }
    bool canAc3() const { return (caps & 0x4) != 0; }
    bool canFloat32() const { return (caps & 0x2) != 0; }
    bool canPcm() const { return (caps & 0x1) != 0; }
    void log(const char *prefix) const {
        joshlog("%spcm=%u,float32=%u,ac3=%u\n",
                prefix, canPcm() ? 1: 0, canFloat32() ? 1 : 0, canAc3() ? 1: 0);
    }
};

struct config_default {
    unsigned long dflt;

    unsigned int getPortConnectivityBits() const { return (dflt >> 30) & 0x3; }
    unsigned int getGrossLocationBits() const { return (dflt >> 28) & 0x3; }
    unsigned int getGeometricLocationBits() const { return (dflt >> 24) & 0xf; }
    unsigned int getDefaultDeviceBits() const { return (dflt >> 20) & 0xf; }
    unsigned int getConnectionTypeBits() const { return (dflt >> 16) & 0xf; }
    unsigned int getColorBits() const { return (dflt >> 12) & 0xf; }
    bool getJackDetectOverrideToIncapable() const { return (dflt & 0x100) != 0; }
    unsigned int getDefaultAssociation() const { return (dflt >> 4) & 0xf; }
    unsigned int getSequence() const { return dflt & 0xf; }
    bool isPresent() const { return dflt != 0; }

    void log(const char *prefix) const {
        char portConn[3];
        lookupLogCode(2, "JANOFIBO",
                      getPortConnectivityBits(), portConn);
        char grossLoc[3];
        lookupLogCode(2, "EXINSEOT",
                      getGrossLocationBits(), grossLoc);
        char geomLoc[3];
        lookupLogCode(2, "NAREFRLERITOBO0708090A0B0C0D0E0F",
                      getGeometricLocationBits(), geomLoc);
        char dfltDev[3];
        lookupLogCode(2, "LOSPHPCDSPDOMLMHLIAUMCTESPDO0EOT",
                      getDefaultDeviceBits(), dfltDev);
        char connTyp[3];
        lookupLogCode(2, "UN/8/4ATRCOPODOADIXLRJCO0C0D0EOT",
                      getConnectionTypeBits(), connTyp);
        char color[3];
        lookupLogCode(2, "UNBKGRBUGRREORYEPUPI0A0B0C0DWHOT",
                      getColorBits(), color);
        unsigned int jdoFlag = getJackDetectOverrideToIncapable() ? 1 : 0;
        joshlog("%sLoc=%s-%s,Conn=%s-%s,Dev=%s,Col=%s,Jdo=%u,Assoc=%u/%u\n"
                  ,prefix,grossLoc,geomLoc,portConn,connTyp,dfltDev,color
                  ,jdoFlag,getDefaultAssociation(),getSequence());
    };
};

struct widget_info {
    unsigned short nodeNumber;
    widget_capabilities widgetCaps;
    pin_capabilities pinCaps;
    amp_capabilities inputAmpCaps;
    amp_capabilities outputAmpCaps;
    unsigned long connectionListCaps;
    volume_knob_capabilities volumeKnobCaps;
    unsigned short numConns;
    unsigned short connList[CON_LIST_BUF_LEN];
    config_default configDefault;
    supported_pcm_caps pcmCaps;
    supported_stream_format_caps streamFormatCaps;

    void load(HdaDevice &dev, unsigned int codecNo, unsigned int nodeNo) {
        HdaDevice::Codec codec(dev, codecNo);
        nodeNumber = nodeNo;
        widgetCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_AUDIO_WIDGET_CAPABILITIES);
        pinCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_PIN_CAPABILITIES);
        inputAmpCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES);
        outputAmpCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES);
        //Workaround for Virtualbox emulator bug - output amp caps and input amp caps are switched
        //Check if widget indicates it has amplifier overrides
        if (widgetCaps.hasAmpOverride()) {
            bool outa = widgetCaps.hasOutputAmp();
            bool ina = widgetCaps.hasInputAmp();
            if (
                    outa != ina
                    && inputAmpCaps.isPresent() != outputAmpCaps.isPresent()
                    && ina != inputAmpCaps.isPresent()) {
                unsigned  long x= inputAmpCaps.caps;
                inputAmpCaps.caps = outputAmpCaps.caps;
                outputAmpCaps.caps = x;
            }
        }

        connectionListCaps = codec.getNodeParam(nodeNo, NODE_PARAM_CONNECTION_LIST_LENGTH);
        volumeKnobCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_VOLUME_KNOB_CAPABILITIES);
        streamFormatCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_SUPPORTED_STREAM_FORMATS);
        pcmCaps.caps = codec.getNodeParam(nodeNo, NODE_PARAM_SUPPORTED_PMC_RATES);

        //Workaround for Virtualbox
        if (widgetCaps.getWidgetType() == WIDGET_TYPE_VOLUME_KNOB) {
            //VirtualBox seems to have misread the volumeKnobCapabilities param number
            //of 13H as a decimal number and put the configuration in 13 decimal, which
            //is input amp capabilities.
            if (!volumeKnobCaps.isPresent() && inputAmpCaps.isPresent()) {
                volumeKnobCaps.caps = inputAmpCaps.caps;
                inputAmpCaps.caps = 0;
            }
        }

        numConns = connectionListCaps & 0x7F;
        numConns = numConns > CON_LIST_BUF_LEN ? CON_LIST_BUF_LEN : numConns;
        memset(&connList, 0, sizeof(connList));
        for(int ci=0;ci < numConns;) {
            //TODO - This is wrong - need to check for range values and handle
            // them correctly
            unsigned long r= codec.nodeVerb(nodeNo, 0xf02, ci);
            unsigned short mask = connectionListCaps & 0x80 ? 0xFFFF : 0xFF;
            unsigned short shift = connectionListCaps & 0x80 ? 16 : 8;
            int cnt = connectionListCaps & 0x80 ? 2 : 4;
            for(int cj=0; cj < cnt && ci < numConns; cj++, ci++) {
                connList[ci] = r & mask;
                r = r >> shift;
            }
        }
        configDefault.dflt = codec.nodeVerb(nodeNo, 0xf1c, 0);

        //NOTE: Linux seems to set the defaults itself.  Here is what virtualBox uses:
        //https://github.com/torvalds/linux/blob/8ca09d5fa3549d142c2080a72a4c70ce389163cd/sound/pci/hda/patch_sigmatel.c#L3287

    }

    bool hasInputFrom(unsigned short fromNode) const {
        for(unsigned int i=0; i<numConns;i++) {
            if (connList[i] == fromNode)
                return true;
        }
    }

    int getOffsetOfInputConnection(unsigned short fromNode) const {
        for(int i=0; i<numConns;i++) {
            if (connList[i] == fromNode)
                return i;
        }
        return -1;
    }

    unsigned int getWidgetType() const {
        return widgetCaps.getWidgetType();
    }
    void logReport() const {
        {
            char p[20];
            sprintf(p," WIDGET %-4u: ", nodeNumber);
            widgetCaps.log(p);
        }
        if (pinCaps.isPresent()) {
            pinCaps.log("      PINCAP: ");
        }
        if (inputAmpCaps.isPresent()) {
            inputAmpCaps.log("      IN AMP: ");
        }
        if (outputAmpCaps.isPresent()) {
            outputAmpCaps.log("     OUT AMP: ");
        }
        if (numConns > 0) {
            char connl[CON_LIST_BUF_LEN * 6 + 1];  //5 chars for num, 1 for space.
            connl[0] = 0;
            for(unsigned int i = 0; i < numConns; i++) {
                char e[8];
                sprintf(e, " %d", connList[i] & 0xFFFF); //Safety mask
                strcat(connl, e);
            }
            joshlog("   CONN LIST:%s\n",connl);
        }
        if (configDefault.isPresent()) {
            configDefault.log("      CONFIG: ");
        }
        if (volumeKnobCaps.isPresent()) {
            volumeKnobCaps.log("       VKNOB: ");
        }
        if (streamFormatCaps.isPresent()) {
            streamFormatCaps.log("         FMT: ");
        }
        if (pcmCaps.isPresent()) {
            pcmCaps.log("         PCM: ");
        }
    }
};



struct audio_function_group_capabilities {
    unsigned long caps;

    bool isPresent() const { return caps != 0; }
    bool hasBeepGen() const { return (caps & 0x10000) != 0; }
    unsigned int getInputDelay() const { return (caps >> 8) & 0xF; }
    unsigned int getOutputDelay() const { return caps & 0xF; }

    void log(const char *prefix) const {
        joshlog("%sbeep=%u,inputDelay=%u,outputDelay=%u\n",
                prefix, hasBeepGen() ? 1 : 0, getInputDelay(), getOutputDelay());
    }

};

static const unsigned int max_audio_widgets = 256;

struct audio_function_group_info {
    unsigned int nodeNumber;
    unsigned char functionGroupType;
    bool canProduceUnsolicitedMessages;

    audio_function_group_capabilities fgCaps;
    amp_capabilities inputAmpCaps;
    amp_capabilities outputAmpCaps;
    supported_pcm_caps pcmCaps;
    supported_stream_format_caps streamFormatCaps;

    unsigned short widgetCount;
    widget_info widgets[max_audio_widgets];
    unsigned short widgetNodeOffset;

    const widget_info * lookupNode (unsigned char nodeNum) const {
        if (nodeNum < widgetNodeOffset)
            return nullptr;
        unsigned short i = nodeNum - widgetNodeOffset;
        if (i >= widgetCount)
            return nullptr;
        return widgets + i;
    }

    void load(HdaDevice &dev, unsigned int codecNo, unsigned int node) {
        HdaDevice::Codec codec(dev, codecNo);
        memset(this, 0, sizeof(*this));
        this->nodeNumber = node;
        unsigned long fgTypeRes = codec.getNodeParam(node, NODE_PARAM_FUNCTION_GROUP_TYPE);
        this->canProduceUnsolicitedMessages = (fgTypeRes & 0x100) != 0;
        this->functionGroupType = fgTypeRes & 0xFF;


        if (this->functionGroupType == NODE_TYPE_AUDIO_FUNCTION_GROUP) {
            fgCaps.caps = codec.getNodeParam(node, NODE_PARAM_AUDIO_FUNCTION_GROUP_CAPABILITIES);
            inputAmpCaps.caps = codec.getNodeParam(node, NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES);
            outputAmpCaps.caps = codec.getNodeParam(node, NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES);
            streamFormatCaps.caps = codec.getNodeParam(node, NODE_PARAM_SUPPORTED_STREAM_FORMATS);
            pcmCaps.caps = codec.getNodeParam(node, NODE_PARAM_SUPPORTED_PMC_RATES);

            unsigned long subordinates = codec.getNodeParam(node, NODE_PARAM_SUBORDINATE_NODES);
            widgetNodeOffset = (subordinates >> 16) & 0xFF;
            widgetCount = subordinates & 0xFF;
            for (unsigned short i = 0; i < widgetCount; i++) {
                widgets[i].load(dev, codecNo, widgetNodeOffset + i);
            }
        }
    }

    void logReport() const {
        char prefix[20];
        sprintf(prefix, "FG NODE %-4u: ", nodeNumber);
        fgCaps.log(prefix);
        if (inputAmpCaps.isPresent()) {
            inputAmpCaps.log("      IN AMP: ");
        }
        if (outputAmpCaps.isPresent()) {
            outputAmpCaps.log("     OUT AMP: ");
        }
        if (streamFormatCaps.isPresent()) {
            streamFormatCaps.log("         FMT: ");
        }
        if (pcmCaps.isPresent()) {
            pcmCaps.log("         PCM: ");
        }
        for(unsigned char i=0;i<widgetCount;i++) {
            widgets[i].logReport();
        }

        const widget_info *psp = findSpeaker();
        joshlog("PREFERRED_SPEAKER = %d\n", psp ? psp->nodeNumber : -1);
        if (psp) {
            unsigned char path[32];
            unsigned int len = findPathToDac(psp->nodeNumber, path, 32);
            char lsto[32*8];
            unsigned int lstolen = 0;
            for(unsigned int i=0;i<len;i++) {
                sprintf(lsto+lstolen, "%u ", path[i] + 0);
                lstolen += strlen(lsto + lstolen);
            }
            lsto[lstolen] = 0;
            joshlog("PATH TO DAC: %s\n", lsto);

            unsigned char vks[32];
            unsigned int vklen = findAssociatedVolumeKnobs(path, len, vks, 32);
            lstolen = 0;
            for(unsigned int i=0;i<vklen;i++) {
                sprintf(lsto+lstolen, "%u ", vks[i] + 0);
                lstolen += strlen(lsto + lstolen);
            }
            lsto[lstolen] = 0;
            joshlog("ASSOCIATED KNOBS: %s\n", lsto);

        }


    }

    const widget_info * findSpeaker() const {
        if (!widgetCount)
            return nullptr;
        const widget_info * wptrs[max_audio_widgets];
        for(unsigned short i=0;i<widgetCount;i++) {
            wptrs[i] = widgets + i;
        }
        tqsort<widget_info, best_speaker_comparator>::run(wptrs, widgetCount);
        return wptrs[0];
    }

    unsigned int findPathToDac(unsigned char fromNode, unsigned char *res, unsigned int maxLen) const {
        if (!res || !maxLen)
            return 0;
        nodeset next, seen;
        next.add(fromNode);
        return bfsToDac(res, 0, maxLen - 1, next, seen);
    }

    unsigned int findAssociatedVolumeKnobs(const unsigned char *toNodes, unsigned int toNodesLength,
                                           unsigned char *output, unsigned int outputMaxLen) const {
        if (!output || !outputMaxLen || !toNodes || !toNodesLength)
            return 0;
        unsigned int len = 0;
        for(unsigned int i = 0; i < widgetCount; i++) {
            if (widgets[i].getWidgetType() != WIDGET_TYPE_VOLUME_KNOB)
                continue;
            for(unsigned int j = 0; j < widgets[i].numConns; j++) {
                unsigned short chk = widgets[i].connList[j];
                bool fnd = false;
                for(unsigned int k = 0; k < toNodesLength && !fnd; k++) {
                    fnd |= (toNodes[k] == chk);
                }
                if (fnd) {
                    output[len++] = widgets[i].nodeNumber;
                    if (len >= outputMaxLen)
                        return len;
                }
            }
        }
        return len;
    }


private:
    unsigned int bfsToDac(unsigned char *res, unsigned int curDepth, const unsigned int maxDepth,
                         const nodeset &next, nodeset &seen) const {
        //Some of these checks are somewhat redundant since I check them again before calling myself
        //recursively.  But they do check the initial call, so i guess Ill keep them
        if (curDepth > maxDepth)
            return 0;
        bool nextEmpty = true;
        for (int i = next.findNext(-1); i >= 0; i = next.findNext(i+1)) {
            nextEmpty = false;
            const widget_info *node = lookupNode(i);
            if (node && node->getWidgetType() == WIDGET_TYPE_AUDIO_OUT) {
                res[curDepth] = i;
                return curDepth+1;
            }
        }
        if (nextEmpty || curDepth >= maxDepth)
            return 0;

        seen.addAll(next);
        nodeset n2;
        bool n2Empty = true;
        for (int i = next.findNext(-1); i >= 0; i = next.findNext(i+1)) {
            const widget_info *node = lookupNode(i);
            if (!node)
                continue;
            for(int j=0; j<node->numConns;j++) {
                unsigned short x = node->connList[j];
                if (x < 256 && !seen.contains(x)) {
                    n2.add(x);
                    n2Empty = false;
                }
            }
        }
        if (n2Empty)
            return 0;
        const unsigned int r = bfsToDac(res, curDepth+1, maxDepth, n2, seen);
        if (!r)
            return 0;
        const unsigned char nextNode = res[curDepth+1];
        for (int i = next.findNext(0); i >= 0; i = next.findNext(i+1)) {
            const widget_info *node = lookupNode(i);
            if (!node)
                continue;
            for(int j=0; j<node->numConns;j++) {
                unsigned short x = node->connList[j];
                if (x == nextNode) {
                    res[curDepth] = i;
                    return r;
                }
            }
        }
        joshlog("Dont think I should get here");
        return 0;
    }

    static int best_speaker_comparator(const widget_info &w1, const widget_info &w2) {
        int f1 = w1.getWidgetType() == WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;
        int f2 = w2.getWidgetType() == WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;

        f1 |= w1.configDefault.isPresent() ? 0x4000 : 0;
        f2 |= w2.configDefault.isPresent() ? 0x4000 : 0;

        f1 |= w1.configDefault.getDefaultDeviceBits() == 1 ? 0x2000 : 0;
        f2 |= w2.configDefault.getDefaultDeviceBits() == 1 ? 0x2000 : 0;

        f1 |= w1.numConns == 0 ? 0x1000 : 0;
        f2 |= w2.numConns == 0 ? 0x1000 : 0;

        f1 |= w1.configDefault.getPortConnectivityBits() == 2 ? 0x800 : 0;
        f2 |= w2.configDefault.getPortConnectivityBits() == 2 ? 0x800 : 0;

        f1 |= w1.configDefault.getPortConnectivityBits() == 3 ? 0x400 : 0;
        f2 |= w2.configDefault.getPortConnectivityBits() == 3 ? 0x400 : 0;
        //Prefer higher score from above checks
        if (f1 != f2) {
            return f1 > f2 ? -1 : 1;
        }
        //Prefer smaller association
        if (w1.configDefault.getDefaultAssociation() != w2.configDefault.getDefaultAssociation()) {
            return w1.configDefault.getDefaultAssociation() < w2.configDefault.getDefaultAssociation() ? -1 : 1;
        }
        //Prefer smaller sequence
        if (w1.configDefault.getSequence() != w2.configDefault.getSequence()) {
            return w1.configDefault.getSequence() < w2.configDefault.getSequence() ? -1 : 1;
        }
        //If all else fails, prefer smaller node number
        if (w1.nodeNumber  != w2.nodeNumber) {
            return w1.nodeNumber < w2.nodeNumber ? -1 : 1;
        }
        return 0;
    }

};


static const unsigned int max_audio_function_groups = 4;

/**
 * Note: This is a big object (around 77kb) mostly because we used
 * fixed size arrays to avoid the need for destructors
 */
struct codec_info {
    unsigned int codecNumber;
    unsigned short vendorId;
    unsigned short deviceId;
    unsigned long revisionId;
    unsigned short audioFunctionGroupCount;
    audio_function_group_info audioFunctionGroups[max_audio_function_groups];

    void load(HdaDevice &dev, unsigned int codecNo) {
        HdaDevice::Codec codec(dev, codecNo);
        this->codecNumber = codecNo;
        unsigned long venDevId = codec.getNodeParam(0, NODE_PARAM_DEVICE_ID);
        vendorId = (venDevId >> 16) & 0xFFFF;
        deviceId = venDevId & 0xFFFF;
        revisionId = codec.getNodeParam(0, NODE_PARAM_REVISION_ID);
        unsigned long subordinates = codec.getNodeParam(0, NODE_PARAM_SUBORDINATE_NODES);
        unsigned long subStart = (subordinates >> 16) & 0xFF;
        unsigned long subCount = subordinates & 0xFF;

        audioFunctionGroupCount = 0;
        memset(audioFunctionGroups, 0, sizeof(audioFunctionGroups));
        for(unsigned long i = 0; i<subCount; i++) {
            audio_function_group_info gi{};
            gi.load(dev, codecNo, subStart + i);
            if (gi.functionGroupType == NODE_TYPE_AUDIO_FUNCTION_GROUP) {
                audioFunctionGroups[audioFunctionGroupCount++] = gi;
                if (audioFunctionGroupCount >= max_audio_function_groups) {
                    break;
                }
            }
        }

    }
    void logReport() const {
        joshlog("CODEC %u\n", codecNumber);
        joshlog("ROOT NODE   : venId=%04x,devId=%04x,revId=%08x,afgCount=%u\n",
                vendorId, deviceId, revisionId, audioFunctionGroupCount);
        for(unsigned long i = 0; i<audioFunctionGroupCount; i++) {
            audioFunctionGroups[i].logReport();
        }

    }
};


static void try_it_out(HdaDevice &dev, const codec_info &codec) {

    HdaOutputStream myStream(&dev, 4096, 2, dev.getNumberOfInputStreamsSupported(), 1);
    if (!myStream.allocationSucceeded) {
        joshlog("ERROR: myStream allocation failed");
        return;
    }
    myStream.fillBufferWithTestTone(0x100);

    joshlog("streamDescriptorNumber=%d streamNumber=%d\n", myStream.getDescriptorNumber(), myStream.getStreamNumber());
    HdaDevice::Codec codecControl(dev, codec.codecNumber);
    const audio_function_group_info &afg = codec.audioFunctionGroups[0];

    //TODO: This is needed to make virtualbox work on the second run (Without this, the first run works
    // but not subsequent runs without hard-resetting the VM
    joshlog("Root Reset %u\n", 0);
    codecControl.nodeVerb(0, 0x7FF, 0);


    joshdebug("AFG Function RESET\n");
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

    codecControl.nodeVerb(dac->nodeNumber, 0x2, myStream.getFormat()); //Set format
    codecControl.nodeVerb(dac->nodeNumber, 0x706, (myStream.getStreamNumber() << 4) + 0);
    //TODO - Set power states
    // TODO - EAPD/BTL ?

    // TODO - Stripe Control ??
    codecControl.nodeVerb(dac->nodeNumber, 0x72D, 1);  // 2 channels
    dev.dumpRegs();
    dev.dumpVendorRegs();
    dev.dumpExtendedRegs();
    dev.dumpDmaBuf();
    myStream.dumpBufferDescriptorList();
    myStream.run();
    for(int i=0; i<60; i++) {
        joshlog("%x %x %x\n", myStream.getDmaPos(), myStream.getLinkPos(), myStream.getFifoSize());
        //dev.dumpRegs();
        //dev.dumpVendorRegs();
        //dev.dumpExtendedRegs();
        //dev.dumpDmaBuf();
        //myStream.dumpBufferDescriptorList();
        usleep(100000);
        if (i==20) {
            myStream.fillBufferWithTestTone(0x80);
        } else if (i == 40) {
            myStream.fillBufferWithTestTone(0x200);
        }
    }
    myStream.stop();
    //usleep(3000000);


    //codecControl.nodeVerb(dac->nodeNumber, )

}

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
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                chunks[0],chunks[1],chunks[2],chunks[3],
                chunks[4],chunks[5],chunks[6],chunks[7]);
    }
    joshdebug("CFG ERROR State=%u\n", hdaFunction->errorDiagnosticBits());
    if (hdaFunction->hadErrors()) {
        return;
    }
    unsigned short pciCommand = hdaFunction->getConfig16(0x4);
    joshdebug("HDA PCI COMMAND=%02hx\n", pciCommand);
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
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
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
    cinfo.load(myDev, 0);
    cinfo.logReport();
    try_it_out(myDev, cinfo);

    for(int line=0;line<4;line++) {
        unsigned long *peeks = allpeeks + 8*line;
        for(int chunk=0; chunk < 8; chunk++) {
            peeks[chunk] = devMem->peek32(4*(line*8+chunk));
        }
        joshdebug("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                peeks[0],peeks[1],peeks[2],peeks[3],
                peeks[4],peeks[5],peeks[6],peeks[7]);
    }
    myDev.force_reset();
    //devMem->peek8(4096); //Force a GPF

    unsigned short pciCommand2 = hdaFunction->getConfig16(0x4);
    joshdebug("HDA PCI COMMAND=%02hx\n", pciCommand2);
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
        joshdebug("PCI found: hw=%02X,maj=%u,min=%u,lb=%u\n",
                pci.hardwareMechanism, pci.versionMajor, pci.versionMinor, pci.lastBusNumber);
    }
    setup_hda();

}