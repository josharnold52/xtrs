//
// Created by arnold on 1/21/25.
//

#ifndef XTRS_DPMUTIL_INI_H
#define XTRS_DPMUTIL_INI_H


namespace dpmutil {



    class IniSettings {
    public:
        struct _bufDesc {
            const char * pBuf;
            int size;
        };
    private:
        const _bufDesc buffer;

    public:
        [[maybe_unused]] explicit IniSettings(const char *filePath);
        IniSettings() : buffer({nullptr, 0}) {}
        ~IniSettings();
        IniSettings(const IniSettings &rhs) = delete;
        IniSettings & operator=(const IniSettings &rhs) = delete;

        [[nodiscard]] bool hadLoadError() const {
            return buffer.pBuf == nullptr;
        }

        [[nodiscard]] bool readInt(const char *section, const char *key, int &value) const ;
        void readInt(const char *section, const char *key, int &value, int defaultValue) const {
            if (!readInt(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] bool readDouble(const char *section, const char *key, double &value) const;
        void readDouble(const char *section, const char *key, double &value, double defaultValue) const {
            if (!readDouble(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] int getStringLength(const char *section, const char *key) const;  //-1 if not found - does not include null terminator
        /**
         * Attempts to read the given setting
         * @param section the section
         * @param key the key
         * @param valueBuf the buffer to receive the value
         * @param valueBufSize the size of the buffer (including space for the null terminator).  If the buffer size is insufficient,
         * then the function will return false and the value will not be read.
         * @param valueLength receives the length of the value (does NOT include the null terminal).  Unchanged if the return value is false
         * @return true if the value could be read, false otherwise.  If false is returned, neither the buffer nor valueLength will be
         * changed.  Reasons for a false return value include: Unable to find the key, Insufficient Buffer, and Invalid Arguments
         */
        [[nodiscard]] bool readString(const char *section, const char *key, char *valueBuf, size_t valueBufSize, int &valueLength) const; //Error if insufficient buffer (including if no room for null terminator)

        /** Same as the overloaded version except it does not take a valueLength param and thus the length is not returned */
        [[nodiscard]] bool readString(const char *section, const char *key, char *valueBuf, size_t valueBufSize) const {
            int placeholder;
            return readString(section, key, valueBuf, valueBufSize, placeholder);
        };

        [[nodiscard]] bool readBoolean(const char *section, const char *key, bool &value) const;
        void readBoolean(const char *section, const char *key, bool &value, bool defaultValue) const {
            if (!readBoolean(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] bool readChar(const char *section, const char *key, char &value) const {
            char buf[2];
            if (!readString(section, key, buf, 2) || !buf[0] || buf[1]) {
                return false;
            }
            value = buf[0];
            return true;
        }
        void readChar(const char *section, const char *key, char &value, char defaultValue) const {
            if (!readChar(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] int getIntOrElse(const char *section, const char *key, int defaultValue) const {
            int v;
            readInt(section, key, v, defaultValue);
            return v;
        }
        [[nodiscard]] double getDoubleOrElse(const char *section, const char *key, double defaultValue) const {
            double v;
            readDouble(section, key, v, defaultValue);
            return v;
        }
        [[nodiscard]] bool getBooleanOrElse(const char *section, const char *key, bool defaultValue) const {
            bool v;
            readBoolean(section, key, v, defaultValue);
            return v;
        }
        [[nodiscard]] char getCharOrElse(const char *section, const char *key, char defaultValue) const {
            char v;
            readChar(section, key, v, defaultValue);
            return v;
        }

    };
}

#endif //XTRS_DPMUTIL_INI_H
