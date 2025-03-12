#ifndef XTRS_DPMHW_HDACODEC_H
#define XTRS_DPMHW_HDACODEC_H

#include "dpmhw.h"
#include "dpmhw_hdadev.h"

namespace dpmhw::hda {

    const int NODE_PARAM_DEVICE_ID = 0x0;
    const int NODE_PARAM_REVISION_ID = 0x2;
    const int NODE_PARAM_SUBORDINATE_NODES = 0x4;
    const int NODE_PARAM_FUNCTION_GROUP_TYPE = 0x5;
    const int NODE_PARAM_AUDIO_FUNCTION_GROUP_CAPABILITIES = 0x8;
    const int NODE_PARAM_AUDIO_WIDGET_CAPABILITIES = 0x9;
    const int NODE_PARAM_SUPPORTED_PMC_RATES = 0xA;
    const int NODE_PARAM_SUPPORTED_STREAM_FORMATS = 0xB;
    const int NODE_PARAM_PIN_CAPABILITIES = 0xC;
    const int NODE_PARAM_INPUT_AMPLIFIER_CAPABILITIES = 0xD;
    const int NODE_PARAM_CONNECTION_LIST_LENGTH = 0xE;
    const int NODE_PARAM_SUPPORTED_POWER_STATES = 0xF;
    const int NODE_PARAM_PROCESSING_CAPABILITIES = 0x10;
    const int NODE_PARAM_GPIO_COUNT = 0x11;
    const int NODE_PARAM_OUTPUT_AMPLIFIER_CAPABILITIES = 0x12;
    const int NODE_PARAM_VOLUME_KNOB_CAPABILITIES = 0x13;





    const int NODE_TYPE_AUDIO_FUNCTION_GROUP = 1;

    const int WIDGET_TYPE_AUDIO_OUT = 0x0;
    const int WIDGET_TYPE_AUDIO_IN = 0x1;
    const int WIDGET_TYPE_AUDIO_MIXER = 0x2;
    const int WIDGET_TYPE_AUDIO_SELECTOR = 0x3;
    const int WIDGET_TYPE_PIN_COMPLEX = 0x4;
    const int WIDGET_TYPE_POWER = 0x5;
    const int WIDGET_TYPE_VOLUME_KNOB = 0x6;
    const int WIDGET_TYPE_BEEP = 0x7;

    struct amp_capabilities {
        unsigned long caps;

        [[nodiscard]] unsigned int getStepSize() const { return (caps >> 16) & 0x7F ; }
        /** Value returned is from 1 to 128 */
        [[nodiscard]] unsigned int getNumSteps() const { return 1 + ((caps >> 8) & 0x7F) ; }
        [[nodiscard]] unsigned int getOffset() const { return (caps) & 0x7F ; }
        [[nodiscard]] bool getMuteCapable() const { return (caps & 0x80000000u) != 0; }
        [[nodiscard]] bool isPresent() const { return caps != 0; }

        void log(const char *prefix) const;
    };

    struct widget_capabilities {
        unsigned long caps;
        [[nodiscard]] unsigned int getWidgetType() const { return (caps >> 20) & 0xF; }
        [[nodiscard]] unsigned int getDelay() const { return (caps >> 16) & 0xF; }
        [[nodiscard]] unsigned int getChannelCount() const { return 1 + (((caps >> 12) & 0xE) | (caps & 0x1)); }
        [[nodiscard]] bool hasCpCaps() const { return (caps & 0x1000) != 0; }
        [[nodiscard]] bool hasLrSwap() const { return (caps & 0x800) != 0; }
        [[nodiscard]] bool hasPowerControl() const { return (caps & 0x400) != 0; }
        [[nodiscard]] bool isDigital() const { return (caps & 0x200) != 0; }
        [[nodiscard]] bool hasConnectionList() const { return (caps & 0x100) != 0; }
        [[nodiscard]] bool isUnsolCapable() const { return (caps & 0x80) != 0; }
        [[nodiscard]] bool isProcWidget() const { return (caps & 0x40) != 0; }
        [[nodiscard]] bool isStripeSupported() const { return (caps & 0x20) != 0; }
        [[nodiscard]] bool hasFormatOverride() const { return (caps & 0x10) != 0; }
        [[nodiscard]]  bool hasAmpOverride() const { return (caps & 0x8) != 0; }
        [[nodiscard]] bool hasOutputAmp() const { return (caps & 0x4) != 0; }
        [[nodiscard]] bool hasInputAmp() const { return (caps & 0x2) != 0; }
        [[nodiscard]] bool isStereo() const { return (caps & 0x1) != 0; }
        [[nodiscard]] bool isPresent() const { return caps != 0; }

        void log(const char *prefix) const;
    };

    struct pin_capabilities {
        unsigned long caps;

