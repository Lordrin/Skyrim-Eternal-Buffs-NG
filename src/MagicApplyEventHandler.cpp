#include "MagicApplyEventHandler.h"

RE::BSEventNotifyControl MagicApplyEventHandler::ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* _source) {
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
    // compare active effects to reserved spells
    std::unordered_map<RE::FormID, ReserveMagicka>& reservedSpells = Config::GetSingleton().GetReservedSpells();

    std::vector<RE::ActiveEffect *> activeEffectsFromReservedSpells;

    for(const auto& [formID, reserveSpell] : reservedSpells) {
        std::vector<RE::ActiveEffect *> activeEffectsFromReservedSpells2 = SpellUtilities::GetActiveEffectsOnActorFromSpellItem(playerActor, reserveSpell.spellItem);
        activeEffectsFromReservedSpells.insert(activeEffectsFromReservedSpells.end(), activeEffectsFromReservedSpells2.begin(), activeEffectsFromReservedSpells2.end());
    }

    // check if the they are inactive
    SpellUtilities::SeparatedEffects separatedEffects = SpellUtilities::GetSeparatedActiveEffects(&activeEffectsFromReservedSpells);

    std::vector<RE::ActiveEffect *> inactiveEffects = separatedEffects.inactive;
    std::vector<RE::ActiveEffect *> a_activeEffects = separatedEffects.active;


    if (inactiveEffects.size() > 0) {
        logger::info("MagicApplyEventHandler: {} inactive effects found.", inactiveEffects.size());
        // get reservedd spell by formID
    }

    if(a_activeEffects.size() > 0) {
        logger::info("MagicApplyEventHandler: {} active effects found.", a_activeEffects.size());
    }


    // Put the associated reserve spell to inactive/active


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
