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
        ~IniSettings();
        IniSettings(const IniSettings &rhs) = delete;
        IniSettings & operator=(const IniSettings &rhs) = delete;

        [[nodiscard]] bool hadLoadError() const {
            return buffer.pBuf == nullptr;
        }

        [[nodiscard]] bool getInt(const char *section, const char *key, int &value) const ;
        void getInt(const char *section, const char *key, int &value, int defaultValue) const {
            if (!getInt(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] bool getDouble(const char *section, const char *key, double &value) const;
        void getDouble(const char *section, const char *key, double &value, double defaultValue) const {
            if (!getDouble(section, key, value)) {
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
        [[nodiscard]] bool getString(const char *section, const char *key, char *valueBuf, size_t valueBufSize, int &valueLength) const; //Error if insufficient buffer (including if no room for null terminator)

        /** Same as the overloaded version except it does not take a valueLength param and thus the length is not returned */
        [[nodiscard]] bool getString(const char *section, const char *key, char *valueBuf, size_t valueBufSize) const {
            int placeholder;
            return getString(section, key, valueBuf, valueBufSize, placeholder);
        };

        [[nodiscard]] bool getBoolean(const char *section, const char *key, bool &value) const;
        void getBoolean(const char *section, const char *key, bool &value, bool defaultValue) const {
            if (!getBoolean(section, key, value)) {
                value = defaultValue;
            }
        }

        [[nodiscard]] bool getChar(const char *section, const char *key, char &value) const {
            char buf[2];
            if (!getString(section, key, buf, 2) || !buf[0] || buf[1]) {
                return false;
            }
            value = buf[0];
            return true;
        }
        void getChar(const char *section, const char *key, char &value, char defaultValue) const {
            if (!getChar(section, key, value)) {
                value = defaultValue;
            }
        }

    };
}

#endif //XTRS_DPMUTIL_INI_H
