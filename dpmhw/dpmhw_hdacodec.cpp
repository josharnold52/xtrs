//
// Created by arnold on 12/25/24.
//

#include <cstring>
#include <cstdio>
#include "dpmhw_hdacodec.h"
#include "dpmhw_impl.h"


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

using dpmhw::hda::amp_capabilities;
using dpmhw::hda::widget_capabilities;
using dpmhw::hda::pin_capabilities;
using dpmhw::hda::volume_knob_capabilities;
using dpmhw::hda::supported_pcm_caps;
using dpmhw::hda::supported_stream_format_caps;
using dpmhw::hda::config_default;
using dpmhw::hda::widget_info;
using dpmhw::hda::audio_function_group_capabilities;
using dpmhw::hda::audio_function_group_info;
using dpmhw::hda::codec_info;

void amp_capabilities::log(const char *prefix) const {
    dpmhw_log("%sM=%d,SS=%u,NS=%u,O=%u\n",
              prefix,getMuteCapable() ? 1: 0, getStepSize(), getNumSteps(), getOffset());
}

void widget_capabilities::log(const char *prefix) const {
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

    dpmhw_log("%s%s,flags=%s,delay=%u,channels=%u\n",
              prefix, typeCode, flags, getDelay(),getChannelCount());

}


void pin_capabilities::log(const char *prefix) const {
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
    dpmhw_log("%seapd=%u,vref=%02x,flags=%s\n",
            prefix, eapd,vref, pinFlags);
}

void volume_knob_capabilities::log(const char *prefix) const {
    dpmhw_log("%sdelta=%u,numsteps=%u\n",
            prefix, isDelta() ? 1 : 0, getNumSteps());
}


void supported_pcm_caps::log(const char *prefix) const {
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
    dpmhw_log("%sdepths=%s,rates=%s\n",prefix, depth, rate);
}

void supported_stream_format_caps::log(const char *prefix) const {
    dpmhw_log("%spcm=%u,float32=%u,ac3=%u\n",
            prefix, canPcm() ? 1: 0, canFloat32() ? 1 : 0, canAc3() ? 1: 0);
}

void config_default::log(const char *prefix) const {
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
    dpmhw_log("%sLoc=%s-%s,Conn=%s-%s,Dev=%s,Col=%s,Jdo=%u,Assoc=%u/%u\n"
            ,prefix,grossLoc,geomLoc,portConn,connTyp,dfltDev,color
            ,jdoFlag,getDefaultAssociation(),getSequence());
};

void widget_info::loadFrom(dpmhw::HdaDevice &dev, unsigned int codecNo, unsigned int nodeNo) {
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

void widget_info::logReport() const {
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
        dpmhw_log("   CONN LIST:%s\n",connl);
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

void audio_function_group_capabilities::log(const char *prefix) const {
    dpmhw_log("%sbeep=%u,inputDelay=%u,outputDelay=%u\n",
            prefix, hasBeepGen() ? 1 : 0, getInputDelay(), getOutputDelay());
}

void audio_function_group_info::loadFrom(dpmhw::HdaDevice &dev, unsigned int codecNo, unsigned int node) {
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
            widgets[i].loadFrom(dev, codecNo, widgetNodeOffset + i);
        }
    }
}

void audio_function_group_info::logReport() const {
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
    dpmhw_log("PREFERRED_SPEAKER = %d\n", psp ? psp->nodeNumber : -1);
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
        dpmhw_log("PATH TO DAC: %s\n", lsto);

        unsigned char vks[32];
        unsigned int vklen = findAssociatedVolumeKnobs(path, len, vks, 32);
        lstolen = 0;
        for(unsigned int i=0;i<vklen;i++) {
            sprintf(lsto+lstolen, "%u ", vks[i] + 0);
            lstolen += strlen(lsto + lstolen);
        }
        lsto[lstolen] = 0;
        dpmhw_log("ASSOCIATED KNOBS: %s\n", lsto);
    }
}

static int best_speaker_comparator(const widget_info &w1, const widget_info &w2) {
    int f1 = w1.getWidgetType() == dpmhw::hda::WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;
    int f2 = w2.getWidgetType() == dpmhw::hda::WIDGET_TYPE_PIN_COMPLEX ? 0x8000 : 0;

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

const widget_info * audio_function_group_info::findSpeaker() const {
    if (!widgetCount)
        return nullptr;
    const widget_info * wptrs[max_audio_widgets];
    for(unsigned short i=0;i<widgetCount;i++) {
        wptrs[i] = widgets + i;
    }
    tqsort<widget_info, best_speaker_comparator>::run(wptrs, widgetCount);
    return wptrs[0];
}

unsigned int audio_function_group_info::findAssociatedVolumeKnobs(const unsigned char *toNodes, unsigned int toNodesLength,
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

typedef dpmhw::bitset<256> nodeset;

static unsigned int bfsToDac(const audio_function_group_info &afg, unsigned char *res, // NOLINT(*-no-recursion)
                      unsigned int curDepth, const unsigned int maxDepth,
                      const nodeset &next, nodeset &seen) {

    //Some of these checks are somewhat redundant since I check them again before calling myself
    //recursively.  But they do check the initial call, so I guess I will keep them
    if (curDepth > maxDepth)
        return 0;
    bool nextEmpty = true;
    for (unsigned int i = next.findNext(0); i != nodeset::not_found(); i = next.findNext(i+1)) {
        nextEmpty = false;
        const widget_info *node = afg.lookupNode(i);
        if (node && node->getWidgetType() == dpmhw::hda::WIDGET_TYPE_AUDIO_OUT) {
            res[curDepth] = i;
            return curDepth+1;
        }
    }
    if (nextEmpty || curDepth >= maxDepth)
        return 0;

    seen.addAll(next);
    nodeset n2;
    bool n2Empty = true;
    for (int unsigned i = next.findNext(0); i != nodeset::not_found(); i = next.findNext(i+1)) {
        const widget_info *node = afg.lookupNode(i);
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
    const unsigned int r = bfsToDac(afg, res, curDepth+1, maxDepth, n2, seen);
    if (!r)
        return 0;
    const unsigned char nextNode = res[curDepth+1];
    for (unsigned int i = next.findNext(0); i != nodeset::not_found(); i = next.findNext(i+1)) {
        const widget_info *node = afg.lookupNode(i);
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
    dpmhw::dpmhw_log("Dont think I should get here\n");
    return 0;
}

unsigned int audio_function_group_info::findPathToDac(unsigned char fromNode, unsigned char *res, unsigned int maxLen) const {
    if (!res || !maxLen)
        return 0;
    nodeset next, seen;
    next.add(fromNode);
    return bfsToDac(*this, res, 0, maxLen - 1, next, seen);
}

void codec_info::loadFrom(dpmhw::HdaDevice &dev, unsigned int codecNo) {
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
        gi.loadFrom(dev, codecNo, subStart + i);
        if (gi.functionGroupType == NODE_TYPE_AUDIO_FUNCTION_GROUP) {
            audioFunctionGroups[audioFunctionGroupCount++] = gi;
            if (audioFunctionGroupCount >= max_audio_function_groups) {
                break;
            }
        }
    }

}

void codec_info::logReport() const {
    dpmhw_log("CODEC %u\n", codecNumber);
    dpmhw_log("ROOT NODE   : venId=%04x,devId=%04x,revId=%08x,afgCount=%u\n",
            vendorId, deviceId, revisionId, audioFunctionGroupCount);
    for(unsigned long i = 0; i<audioFunctionGroupCount; i++) {
        audioFunctionGroups[i].logReport();
    }
}
