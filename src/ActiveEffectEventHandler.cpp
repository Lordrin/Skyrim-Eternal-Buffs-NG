#include "ActiveEffectEventHandler.h"

// Helper function to find the ActiveEffect instance
RE::ActiveEffect* GetActiveEffectByUniqueID(RE::Actor* a_targetActor, uint32_t a_uniqueID)
{
    if (!a_targetActor || a_uniqueID == 0) { // 0 is often an invalid ID
        return nullptr;
    }

    auto* magicTarget = a_targetActor->GetMagicTarget();
    if (!magicTarget) {
        // This shouldn't happen for actors, but good to check
        SKSE::log::warn("GetActiveEffectByUniqueID: Actor '{}' ({:#010x}) has no MagicTarget.",
                        a_targetActor->GetName(), a_targetActor->GetFormID());
        return nullptr;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();

    // Iterate through the actor's active effects
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (activeEffect && activeEffect->usUniqueID == a_uniqueID) {
            // Found it!
            return activeEffect;
        }
    }

    // Not found (might happen if the effect was removed just before the event processed, especially for 'remove' events)
    // SKSE::log::trace("GetActiveEffectByUniqueID: Could not find ActiveEffect with UniqueID {} on Actor '{}'.",
    //                 a_uniqueID, a_targetActor->GetName()); // Optional trace
    return nullptr;
}

RE::BSEventNotifyControl ActiveEffectEventHandler::ProcessEvent(const RE::TESActiveEffectApplyRemoveEvent* event,
                                                                RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* source) {
    // if (!event || !event->activeEffectUniqueID || !event->caster) {
    //     return RE::BSEventNotifyControl::kContinue;
    // }

    // Log basic information about the event
    SKSE::log::info("ActiveEffectEventHandler: ProcessEvent called.");
    SKSE::log::info("  Caster: {}", event->caster->GetName());
    
    RE::Actor* targetActor = event->target ? event->target->As<RE::Actor>() : nullptr;
    RE::ActiveEffect* activeEffect = GetActiveEffectByUniqueID(targetActor, event->activeEffectUniqueID);

    LogActiveEffectDetails(activeEffect);

    // Check if the caster is the player
    if (event->caster->IsPlayerRef() && event->target->IsPlayerRef()) {
        SKSE::log::info("  The active effect was applied/removed on the player.");

        // Log details about the active effect
        // LogActiveEffectDetails(event->activeEffect);

        // Example: Handle the effect (e.g., apply custom rules or modify the effect)
        // ApplyConfigRulesToActiveEffect(event->activeEffect);
    }

    return RE::BSEventNotifyControl::kContinue;
}

void ActiveEffectEventHandler::Register() {
    auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (eventSource) {
        eventSource->AddEventSink<RE::TESActiveEffectApplyRemoveEvent>(&GetSingleton());
        SKSE::log::info("ActiveEffectEventHandler registered successfully.");
    } else {
        SKSE::log::error("Failed to register ActiveEffectEventHandler: Event source is null.");
    }
}