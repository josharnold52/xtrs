//
// Created by arnold on 12/23/24.
//

#ifndef XTRS_DPMHW_MEMORY_H
#define XTRS_DPMHW_MEMORY_H

#include <cstdlib>
#include <cstdint>
#include <sys/farptr.h>
#include <dpmi.h>
#include "dpmhw.h"

namespace dpmhw {

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
            ///TODO: I don't think we need to restore the old value value of _fargetsel (a.k.a the fs register)
            //  see the _farpeek* calls for examples where it isn't restored.  ALSO...We don't need to hardcode
            //  the use of eax here - I think we can tell the compiler to use any r/m value and let it choose.
            //  Again see the existing _far* macros for examples.
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
            ref8(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint8_t peek() const { return  _farpeekb(selector, offset);  }
            void poke(uint8_t v) const { _farpokeb(selector, offset, v);  }
        };
        [[nodiscard]] SelectorMem::ref8 r8(uint32_t offset) const { return {*this, offset}; }

        class ref16 : public ref {
        public:
            ref16(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint16_t peek() const { return  _farpeekw(selector, offset);  }
            void poke(uint16_t v) const { _farpokew(selector, offset, v);  }
        };
        [[nodiscard]] SelectorMem::ref16 r16(uint32_t offset) const { return {*this, offset}; }

        class ref32 : public ref {
        public:
            ref32(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint32_t peek() const { return  _farpeekl(selector, offset);  }
            void poke(uint32_t v) const { _farpokel(selector, offset, v);  }
        };
        [[nodiscard]] SelectorMem::ref32 r32(uint32_t offset) const { return {*this, offset}; }


    };

    /**
     * A region of memory with known physical address, suitable for DMA
     */
    class DmaRegion {
    public:
        const SelectorMem selector;
        const uint32_t selectorBase;
        const uint32_t physicalBase;
        const uint32_t regionSize;
        const uint32_t alignment;
    private:
        uint32_t allocated;

        DmaRegion(SelectorMem s, uint32_t sbase, uint32_t pbase, uint32_t sz, uint32_t algn) :
            selector(s) , selectorBase(sbase), physicalBase(pbase), regionSize(sz), alignment(algn), allocated(0) {
        }

        void * operator new(size_t sz, void * p) {
            return p;
        }

    public:
        DmaRegion(const DmaRegion &rhs) = delete;
        DmaRegion & operator =(const DmaRegion &rhs) = delete;
        void * operator new(size_t sz) = delete;
        void operator delete(void *p) = delete;

        [[nodiscard]] uint32_t remaining() const {
            return regionSize > allocated ? (regionSize - allocated) : 0;
        }

        const static uint32_t ALLOC_FAILED = 0xFFFFFFFFu;

        struct DmaBlock {
            const SelectorMem selector;
            const uint32_t selectorAddress;
            const uint32_t physicalAddress;
            const uint32_t size;

            [[nodiscard]] bool isError() const {
                return selectorAddress == ALLOC_FAILED || physicalAddress == ALLOC_FAILED || size == 0;
            }

            void flushFromCache() const {
                for(int i=0; i< size; i+=64) {
                    selector.flushLine(selectorAddress + i);
                }
            }

            static DmaBlock invalidBlock() {
                return {SelectorMem::invalid(), ALLOC_FAILED, ALLOC_FAILED, 0};
            }
        };

        /**
         * Reserve a chunk from this region - returns the offset to the chunk or ALLOC_FAILED.
         * Note that there is no way to release a chunk once reserved.
         *
         * Call isError on the returned value to confirm that it was allocated correctly
         */
        DmaBlock reserveBlock(uint32_t size);

        static DmaBlock reserveBlock(DmaRegion *pRegion, uint32_t sz) {
            return pRegion ? pRegion->reserveBlock(sz) : DmaBlock::invalidBlock();
        }

        /**
         * Allocates a DMA region. This is the only public way to create a new DMA region.
         * Use "deallocate" to free.
         *
         * Note the alignment refers to the _PHYSICAL_ address, not the selector-relative address.
         *
         * Returns null on failure - be sure to check for that.
         */
        static DmaRegion *allocate(uint32_t size, uint32_t alignment);

        /** Deallocates the given region.  No-op if null is passed */
        static void deallocate(DmaRegion *dmar);


    };


};


#endif //XTRS_DPMHW_MEMORY_H
