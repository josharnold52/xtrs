//
// Created by arnold on 12/22/24.
//

#ifndef XTRS_DPMHW_H
#define XTRS_DPMHW_H

#include <cstdint>

namespace dpmhw {
    extern bool dpmhw_debug_enabled;
    void dpmhw_debug(const char *msg, ...);
    void dpmhw_log(const char *fmt, ...);

    inline int64_t dpmhw_rdtsc(){
        int64_t tick;
        __asm__ __volatile__("rdtsc":"=A"(tick));
        return tick;
    }

    inline void dpmhw_x86pause() {
        __asm__ __volatile__ ("pause");
    }


    /**
     * Kind of like option in java/scala but has a placeholder value
     * even in the invalid case.
     * @tparam T
     */
    template <class T> class option {
    private:
        const bool valid;
        T value;
    public:
        explicit option(const T &value) : valid(true), value(value) {}
        explicit option(bool valid, const T &value) : valid(valid), value(value) {}

        [[nodiscard]] bool exists() const { return valid; }

        /** Dereferences a null (segfault) if not exists */
        T * operator->() { return valid ? &value : static_cast<T*>(0); }
        /** Dereferences a null (segfault) if not exists */
        T * operator->() const { return valid ? &value : static_cast<T*>(0); }

        T & get() { return value; }
        const T & get() const { return value; }
    };

};

#endif //XTRS_DPMHW_H
