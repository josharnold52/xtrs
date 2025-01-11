//
// Created by arnold on 12/23/24.
//

#include <cstdlib>
#include "dpmhw.h"
#include "dpmhw_memory.h"
#include "dpmhw_impl.h"


dpmhw::DmaRegion *dpmhw::DmaRegion::allocate(uint32_t size, uint32_t alignment) {
    //For now, we'll allocate this as DOS memory because it is easy to both figure out its physical
    //address and get a selector for it.   Eventually we should change this to allocating physical
    //memory elsewhere - See https://www.delorie.com/djgpp/v2faq/faq18_13.html

    if (size == 0 || size > 0x20000 || !isPowerOfTwo(alignment) || alignment > 0x1000) {
        dpmhw_log("Bad arguments %u %u\n", size, alignment);
        return nullptr;
    }
    auto p = malloc(sizeof(DmaRegion));
    if (!p) {
        dpmhw_log("Error allocating DmaRegion");
        return nullptr;
    }
    const auto paras = (size + alignment + 15u) >> 4;
    int selector;
    int seg = __dpmi_allocate_dos_memory((int)paras, &selector);
    if (seg == -1) {
        dpmhw_log("Error allocating dos memory for DMA region");
        return nullptr;
    }
    uint32_t physAddress = ((uint32_t)seg) << 4;
    uint32_t fixup = 0;
    if (physAddress & (alignment - 1)) {
        fixup = alignment - (physAddress & (alignment - 1));
    }

    return new (p) DmaRegion(SelectorMem((uint16_t)selector), fixup, physAddress + fixup, size, alignment);
}

void dpmhw::DmaRegion::deallocate(dpmhw::DmaRegion *p) {
    if (p) {
        __dpmi_free_dos_memory(p->selector.selector);
        free(p);
    }
}


dpmhw::DmaRegion::DmaBlock dpmhw::DmaRegion::reserveBlock(uint32_t size) {
    if (size == 0 || size > remaining()) {
        return DmaBlock::invalidBlock();
    }
    const uint32_t offset = allocated;
    allocated += size;
    const uint32_t rem = size % alignment;
    if (rem != 0) {
        const uint32_t extra = alignment - rem;
        if (remaining() >= extra) {
            allocated += extra;
        } else {
            allocated = regionSize;
        }
    }
    return { selector, selectorBase + offset, physicalBase + offset, size};
}

void dpmhw::DmaRegion::DmaBlock::fill16(uint16_t value) const {
    //TODO: Can optimize this by preloading the selector
    for(uint32_t offset = 0; offset < size; offset += 2) {
        selector.poke16(selectorAddress + offset, value);
    }
    flushFromCache();
}