#pragma once

// --- must-have headers ---
#include "RE/B/BSContainer.h"         // RE::BSContainer::ForEachResult
#include "RE/P/PerkEntryVisitor.h"    // RE::PerkEntryVisitor
#include "RE/B/BGSPerkEntry.h"          // RE::BGSPerkEntry
#include "RE/P/PlayerCharacter.h"       // RE::PlayerCharacter
#include "RE/B/BGSEntryPoint.h"         // RE::BGSEntryPoint::ENTRY_POINTS
#include <cstdint>

// --- your visitor ---
struct CmdLimitVisitor : public RE::PerkEntryVisitor {
    uint32_t& total;
    explicit CmdLimitVisitor(uint32_t& t) : total(t) {}

    // must match RE::PerkEntryVisitor exactly, and fully qualify types
    RE::BSContainer::ForEachResult Visit(RE::BGSPerkEntry* entry) override {
        // Cast to the real subclass that holds the data
        auto* epEntry = static_cast<RE::BGSEntryPointPerkEntry*>(entry);

        // Grab the functionData pointer and cast to the ONE‐VALUE variant
        auto* fv = static_cast<RE::BGSEntryPointFunctionDataOneValue*>(
            epEntry->GetFunctionData()
        );

        // 'fv->data' is the float magnitude specified by the Creation Kit
        total += static_cast<std::uint32_t>(fv->data);
        // total += static_cast<uint32_t>(entry->entryData.fMagnitude);
        // total += static_cast<uint32_t>(entry->header.unk4);
        logger::info("entry id: {}, unk4: {}, unk2: {}, rank: {}, type: {}, priority: {}", static_cast<uint32_t>(entry->GetID()), entry->header.unk4, static_cast<uint32_t>(entry->header.unk2), static_cast<uint32_t>(entry->GetRank()), static_cast<uint32_t>(entry->GetType()), static_cast<uint32_t>(entry->header.priority));
        logger::info("magnitude: {}", fv->data);
        return RE::BSContainer::ForEachResult::kContinue;
    }
};

// // --- your getter ---
// inline uint32_t GetPlayerSummonLimit() {
//     auto* player = RE::PlayerCharacter::GetSingleton();
//     if (!player) {
//         return 0;
//     }

//     uint32_t total = player->GetPlayerRuntimeData().commandedActorLimit;
//     CmdLimitVisitor visitor(total);
//     player->ForEachPerkEntry(
//         RE::BGSEntryPoint::ENTRY_POINTS::kModCommandedActorLimit,
//         visitor
//     );
//     return total;
// }