//
// Created by arnold on 12/23/24.
//

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <go32.h>
#include "dpmhw.h"
#include "dpmhw_memory.h"
#include "dpmhw_impl.h"


static unsigned long prepare_bios_regs(__dpmi_regs *regs) {
    memset(regs, 0, sizeof(__dpmi_regs));
    regs->x.ss = __tb >> 4;     /* nearest transfer buffer segment  */
    //Set SP.  Subtract 8 because I can never remember if top of stack is where
    // the next pushed value goes.  Round down to 16 bit alignment.    Note that
    // we also may be up to 15 bytes from the top of the transfer buffer because
    // it may not start on a real-mode segment boundary
    regs->x.sp = (_go32_info_block.size_of_transfer_buffer - 8) & 0xFFF0;

    //In case we need other dos memory, set ds and es to the transfer buffer.
    //This time we add 1 to the segments to ensure that offset 0 is in the buffer.
    regs->x.es = regs->x.ss + 1;
    regs->x.ds = regs->x.ss + 1;

    //Redundant, but keeping it here because _dpmi says we should do this or else
    //must set a valid? flags register...
    regs->x.flags = 0;

    //Return the usable space starting at offset 0 from the ds/es sergments.
    //We need to reserve 1K of stack (from the dpmi spec).  Also
    //subtract 16 because the ds/es are 1 above ss.  Finally subtract another
    //16 in case I did something wrong...
    return regs->x.sp - 1024 - 32;
}


static __dpmi_raddr findXms() {
    static std::atomic<uint32_t> xms{0};

    auto chk = xms.load();
    if (!chk) {
        __dpmi_regs regs;
        prepare_bios_regs(&regs);
        regs.h.ah  = 0x43;
        regs.h.al = 0;
        auto rmires = __dpmi_simulate_real_mode_interrupt(0x2f, &regs);
        uint32_t upd = ~0;
        if (rmires || regs.h.al != 0x80) {
            dpmhw::dpmhw_log("XMS Driver not detected 0x%04x\n", regs.x.ax);
        } else {
            prepare_bios_regs(&regs);
            regs.h.ah = 0x43;
            regs.h.al = 0x10;
            rmires = __dpmi_simulate_real_mode_interrupt(0x2f, &regs);
            if (rmires) {
                dpmhw::dpmhw_log("Failed to get XMS Driver address\n");
            } else {
                auto xmsseg = regs.x.es;
                auto xmsoff = regs.x.bx;
                upd = (((uint32_t)xmsseg) << 16) | ((uint32_t)xmsoff);
                dpmhw::dpmhw_log("XMS Driver is at %04hx:%04hx\n", xmsseg, xmsoff);
            }
        }
        if (xms.compare_exchange_strong(chk, upd)) {
            chk = upd;
        }
    }
    if (chk == ~0) {
        return {0,0};
    }
    __dpmi_raddr res;
    res.offset16 = (uint16_t)(chk);
    res.segment = (uint16_t)(chk >> 16);
    return res;
}


class XmsMemory {
public:
    const uint16_t handle;
    const uint32_t lockedAddress;
    const bool valid;
private:
    bool owned;
public:
    XmsMemory() : handle(0), lockedAddress(0), valid(false), owned(false) {}
    XmsMemory(uint16_t handle, bool owned) : handle(handle), lockedAddress(0), valid(true), owned(owned) {}
    XmsMemory(uint16_t handle, uint32_t lockedAddress, bool owned) : handle(handle), lockedAddress(lockedAddress), valid(true), owned(owned) {}
    XmsMemory(const XmsMemory &rhs) = delete;
    XmsMemory(XmsMemory &&rhs) : handle(rhs.handle),lockedAddress(rhs.lockedAddress),valid(rhs.valid),owned(rhs.owned) {
        rhs.owned = false;
    }
    XmsMemory &operator=(const XmsMemory &rhs) = delete;
    XmsMemory &operator=(const XmsMemory &&rhs) = delete;
    ~XmsMemory() {
        if (!valid || !owned) {
            return;
        }
        auto xms = findXms(); // Can assume this succeeds
        if (lockedAddress) {
            dpmhw::dpmhw_log("XMS: Unlocking %u at 0x%08x\n", handle, lockedAddress);
            __dpmi_regs regs;
            memset(&regs, 0, sizeof(regs));
            regs.h.ah = 0x0D; //Unlock
            regs.x.dx = handle;
            regs.x.cs = xms.segment;
            regs.x.ip = xms.offset16;
            __dpmi_simulate_real_mode_procedure_retf(&regs);
        }
        dpmhw::dpmhw_log("XMS: Freeing %u\n", handle);
        __dpmi_regs regs;
        memset(&regs, 0, sizeof(regs));
        regs.h.ah = 0x0A; //Free
        regs.x.dx = handle;
        regs.x.cs = xms.segment;
        regs.x.ip = xms.offset16;
        __dpmi_simulate_real_mode_procedure_retf(&regs);
    }

