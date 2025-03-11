//
// Created by arnold on 12/23/24.
//

#ifndef XTRS_DPMHW_MEMORY_H
#define XTRS_DPMHW_MEMORY_H

#include <cstdlib>
#include <cstdint>
#include <sys/farptr.h>
#include <dpmi.h>
#include <memory>
#include "dpmhw.h"

namespace dpmhw {

    /**
     * Simple wrapper around a dpmi memory selector for ease of use.
     * Does not manage the "life" of the selector (e.g. - does not do anything on destruction).
     */
    class SelectorMem {
    private:
        // The null selector points to the first entry of the global descriptor table which is supposed
        // to be reserved for NULL.
        static const unsigned short null_selector = 0;
    public:
        const unsigned short selector;
        explicit SelectorMem(unsigned short sel) : selector(sel) {}
        SelectorMem() : selector(null_selector) {}

        [[nodiscard]] bool isNull() const { return selector == null_selector; }

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
            // Look at _farpeek*/_farpoke* calls for similar examples
            // Also if doing lots of flushes we might want a way to load %fs once and reuse it (see above comment)
            __asm__ __volatile__ ("movw %w0,%%fs \n"
                                  "	clflush %%fs:(%k1)"
                    :
                    : "rm" (selector), "r" (offset));
        }

        /** Sentinel selector used to mark an invalid selector */
        static SelectorMem invalid() { return SelectorMem(null_selector) ; }

        /**
         * Maps a selector corresponding to a given physical address (usually for accessing device memory)
         * Empty option on error
         */
        static option<SelectorMem> mapDevice(uint32_t addr, uint32_t size);

        /** Call this to free a _succesfully_ allocated descriptor created via mapDevice .
         * Do not call this for other types of descriptors
         */
        static void freeMappedDeviceDescriptor(const SelectorMem & sel) {
            __dpmi_free_ldt_descriptor(sel.selector);
        }

        class ref {
        public:
            const unsigned short selector;
            const uint32_t offset;
            ref(unsigned short selector, uint32_t offset) : selector(selector), offset(offset) {}
            [[nodiscard]] bool isNull() const { return selector == null_selector; }
        };
        class ref8 : public ref {
        public:
            ref8(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint8_t peek() const { return  _farpeekb(selector, offset);  }
            void poke(uint8_t v) const { _farpokeb(selector, offset, v);  }
            static ref8 nullRef() { return { SelectorMem::invalid(), 0}; }
        };
        [[nodiscard]] SelectorMem::ref8 r8(uint32_t offset) const { return {*this, offset}; }

        class ref16 : public ref {
        public:
            ref16(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint16_t peek() const { return  _farpeekw(selector, offset);  }
            void poke(uint16_t v) const { _farpokew(selector, offset, v);  }
            static ref16 nullRef() { return { SelectorMem::invalid(), 0}; }
        };
        [[nodiscard]] SelectorMem::ref16 r16(uint32_t offset) const { return {*this, offset}; }

        class ref32 : public ref {
        public:
            ref32(const SelectorMem &mem, uint32_t offset) : ref(mem.selector, offset) {}
            [[nodiscard]] uint32_t peek() const { return  _farpeekl(selector, offset);  }
            void poke(uint32_t v) const { _farpokel(selector, offset, v);  }
            static ref32 nullRef() { return { SelectorMem::invalid(), 0}; }
        };
        [[nodiscard]] SelectorMem::ref32 r32(uint32_t offset) const { return {*this, offset}; }


    };

    /**
     * A region of memory with known physical address, suitable for DMA
     *
     * You must use the allocate/deallocate methods to get a DMARegion.  So, e.g. you can't
     * put a DMARegion directly in an object - you have to use a pointer
     */
    class DmaRegion {
    public:
        const SelectorMem selector;
        const uint32_t selectorBase;
        const uint32_t physicalBase;
        const uint32_t regionSize;
        const uint32_t alignment;
        const uint8_t regionType;
        const uint16_t xmsHandle;
    private:
        static const uint8_t regionTypeDos = 0;
        static const uint8_t regionTypeXms = 1;

        uint32_t allocated;

        DmaRegion(SelectorMem s, uint32_t sbase, uint32_t pbase, uint32_t sz, uint32_t algn, uint8_t regionType, uint16_t xmsHandle) :
            selector(s) , selectorBase(sbase), physicalBase(pbase), regionSize(sz), alignment(algn)
            ,regionType(regionType), xmsHandle(xmsHandle), allocated(0){
        }

        void * operator new(size_t sz, void * p) {
            return p;
        }

    public:
        DmaRegion() = delete;
        DmaRegion(const DmaRegion &rhs) = delete;
        DmaRegion & operator =(const DmaRegion &rhs) = delete;
        void * operator new(size_t sz) = delete;
        void operator delete(void *p) = delete;

        [[nodiscard]] uint32_t remaining() const {
            return regionSize > allocated ? (regionSize - allocated) : 0;
        }

        const static uint32_t ALLOC_FAILED = 0xFFFFFFFFu;

        /**
         * Note - a zero-initialized DmaBlock (i.e. - default constructor) will show as isError
         */
        class DmaBlock {
        public:
            const SelectorMem selector;
            const uint32_t selectorAddress;
            const uint32_t physicalAddress;
            const uint32_t size;

            DmaBlock(const SelectorMem sel, uint32_t selAddr, uint32_t physAddr, uint32_t sz) :
                selector(sel), selectorAddress(selAddr), physicalAddress(physAddr), size(sz) {}

            DmaBlock() :
                selector(SelectorMem::invalid()), selectorAddress(0), physicalAddress(0), size(0) {}

            [[nodiscard]] bool isError() const {
                return selector.isNull() || selectorAddress == ALLOC_FAILED || physicalAddress == ALLOC_FAILED || size == 0;
            }

            void flushFromCache() const {
                for(int i=0; i< size; i+=64) {
                    selector.flushLine(selectorAddress + i);
                }
            }

            static DmaBlock invalidBlock() {
                return {SelectorMem::invalid(), ALLOC_FAILED, ALLOC_FAILED, 0};
            }

            void fill16(uint16_t value) const;

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
         * Allocates a DMA region. This is one of the two public ways to create a new DMA region.
         * Use "deallocate" to free.  Memory is reserved from conventional memory.
         *
         * Note the alignment refers to the _PHYSICAL_ address, not the selector-relative address.
         *
         * Returns null on failure - be sure to check for that.
         */
        static DmaRegion *allocateConventional(uint32_t size, uint32_t alignment);

        /** Deallocates the given region.  No-op if null is passed */
        static void deallocate(dpmhw::DmaRegion *p);

        /**
         * Allocates a DMA region. This is one of the two public ways to create a new DMA region.
         * Use "deallocate" to free.  Memory is reserved from XMS (e.g. - himem.sys)
         *
         * Note the alignment refers to the _PHYSICAL_ address, not the selector-relative address.
         *
         * Returns null on failure - be sure to check for that.
         */
        static DmaRegion *allocateXms(uint32_t size, uint32_t alignment);

    };



};


#endif //XTRS_DPMHW_MEMORY_H
