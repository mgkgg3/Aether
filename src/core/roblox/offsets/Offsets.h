#pragma once

// Host layer redirect.
//
// The Lua VM's host layer (src/core/memory, src/core/roblox/classes, and the
// vm/ files) was written against the offsets dump that shipped with it -
// version-02c37bc51a384b8f.  This tree's src/sdk/offsets.h is the current
// client (version-7a0c6d10fcb34a2c) and carries the same namespaces, so the
// host layer reads this client through it instead of through a frozen copy of
// a superseded dump.  `tools/update_offsets.py` keeps that file current; the
// copy that used to live here was 88 values behind and would have had the VM
// reading the wrong fields while looking like it worked.
//
// The few names the older dump had and the current one does not are aliased
// below, each to the entry that means the same thing - not to an invented
// value.  (The client-heap entries - LuaState::*, LuauGlobal::*, the GC
// internals - have no equivalent here on purpose: nothing in this tree reads
// the client's Luau heap, see vm/LuaGc.cpp's note.)
#include "sdk/offsets.h"

namespace Offsets
{
    namespace Instance
    {
        // The creator object's vtable: slot 0 creates an instance.  The
        // current dump names the same slot ICreator::Create.
        inline constexpr uintptr_t Creator_create = ICreator::Create;

        // Raw Instance::SetParent(instance, parent).  The old dump called this
        // Instance::SetParent; the current one groups it under Functions.
        inline constexpr uintptr_t SetParent = Functions::SetParent;
    }

    // The name is one indirection, not two.
    //
    // This client keeps the character pointer in the name container itself, so
    // `read<ptr>(instance + NameContainer)` *is* the string.  Everything that
    // reads a name goes straight there; if a future dump ever gives Name a
    // non-zero value again, this fires and the two readers have to grow the
    // second hop back rather than quietly reading the first bytes of a string
    // as a pointer.
    static_assert(Instance::Name == 0,
        "Instance::Name is no longer 0 - the name readers must add the extra hop back");
}
