#include "SpellCastEventHandler.h"

#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"
#include "Visitor.h"

// struct CmdLimitVisitor : RE::PerkEntryVisitor {
//     std::uint32_t& total;
//     CmdLimitVisitor(std::uint32_t& t) : total(t) {}

//     // Called once per matching perk entry
//     BSContainer::ForEachResult Visit(RE::BGSPerkEntry* entry) override {
//         // entry->data.fMagnitude is the float amount
//         total += static_cast<std::uint32_t>(entry->entryData.fMagnitude);
//         return BSContainer::ForEachResult::kContinue;
//     }
// };

// int GetSummonLimit(RE::Actor* actor) {
//     if (!actor) return 0;

//     int summonLimit = 1;  // Default is 1 summon

//     auto activeEffects = actor->GetMagicTarget()->GetActiveEffectList();
//     if (!activeEffects) return summonLimit;

//     for (const auto& effect : *activeEffects) {
//         if (!effect || !effect->effect) continue;

//         auto baseEffect = effect->effect->baseEffect;
//         if (!baseEffect) continue;

//         // Check if it's a summon limit modifier
//         if (baseEffect->data.archetype == RE::EffectArchetypes::ArchetypeID::kSummonCreature) {
//             if (baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsMagnitude)) {
//                 // Usually Twin Souls sets this via magnitude
//                 summonLimit = static_cast<int>(effect->magnitude);
//             }
//         }
//     }

//     return summonLimit;
// }

// int GetSummonLimit(RE::Actor* actor) {
//     if (!actor) return 0;

//     int baseLimit = 1;  // Default is 1 summon
//     float additional = 0.0f;

//     auto activeEffects = actor->GetMagicTarget()->GetActiveEffectList();
//     if (!activeEffects) return baseLimit;

//     for (const auto& effect : *activeEffects) {
//         if (!effect || !effect->effect) continue;

//         auto baseEffect = effect->effect->baseEffect;
//         if (!baseEffect) continue;

//         if (baseEffect->data.archetype == RE::EffectArchetypes::ArchetypeID::kSummonCreature &&
//             baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsMagnitude)) {
//             additional += effect->magnitude;
//         }
//     }

//     return baseLimit + static_cast<int>(additional);
// }

/// @brief Returns the current maximum number of commanded (summoned) actors.
/// @return The integer value of the "iMaxSummonedCreatures" game setting, or 0 on failure.
std::int32_t GetPlayerSummonLimit() {
    auto coll =
        RE::GameSettingCollection::GetSingleton();  // get the settings singleton :contentReference[oaicite:0]{index=0}
    if (auto setting = coll->GetSetting("iMaxSummonedCreatures")) {  // look up the summon‐limit setting
        return setting->GetUInt();                                   // read its integer value
    }
    return 0;
}

/// Returns the total summoned/commanded‐actor limit for the player,
/// including any bonuses from perks (Modify Commanded Actor Limit).
inline std::uint32_t GetPlayerSummonLimit1() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return 0;
    }

    // 1) Get the “base” limit as stored in PlayerRuntimeData.
    const auto rt = player->GetPlayerRuntimeData();
    std::uint32_t baseLimit = GetPlayerSummonLimit();

    // 2) Add any perk‐based bonuses to that limit.
    // float bonus = RE::BGSEntryPoint::GetValue(
    //     player,
    //     RE::BGSEntryPoint::EntryPoint::kModCommandedActorLimit
    // );

    // RE::PerkEntryVisitor visitor;

    // 2) Walk every active perk with kModCommandedActorLimit (entry 68) and sum its value
    // player->ForEachPerkEntry(
    //     RE::BGSEntryPoint::ENTRY_POINT::kModCommandedActorLimit,  // entry 68 :contentReference[oaicite:1]{index=1}
    //     [&total](const RE::BGSEntryPoint::PerkEntryVisitorParams& /*unused*/, float value) {
    //         total += static_cast<std::uint32_t>(value);
    //         return true;  // continue iterating through perks
    //     });

    CmdLimitVisitor visitor(baseLimit);
    player->ForEachPerkEntry(RE::BGSEntryPoint::ENTRY_POINTS::kModCommandedActorLimit, visitor);

    return baseLimit;
}

