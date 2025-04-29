#include "SpellCastEventHandler.h"

#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"

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

    if (!Config::GetSingleton().GetGeneralRule().shoutsEnabled || !Config::GetSingleton().GetGeneralRule().spellsEnabled) {
        logger::debug("Shouts are enabled in the general rule.");
        auto it = Config::GetSingleton().GetShoutSpellMap().find(spellItem->GetFormID());
        if (it != Config::GetSingleton().GetShoutSpellMap().end()) {
            RE::TESShout* shoutFound = it->second;
            const char* shoutName = shoutFound->GetName();
            logger::debug("Spell is part of shout:");
            logger::debug("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
            logger::debug("  Shout FormID: {:#010x}", shoutFound->GetFormID());

            isShout = true;
        }
    }

    if (Config::GetSingleton().GetGeneralRule().shoutsEnabled && isShout) {
        logger::debug("Shouts are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!Config::GetSingleton().GetGeneralRule().spellsEnabled && !isShout) {
        logger::debug("Spells are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }

    const char* spellName = spellItem->GetName();
    logger::debug("Player casting spell:");
    logger::debug("  Name: {}", spellName);
    logger::debug("  FormID: {:#010x}", spellItem->GetFormID());


    logger::info("istoggled {}", Config::GetSingleton().GetToggleKeyHeld());

    if (Config::GetSingleton().GetToggleKeyHeld()) {
        logger::info("istoggled {} and is not running {}", Config::GetSingleton().GetToggleKeyHeld(), !playerActor->IsRunning());
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
    }

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
