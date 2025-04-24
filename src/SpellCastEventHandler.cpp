#include "SpellCastEventHandler.h"

#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"

SpellCastEventHandler::SpellCastEventHandler() : reserveSPellId_(0) {
    RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
    uint32_t localFormID = 0x00801;
    RE::TESForm* localForm = dataHandler->LookupForm(localFormID, "BlinkTeleport.esp");
    logger::info("Local form ID of blink lesser power: {}", localForm->formID);
    reserveSPellId_ = localForm->formID;
}

RE::BSEventNotifyControl SpellCastEventHandler::ProcessEvent(const RE::TESSpellCastEvent* event,
                                                             RE::BSTEventSource<RE::TESSpellCastEvent>* /*source*/) {
    // Basic Event Checks
    if (!event || !event->object || !event->spell) {
        return RE::BSEventNotifyControl::kContinue;
    }

    logger::info("SpellCastEventHandler: ProcessEvent called with event: {:#010x}", event->spell);

    // Check if Caster is Player
    if (!event->object->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Get Player Actor (needed for handle later)
    RE::Actor* playerActor = event->object->As<RE::Actor>();
    if (!playerActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // --- Get Spell Item ---
    RE::FormID formID = event->spell;

    RE::TESForm* form = RE::TESForm::LookupByID(formID);
    if (!form) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::SpellItem* spellItem = form->As<RE::SpellItem>();  // RE::TESForm::LookupByID<RE::SpellItem>(form);
    if (!spellItem) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Start timing
    auto start = std::chrono::high_resolution_clock::now();

    bool isShout = false;

    if (!GBL::generalRule.shoutsEnabled || !GBL::generalRule.spellsEnabled) {
        logger::info("Shouts are enabled in the general rule.");
        auto it = GBL::GetShoutSpellMap().find(spellItem->GetFormID());
        if (it != GBL::GetShoutSpellMap().end()) {
            RE::TESShout* shoutFound = it->second;
            const char* shoutName = shoutFound->GetName();
            logger::info("Spell is part of shout:");
            logger::info("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
            logger::info("  Shout FormID: {:#010x}", shoutFound->GetFormID());

            isShout = true;

            
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            
            // Log the time taken
            logger::info("Time taken to check shouts: {} microseconds", duration);
            
            // return RE::BSEventNotifyControl::kContinue;
        }
    }
    // float originalShoutRecoveryMult = playerActor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kShoutRecoveryMult);
    // logger::info("Original shout recovery multiplier: {}", originalShoutRecoveryMult);
    // playerActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kShoutRecoveryMult, 1.0f);

    if (!GBL::generalRule.shoutsEnabled && isShout) {
        logger::info("Shouts are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!GBL::generalRule.spellsEnabled && !isShout) {
        logger::info("Spells are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }

    // Check if the spell is part of a shout
    // auto dataHandler = RE::TESDataHandler::GetSingleton();
    // for (auto* shout : dataHandler->GetFormArray<RE::TESShout>()) {
    //     if (!shout) {
    //         continue;
    //     }

    //     for (auto& word : shout->variations) {
    //         if (word.spell) {
    //             RE::SpellItem* associatedSpell = word.spell;
    //             logger::info("  Associated Spell Name: {}", associatedSpell->GetName());
    //             logger::info("  Associated Spell FormID: {:#010x}", associatedSpell->GetFormID());

    //             if (associatedSpell->GetFormID() == spellItem->GetFormID()) {
    //                 const char* shoutName = shout->GetName();
    //                 logger::info("Spell is part of shout:");
    //                 logger::info("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
    //                 logger::info("  Shout FormID: {:#010x}", shout->GetFormID());
    //                 // End timing
    //                 auto end = std::chrono::high_resolution_clock::now();
    //                 auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    //                 // Log the time taken
    //                 logger::info("Time taken to check shouts: {} microseconds", duration);
    //                 return RE::BSEventNotifyControl::kContinue;
    //             }
    //         }
    //         // if (word && word->spell && word->spell->GetFormID() == spellItem->GetFormID()) {
    //         //     const char* shoutName = shout->GetName();
    //         //     logger::info("Spell is part of shout:");
    //         //     logger::info("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
    //         //     logger::info("  Shout FormID: {:#010x}", shout->GetFormID());
    //         //     return RE::BSEventNotifyControl::kContinue;
    //         // }
    //     }
    // }

    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Log the time taken
    logger::info("Time taken to check shouts: {} microseconds", duration);

    const char* spellName = spellItem->GetName();
    logger::info("Player casting spell:");
    logger::info("  Name: {}", spellName);
    logger::info("  FormID: {:#010x}", spellItem->GetFormID());  // Log resolved FormID

    LogAllActiveEffectsOfSpell(spellItem);  // Log all active effects of the spell
    // --- Log MGEF Keywords ---
    LogKeywords(spellItem, "      ");  // Pass mgef and appropriate indent

    // --- NEW: Check for Concentration Spell ---
    if (spellItem->data.castingType == RE::MagicSystem::CastingType::kConcentration) {
        if (!spellName || spellName[0] == '\0') spellName = "Unnamed Spell";
        logger::info("Player casting concentration spell '{}' ({:#010x}). Skipping further checks.", spellName,
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
            logger::info("Spell {} is already active. Will dispell next frame",
                            activeEffect->GetBaseObject()->GetName());
            alreadyOnPlayer = true;
            break;
        }
    }

    // spellItem->effects[0]->effectItem.duration = 0;  // Set duration to 0 for the first effect
    // auto magicItem = form->As<RE::MagicItem>();
    // if (!magicItem) {
    //     logger::error("Failed to cast spell: MagicItem is null.");
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // magicItem.eff
    // spellItem->effects[0]->baseEffect->data.
    // --- End of Concentration Check ---
    RE::ActorHandle playerHandle = playerActor->GetHandle();

    // Package the data
    SpellCastInfo info{*spellItem, playerHandle, alreadyOnPlayer};

    // Schedule the CheckAppliedEffects function to run on the next UI update cycle
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        taskInterface->AddUITask([info]() {  // Use lambda to capture 'info'
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