RE::BSEventNotifyControl SpellCastEventHandler::ProcessEvent(const RE::TESSpellCastEvent* event,
                                                             RE::BSTEventSource<RE::TESSpellCastEvent>* /*source*/) {
    if (!event || !event->object || !event->spell) {
        return RE::BSEventNotifyControl::kContinue;
    }

    logger::debug("SpellCastEventHandler: ProcessEvent called with event: {:#010x}", event->spell);

    if (!event->object->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::Actor* playerActor = event->object->As<RE::Actor>();
    if (!playerActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::FormID formID = event->spell;

    RE::TESForm* form = RE::TESForm::LookupByID(formID);
    if (!form) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::SpellItem* spellItem = form->As<RE::SpellItem>();
    if (!spellItem) {
        return RE::BSEventNotifyControl::kContinue;
    }

    bool isShout = false;

    if (!Config::generalRule.shoutsEnabled || !Config::generalRule.spellsEnabled) {
        logger::debug("Shouts are enabled in the general rule.");
        auto it = Config::GetShoutSpellMap().find(spellItem->GetFormID());
        if (it != Config::GetShoutSpellMap().end()) {
            RE::TESShout* shoutFound = it->second;
            const char* shoutName = shoutFound->GetName();
            logger::debug("Spell is part of shout:");
            logger::debug("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
            logger::debug("  Shout FormID: {:#010x}", shoutFound->GetFormID());

            isShout = true;
        }
    }

    if (!Config::generalRule.shoutsEnabled && isShout) {
        logger::debug("Shouts are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!Config::generalRule.spellsEnabled && !isShout) {
        logger::debug("Spells are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }

    const char* spellName = spellItem->GetName();
    logger::debug("Player casting spell:");
    logger::debug("  Name: {}", spellName);
    logger::debug("  FormID: {:#010x}", spellItem->GetFormID());


    logger::info("istoggled {}", Config::toggleKeyHeld);

    // --- Summon Check ---
    // --- Check Max Summon Limit ---
    // RE::ActorValue avSummonLimit = RE::ActorValue::kSummonCreatureLimit;
    // float currentMaxSummons = playerActor->AsActorValueOwner()->GetActorValue(avSummonLimit);

    // ActorValues are floats, but this limit is practically an integer.
    // int maxSummonsInt = static_cast<int>(currentMaxSummons);
    int maxSummonsInt = GetPlayerSummonLimit1();

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
    }

    logger::debug("Player's current maximum summon limit: {}", maxSummonsInt);

    bool isSummonSpell = false;
    if (spellItem->effects.empty()) {
        logger::debug("Spell {} has no effects.", spellItem->GetName());
    } else {
        logger::debug("Checking effects for spell {} ({:#010x}):", spellItem->GetName(), spellItem->GetFormID());
        for (auto* effect : spellItem->effects) {
            if (effect && effect->baseEffect) {
                auto* mgef = effect->baseEffect;
                RE::EffectSetting::Archetype archetype = mgef->data.archetype;
                const char* mgefName = mgef->GetName();
                if (!mgefName || mgefName[0] == '\0') mgefName = "Unnamed MGEF";

                logger::trace("  - Effect: {} ({:#010x}), Archetype: {}", mgefName, mgef->GetFormID(),
                              static_cast<int>(archetype));  // Log archetype value

                // Check if the archetype is Summon Creature
                if (archetype == RE::EffectSetting::Archetype::kSummonCreature) {
                    logger::debug("    Found Summon Creature effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                    isSummonSpell = true;
                    break;
                }
                // Optional: Also check for Reanimate if you consider that summoning
                else if (archetype == RE::EffectSetting::Archetype::kReanimate) {
                   logger::debug("    Found Reanimate effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                   isSummonSpell = true; // Or use a different flag if needed
                   break;
                } else if (archetype == RE::EffectSetting::Archetype::kCommandSummoned) {
                    logger::debug("    Found Command Summon effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                    isSummonSpell = true;
                    break;
                }
            } else {
                logger::warn("  - Found null effect or null baseEffect in spell {:#010x}", spellItem->GetFormID());
            }
        }
    }

    if (isSummonSpell) {
        logger::info("Detected Summon Spell: {} ({:#010x})", spellItem->GetName(), spellItem->GetFormID());
        return RE::BSEventNotifyControl::kContinue;
        // --- Add any specific logic for summon spells here ---
        // For example, maybe you want to skip further processing?
        // return RE::BSEventNotifyControl::kContinue;
        // Or maybe you want to pass this info along?
        // info.isSummon = true; // (Would need to add 'isSummon' to SpellCastInfo)
        // -------------------------------------------------------
    } else {
        logger::debug("Spell {} ({:#010x}) is NOT detected as a summon spell.", spellItem->GetName(),
                      spellItem->GetFormID());
    }
    // --- End Summon Check ---

    LogAllActiveEffectsOfSpell(spellItem);  // Log all active effects of the spell
    // --- Log MGEF Keywords ---
    LogKeywords(spellItem, "      ");  // Pass mgef and appropriate indent

    if (spellItem->data.castingType == RE::MagicSystem::CastingType::kConcentration) {
        if (!spellName || spellName[0] == '\0') spellName = "Unnamed Spell";
        logger::debug("Player casting concentration spell '{}' ({:#010x}). Skipping further checks.", spellName,
                      spellItem->GetFormID());
        return RE::BSEventNotifyControl::kContinue;  // Exit early for concentration spells
    }

    auto magicTarget = playerActor->GetMagicTarget();
    if (!magicTarget) {
        logger::error("Failed to get MagicTarget from player actor.");
        return RE::BSEventNotifyControl::kContinue;
    }
    auto activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects) {
        logger::error("Failed to get active effects list from MagicTarget.");
        return RE::BSEventNotifyControl::kContinue;
    }

    bool alreadyOnPlayer = false;
    for (auto* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        if (activeEffect->spell->GetFormID() == spellItem->GetFormID() && activeEffect->elapsedSeconds > 0) {
            logger::debug("Spell {} is already active. Will dispell next frame",
                          activeEffect->GetBaseObject()->GetName());
            alreadyOnPlayer = true;
            break;
        }
    }

    RE::ActorHandle playerHandle = playerActor->GetHandle();

    // Package the data
    SpellCastInfo info{*spellItem, playerHandle, alreadyOnPlayer};

    // Schedule the CheckAppliedEffects function to run on the next UI update cycle
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        taskInterface->AddUITask([info]() {  // capture 'info'
            ConvertToPermanentEffectOnPlayer(info);
        });
    } else {
        logger::error("Failed to get TaskInterface, cannot schedule effect check.");
    }

    return RE::BSEventNotifyControl::kContinue;
}

void SpellCastEventHandler::Register() {
    auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (eventSource) {
        eventSource->AddEventSink<RE::TESSpellCastEvent>(&GetSingleton());
    }
}
