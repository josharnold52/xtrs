//
// Created by arnold on 12/22/24.
//

#ifndef XTRS_DPMHW_IMPL_H
#define XTRS_DPMHW_IMPL_H

#include <cstring>
#include <sys/farptr.h>
#include <dpmi.h>
#include <cstdint>

#define INLINE_PAUSE  { __asm__ __volatile__ ("pause"); }

namespace dpmhw {


    /**
     * A template wrapper around qsort for sorting pointers to a type T.  Null pointers
     * always sort to the end of the list.  The "run" method does the actual sort
     * @tparam T the type of objects to sort
     * @tparam cf functopn that compares the 2 objects
     */
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
        static void run(const T **base, size_t arrlen) {
            qsort(base, arrlen, sizeof(T*), vcf);
        }
    };



    template<unsigned int sz> class bitset {
        private:
        unsigned int bits[(sz + 31)>>5]{};

        public:
        void add(unsigned int x) {
            if (x < sz) {
                bits[(x>>5)] |= 1u << (x & 31);
            }
        }

        void addAll(const bitset<sz> &rhs) {
            const unsigned int arrsz = (sz + 31)>>5;
            for(unsigned int i=0;i<arrsz;i++) {
                bits[i] |= rhs.bits[i];
            }
        }
        [[nodiscard]] bool contains(unsigned int x) const {
            if (x < sz)
                return (bits[(x>>5) & 7] & (1u << (x & 31))) != 0;
            else
                return false;
        }

        [[nodiscard]] unsigned int not_found() const {
            return sz;
        }

        /** Returns sz (or "not_found()" if not found */
        [[nodiscard]] unsigned int findNext(unsigned int startAt) const {
            //Can optimize this...
            for(unsigned int i=startAt; i < sz; i++) {
                if (contains(i)) {
                    return i;
                }
            }
            return sz;
        }
    };

    /**
     * Simple wrapper around a dpmi memory selector for ease of use.
     * Does not manage the "life" of the selector (e.g. - does not do anything on destruction).
     */
    class SelectorMem {
    public:
        unsigned short selector;
        explicit SelectorMem(unsigned short sel) : selector(sel) {}
        [[nodiscard]] uint8_t peek8 (uint32_t offset) const { return  _farpeekb(selector, offset); }
        [[nodiscard]] uint16_t peek16 (uint32_t offset) const { return  _farpeekw(selector, offset); }
        [[nodiscard]] uint32_t peek32 (uint32_t offset) const { return  _farpeekl(selector, offset); }
        void poke8 (unsigned long offset, uint8_t v) const {  _farpokeb(selector, offset, v); }
        void poke16 (unsigned long offset, uint16_t v) const {  _farpokew(selector, offset, v); }
        void poke32 (unsigned long offset, uint32_t v) const {  _farpokel(selector, offset, v); }
        /*
         * NOTE: If accessing in a tight loop, better to load the descriptor into a segment
         * reg and access relative that reg many times.  DJGPP has macros for this (see the other _far
         * macros).  Or can do it ourselves with inline assembly
         */

        /** Flushes the cache line at the given offset */
        void flushLine(uint32_t offset) const {
            unsigned short sv = _fargetsel();
            _farsetsel(selector);
            __asm__ __volatile__ ("clflush %%fs:(%%eax)" : /*none*/ : "a" (offset));
            _farsetsel(sv);
        }

        /** Sentinel selector used to mark an invalid selector */
        static SelectorMem invalid() { return SelectorMem(0) ; }

        /**
         * Maps a selector corresponding to a given physical address (usually for accessing device memory)
         */
        static option<SelectorMem> mapDevice(uint32_t addr, uint32_t size) {
            if (size >= 0x100000) {
                dpmhw_log("ERROR: Segments > 1M not supported (because I have to be smarter about granularity bit\n");
                return option<SelectorMem>(false, SelectorMem::invalid());
            }
            __dpmi_meminfo mi;
            mi.size=size;
            mi.address = addr;
            mi.handle = 0;
            if (__dpmi_physical_address_mapping(&mi)!=0) {
                dpmhw_log("ERROR: DPMI map of %x(%u) failed\n", addr,size);
                return option<SelectorMem>(false, SelectorMem::invalid());
            }
            int sel = __dpmi_allocate_ldt_descriptors(1);
            if (sel  == -1) {
                dpmhw_log("ERROR: Unable to allocate descriptor\n");
                return option<SelectorMem>(false, SelectorMem::invalid());
            }
            //Access rights - Data, RW, Ring 3, size in bytes
            if (__dpmi_set_segment_base_address(sel, addr) |
                __dpmi_set_segment_limit(sel, size - 1) |
                __dpmi_set_descriptor_access_rights(sel, 0x4F3) ) {
                dpmhw_log("Unable to set descriptor params\n");
                return option<SelectorMem>(false, SelectorMem::invalid());
            }
            return option<SelectorMem>(SelectorMem(sel));
        }

        class ref {
        public:
            const unsigned short selector;
            const uint32_t offset;
            ref(unsigned short selector, uint32_t offset) : selector(selector), offset(offset) {}
        };
        class ref8 : public ref {
        public:
            ref8(SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint8_t peek() const { return  _farpeekb(selector, offset);  }
            void poke(uint8_t v) const { _farpokeb(selector, offset, v);  }
        };
        SelectorMem::ref8 r8(uint32_t offset) { return {*this, offset}; }
        class ref16 : public ref {
        public:
            ref16(SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint16_t peek() const { return  _farpeekw(selector, offset);  }
            void poke(uint16_t v) const { _farpokew(selector, offset, v);  }
        };
        SelectorMem::ref16 r16(uint32_t offset) { return {*this, offset}; }
        class ref32 : public ref {
        public:
            ref32(SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint32_t peek() const { return  _farpeekl(selector, offset);  }
            void poke(uint32_t v) const { _farpokel(selector, offset, v);  }
        };
        SelectorMem::ref32 r32(uint32_t offset) { return {*this, offset}; }


    };


};



#endif //XTRS_DPMHW_IMPL_H