    static XmsMemory allocate(uint32_t  size) {
        auto xms = findXms(); // Can assume this succeeds
        if (xms.segment == 0 && xms.offset16 == 0) {
            dpmhw::dpmhw_log("XMS: Not available\n");
            return {};
        }
        __dpmi_regs regs;
        memset(&regs, 0, sizeof(regs));
        regs.h.ah = 0x08; //Query Free
        regs.x.cs = xms.segment;
        regs.x.ip = xms.offset16;
        dpmhw::dpmhw_log("XMS: Calling XMS Query Free (%x:%x) ah=%u,dx=%u\n", regs.x.cs, regs.x.ip, regs.h.ah, regs.x.dx);
        if (__dpmi_simulate_real_mode_procedure_retf(&regs) != 0) {
            dpmhw::dpmhw_log("real mode call failed\n");
            return {};
        }
        dpmhw::dpmhw_log("XMS: Query Free Returned: ax=0x%04x,dx=0x%04x,bl=0x%02x\n", regs.x.ax, regs.x.dx, regs.h.bl);

        //https://github.com/MikeyG/himem/blob/master/spec/xms.txt
        const uint32_t sizeRounded = ((size + 4095) & (~4095));
        memset(&regs, 0, sizeof(regs));
        regs.h.ah = 0x09; //Allocate
        regs.x.dx = sizeRounded >> 10;
        regs.x.cs = xms.segment;
        regs.x.ip = xms.offset16;
        dpmhw::dpmhw_log("XMS: Calling XMS Allocation (%x:%x) ah=%u,dx=%u\n", regs.x.cs, regs.x.ip, regs.h.ah, regs.x.dx);
        if (__dpmi_simulate_real_mode_procedure_retf(&regs) != 0) {
            dpmhw::dpmhw_log("real mode call failed\n");
            return {};
        }
        if (regs.x.ax != 1) {
            dpmhw::dpmhw_log("XMS: Allocation failed (ax=0x%04x, dx=0x%04x, bl=0x%02x)\n", regs.x.ax, regs.x.dx, regs.h.bl);
            return {};
        }
        dpmhw::dpmhw_log("XMS: Allocation succeeded (ax=0x%04x, dx=0x%04x, bl=0x%02x)\n", regs.x.ax, regs.x.dx, regs.h.bl);
        return { regs.x.dx, true };
    }

    operator bool () const { return valid; } // NOLINT(*-explicit-constructor)
    [[nodiscard]] bool isOwned() const { return owned; }
    [[nodiscard]] bool isValid() const { return valid; }
    [[nodiscard]] bool isLocked() const { return valid && lockedAddress != 0; }

    void release() {
        owned = false;
    }

    /**
     * Attempts to lock this memory.  If successful, returns a locked XmsMemory and transfers
     * ownership to that object.   If it fails, an invalid XmsMemory is returned and ownership
     * stays with this object.
     * <p>
     * If this object does not own its handle then an invalid XmsMemory is returned.
     * <p>
     * If the handle is already locked, a locked XmsMemory is returned with the same address and
     * ownership is transferred to it.
     */
    XmsMemory lock() {
        if (!owned || !valid) {
            dpmhw::dpmhw_log("XMS: lock called on an invalid or unowned xms block\n");
            return {};
        }
        if (isLocked()) {
            owned = false;
            return {handle, lockedAddress, true};
        }
        auto xms = findXms(); // Can assume this succeeds
        __dpmi_regs regs;
        memset(&regs, 0, sizeof(regs));
        regs.h.ah = 0x0C; //Lock
        regs.x.dx = handle;
        regs.x.cs = xms.segment;
        regs.x.ip = xms.offset16;
        dpmhw::dpmhw_log("XMS: Calling XMS Lock (%x:%x) ah=%u,dx=%u\n", regs.x.cs, regs.x.ip, regs.h.ah, regs.x.dx);
        if (__dpmi_simulate_real_mode_procedure_retf(&regs) != 0) {
            dpmhw::dpmhw_log("real mode call failed\n");
            return {};
        }
        if (regs.x.ax != 1) {
            dpmhw::dpmhw_log("XMS: Lock failed (ax=0x%04x, dx=0x%04x, bl=0x%02x)\n", regs.x.ax, regs.x.dx, regs.h.bl);
            return {};
        }
        dpmhw::dpmhw_log("XMS: Lock succeeded (ax=0x%04x, dx=0x%04x, bx=0x%04x)\n", regs.x.ax, regs.x.dx, regs.x.bx);
        owned = false;
        return { handle, (((uint32_t)regs.x.dx) << 16) | ((uint32_t)regs.x.bx), true };
    }


};


