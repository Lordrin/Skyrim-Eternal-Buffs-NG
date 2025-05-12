#include "SpellCastEventHandler.h"

#include "ConfigRules.h"
#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"
#include "SpellUtilities.h"

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

    LogSpellItemDetails(spellItem);

    if (!IsSpellRuleDefined(spellItem)) {  // Rule defined for this specific spell
        const auto& checks = Config::GetSingleton().GetGeneralRule().checks;
        logger::debug("No rule defined for this spell.");

        // Iterate through the checks
        for (const auto& check : checks) {
            bool isEnabled = false;
            if(check.isEnabledConfig) {
                isEnabled = *check.isEnabledConfig;
            }
            // If the category is disabled AND the spell matches the check function
            if (!isEnabled && check.checkFn(spellItem)) {
                logger::debug("{} are disabled. Skipping spell: {}", check.description, spellItem->GetName());
                return RE::BSEventNotifyControl::kContinue;  // Skip processing this spell
            }
        }
    }

    if (Config::GetSingleton().GetToggleKeyHeld()) {
        logger::info("istoggled {} and is not running {}", Config::GetSingleton().GetToggleKeyHeld(),
                     !playerActor->IsRunning());
    }

    LogAllActiveEffectsOfSpell(spellItem);  // Log all active effects of the spell
    LogKeywords(spellItem, "      ");       // Pass mgef and appropriate indent

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
            // Check if this spell should be ignored
            SpellRule spellRule;
            if (GetSpellRuleForActiveEffect(activeEffect, spellRule)) {
                if (!spellRule.isPermanentEnabled && !spellRule.toggleable) {
                    return RE::BSEventNotifyControl::kContinue;
                }
            }
            break;
        }
    }

    RE::ActorHandle playerHandle = playerActor->GetHandle();

    // Package the data
    SpellCastInfo info{spellItem, playerHandle, alreadyOnPlayer};

    // Schedule the ConvertToPermanentEffectOnPlayer function to run on the next UI update cycle
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        taskInterface->AddUITask([info]() { ConvertToPermanentEffectOnPlayer(info); });
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