        [[nodiscard]] bool canHighBitRate() const { return (caps & 0x8000000) != 0; }
        [[nodiscard]] bool canDisplayPort() const { return (caps & 0x1000000) != 0; }
        [[nodiscard]] bool canEapd() const { return (caps & 0x10000) != 0; }
        [[nodiscard]] unsigned char getVrefControlBits() const { return (caps >> 8) & 0xFF; }
        [[nodiscard]] bool canHdmi() const { return (caps & 0x80) != 0; }
        [[nodiscard]] bool hasBalancedPins() const { return (caps & 0x40) != 0; }
        [[nodiscard]] bool isInputCapable() const { return (caps & 0x20) != 0; }
        [[nodiscard]] bool isOutputCapable() const { return (caps & 0x10) != 0; }
        [[nodiscard]] bool canHeadphoneDrive() const { return (caps & 0x8) != 0; }
        [[nodiscard]] bool canPresenceDetect() const { return (caps & 0x4) != 0; }
        [[nodiscard]] bool isTriggerRequiredForImpedanceSense() const { return (caps & 0x2) != 0; }
        [[nodiscard]] bool isImpedanceSenseCapable() const { return (caps & 0x1) != 0; }
        [[nodiscard]] bool isPresent() const { return caps != 0; }

        void log(const char *prefix) const;
    };

    struct volume_knob_capabilities {
        unsigned long caps;

        [[nodiscard]] bool isPresent() const { return caps != 0; }
        [[nodiscard]] bool isDelta() const { return (caps & 0x80) != 0; }
        [[nodiscard]] unsigned int getNumSteps() const { return caps & 0x7f; }

        void log(const char *prefix) const;
    };

    struct supported_pcm_caps {
        unsigned long caps;
        [[nodiscard]] bool isPresent() const { return caps != 0; }
        [[nodiscard]] unsigned int getDepthBits() const { return ( caps >> 16) & 0x1F; }
        [[nodiscard]] unsigned int getRateBits() const { return caps & 0xFFF; }

        void log(const char *prefix) const;
    };

    struct supported_stream_format_caps {
        unsigned long caps;
        [[nodiscard]] bool isPresent() const { return caps != 0; }
        [[nodiscard]] bool canAc3() const { return (caps & 0x4) != 0; }
        [[nodiscard]] bool canFloat32() const { return (caps & 0x2) != 0; }
        [[nodiscard]] bool canPcm() const { return (caps & 0x1) != 0; }
        
        void log(const char *prefix) const;
    };

    struct config_default {
        unsigned long dflt;

        [[nodiscard]] unsigned int getPortConnectivityBits() const { return (dflt >> 30) & 0x3; }
        [[nodiscard]] unsigned int getGrossLocationBits() const { return (dflt >> 28) & 0x3; }
        [[nodiscard]] unsigned int getGeometricLocationBits() const { return (dflt >> 24) & 0xf; }
        [[nodiscard]] unsigned int getDefaultDeviceBits() const { return (dflt >> 20) & 0xf; }
        [[nodiscard]] unsigned int getConnectionTypeBits() const { return (dflt >> 16) & 0xf; }
        [[nodiscard]] unsigned int getColorBits() const { return (dflt >> 12) & 0xf; }
        [[nodiscard]] bool getJackDetectOverrideToIncapable() const { return (dflt & 0x100) != 0; }
        [[nodiscard]] unsigned int getDefaultAssociation() const { return (dflt >> 4) & 0xf; }
        [[nodiscard]] unsigned int getSequence() const { return dflt & 0xf; }
        [[nodiscard]] bool isPresent() const { return dflt != 0; }

        void log(const char *prefix) const;
    };

    const int CON_LIST_BUF_LEN = 16;

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

        void loadFrom(HdaDevice &dev, unsigned int codecNo, unsigned int nodeNo);

        [[nodiscard]] bool hasInputFrom(unsigned short fromNode) const {
            for(unsigned int i=0; i<numConns;i++) {
                if (connList[i] == fromNode)
                    return true;
            }
        }

        [[nodiscard]] int getOffsetOfInputConnection(unsigned short fromNode) const {
            for(int i=0; i<numConns;i++) {
                if (connList[i] == fromNode)
                    return i;
            }
            return -1;
        }

        [[nodiscard]] unsigned int getWidgetType() const {
            return widgetCaps.getWidgetType();
        }

        void logReport() const;
    };



    struct audio_function_group_capabilities {
        unsigned long caps;

        [[nodiscard]] bool isPresent() const { return caps != 0; }
        [[nodiscard]] bool hasBeepGen() const { return (caps & 0x10000) != 0; }
        [[nodiscard]] unsigned int getInputDelay() const { return (caps >> 8) & 0xF; }
        [[nodiscard]] unsigned int getOutputDelay() const { return caps & 0xF; }

        void log(const char *prefix) const;
    };

    const unsigned int max_audio_widgets = 256;

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

        [[nodiscard]] const widget_info * lookupNode (unsigned char nodeNum) const {
            if (nodeNum < widgetNodeOffset)
                return nullptr;
            unsigned short i = nodeNum - widgetNodeOffset;
            if (i >= widgetCount)
                return nullptr;
            return widgets + i;
        }

        void loadFrom(HdaDevice &dev, unsigned int codecNo, unsigned int node);

        void logReport() const;

        [[nodiscard]] const widget_info * findSpeaker() const;

        unsigned int findPathToDac(unsigned char fromNode, unsigned char *res, unsigned int maxLen) const;

        unsigned int findAssociatedVolumeKnobs(const unsigned char *toNodes, unsigned int toNodesLength,
                                               unsigned char *output, unsigned int outputMaxLen) const;

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

        void loadFrom(HdaDevice &dev, unsigned int codecNo);
        void logReport() const;
    };

}


#endif // XTRS_DPMHW_HDACODEC_H