dpmhw::option<dpmhw::SelectorMem> dpmhw::SelectorMem::mapDevice(uint32_t addr, uint32_t size) {
    if (size >= 0x100000) {
        dpmhw_log("ERROR: Segments > 1M not supported (because I have to be smarter about granularity bit\n");
        return option<SelectorMem>(false, SelectorMem::invalid());
    }
    __dpmi_meminfo mi;
    mi.size=size;
    mi.address = addr;
    mi.handle = 0;
    dpmhw_log("mapping phys 0x%08x(0x%08x)\n", addr, size);
    if (__dpmi_physical_address_mapping(&mi)!=0) {
        dpmhw_log("ERROR: DPMI map of %x(%u) failed\n", addr,size);
        return option<SelectorMem>(false, SelectorMem::invalid());
    }
    dpmhw_log("mapped phys 0x%08x(0x%08x)\n", mi.address, mi.size);
    int sel = __dpmi_allocate_ldt_descriptors(1);
    if (sel  == -1) {
        dpmhw_log("ERROR: Unable to allocate descriptor\n");
        return option<SelectorMem>(false, SelectorMem::invalid());
    }
    // NOTE: I think this should be set to the linear address - not the physical address.
    //   I think I got this wrong before but got away with it since they are the same in simple CDWDPMI
    //Access rights - Data, RW, Ring 3, size in bytes
    if (__dpmi_set_segment_base_address(sel, mi.address) |
        __dpmi_set_segment_limit(sel, size - 1) |
        __dpmi_set_descriptor_access_rights(sel, 0x4F3) ) {
        dpmhw_log("Unable to set descriptor params\n");
        return option<SelectorMem>(false, SelectorMem::invalid());
    }
    return option<SelectorMem>(SelectorMem(sel));
}



dpmhw::DmaRegion *dpmhw::DmaRegion::allocateConventional(uint32_t size, uint32_t alignment) {
    //For now, we'll allocate this as DOS memory because it is easy to both figure out its physical
    //address and get a selector for it.   Eventually we should change this to allocating physical
    //memory elsewhere - See https://www.delorie.com/djgpp/v2faq/faq18_13.html

    dpmhw_log("Attempting to allocate DMA region of size %u and alignment %u\n", size, alignment);

    if (size == 0 || size > 0x42000 || !isPowerOfTwo(alignment) || alignment > 0x1000) {
        dpmhw_log("Bad arguments %u %u\n", size, alignment);
        return nullptr;
    }
    auto p = malloc(sizeof(DmaRegion));
    if (!p) {
        dpmhw_log("Error allocating DmaRegion\n");
        return nullptr;
    }
    const auto paras = (size + alignment + 15u) >> 4;
    int selector;
    int seg = __dpmi_allocate_dos_memory((int)paras, &selector);
    if (seg == -1) {
        dpmhw_log("Error allocating dos memory for DMA region\n");
        return nullptr;
    }
    uint32_t physAddress = ((uint32_t)seg) << 4;
    uint32_t fixup = 0;
    if (physAddress & (alignment - 1)) {
        fixup = alignment - (physAddress & (alignment - 1));
    }

    return new (p) DmaRegion(SelectorMem((uint16_t)selector), fixup, physAddress + fixup, size, alignment,
                             DmaRegion::regionTypeDos, 0);
}

void dpmhw::DmaRegion::deallocate(dpmhw::DmaRegion *p) {
    if (!p) {
        return;
    }
    if (p->regionType == DmaRegion::regionTypeDos) {
        __dpmi_free_dos_memory(p->selector.selector);
    } else if (p->regionType == DmaRegion::regionTypeXms) {
        SelectorMem::freeMappedDeviceDescriptor(p->selector);
        //Copy the Xms info back into a XmsMemory so that it gets freed;
        XmsMemory (p->xmsHandle, p->physicalBase, true); // NOLINT(*-unused-raii)
    } else {
        dpmhw_log("Deallocating unknown region type %d\n", p->regionType);
    }
    free(p);  //This is how we allocate DmaRegion pointers
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

dpmhw::DmaRegion *dpmhw::DmaRegion::allocateXms(uint32_t size, uint32_t alignment) {
    if (size == 0 || size > 0x200000 || !isPowerOfTwo(alignment) || alignment > 0x1000) {
        dpmhw_log("Bad arguments %u %u\n", size, alignment);
        return nullptr;
    }
    auto xms = findXms();
    if (xms.segment == 0 && xms.offset16 == 0) {
        dpmhw_log("XMS Driver not found\n");
        return nullptr;
    }
    auto p = std::unique_ptr<void,void(*)(void*)>(malloc(sizeof(DmaRegion)), free);

    if (!p) {
        dpmhw_log("Error allocating DmaRegion\n");
        return nullptr;
    }
    //I'm going to assume alignment isn't an issue for XMS allocation...
    XmsMemory x1(XmsMemory::allocate(size));
    XmsMemory x2(x1.lock());
    if (!x2.isLocked()) {
        return nullptr;
    }
    auto mem = SelectorMem::mapDevice(x2.lockedAddress, size);
    if (!mem.exists()) {
        return nullptr;
    }

    dpmhw_log("XMS Allocated XMS %u(0x%08x) - Mapped to %u(0x%08x)\n", x2.handle, x2.lockedAddress, mem.get().selector, 0);

    x2.release();
    return new(p.release()) DmaRegion(mem.get(), 0, x2.lockedAddress, size, alignment, regionTypeXms, x2.handle);

}
