#include "MagicApplyEventHandler.h"

RE::BSEventNotifyControl MagicApplyEventHandler::ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* source) {
    if (!event || !event->magicEffect || !event->caster) {
        logger::info("MagicApplyEventHandler: Invalid arguments.");
        return RE::BSEventNotifyControl::kContinue;
    }
    
    if (!event->target->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::Actor* playerActor = RE::PlayerCharacter::GetSingleton()->As<RE::Actor>();
    if (!playerActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    logger::info("MagicApplyEventHandler: ProcessEvent called with event: {:#010x}", event->magicEffect);
    LogAllActiveEffectsOnActor(*playerActor);

    RE::ActorHandle playerHandle = playerActor->GetHandle();

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = SpellUtilities::GetActiveEffectsOnPlayer();
    if (!activeEffects) {
        return RE::BSEventNotifyControl::kContinue;
    }
    
    if(event->target) {
        logger::info("Target Name: '{}'", event->target->GetName());
        logger::info("Target FormID: '{:#010x}'", event->target->GetFormID());
    }
    if(event->caster) {
        logger::info("Caster Name: '{}'", event->caster->GetName());
        logger::info("Caster FormID: '{:#010x}'", event->caster->GetFormID());
    }

    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        if (activeEffect->spell->GetFormID() == event->magicEffect) {
            logger::info("MagicApplyEventHandler: Spell already found.");
            logger::info("Spell Name: '{}'", activeEffect->GetBaseObject()->GetName());
            logger::info("Spell FormID: '{:#010x}'", activeEffect->GetBaseObject()->GetFormID());
            logger::info("Spell Name: '{}'", activeEffect->spell->GetName());
            if(activeEffect->caster == playerHandle) {
                logger::info("Caster is the player.");
            }
        }
    }

    

    return RE::BSEventNotifyControl::kContinue;
    // return RE::BSEventNotifyControl();
}

void MagicApplyEventHandler::Register() {
    auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (eventSource) {
        eventSource->AddEventSink<RE::TESMagicEffectApplyEvent>(&GetSingleton());
    }
}
