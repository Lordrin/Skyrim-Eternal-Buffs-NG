#include "SpellCastEventHandler.h"
#include "SpellUtilities.h"
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

    auto& config = Config::GetSingleton().GetGeneralRule();

    logger::debug("Spell Type: {}", static_cast<int>(spellItem->data.spellType));
    logger::debug("Spell Delivery: {}", static_cast<int>(spellItem->data.delivery));
    logger::debug("Spell Casting Type: {}", static_cast<int>(spellItem->data.castingType));
    
    if (!config.shoutsEnabled && IsShout(spellItem)) {
        logger::debug("Shouts are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!config.lesserPowersEnabled && IsLesserPower(spellItem)) {
        logger::debug("Lesser Powers are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!config.greaterPowersEnabled && IsGreaterPower(spellItem)) {
        logger::debug("Greater Powers are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!config.summonsEnabled && IsSummon(spellItem)) {
        logger::debug("Summons are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!config.spellsEnabled && IsSpell(spellItem)) {
        logger::debug("Spells are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if(!config.scrollsEnabled && IsScroll(spellItem)) {
        logger::debug("Scrolls are disabled in the general rule. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if(IsConcentration(spellItem)) {
        logger::debug("Concentrations are disabled. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }
    if(spellItem->data.flags & RE::SpellItem::SpellFlag::kFoodItem) {
        logger::debug("Food Items are disabled. {}", spellItem->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }

    const char* spellName = spellItem->GetName();
    logger::debug("Player casting spell:");
    logger::debug("  Name: {}", spellName);
    logger::debug("  FormID: {:#010x}", spellItem->GetFormID());
    logger::debug("  istoggled key held: {}", Config::GetSingleton().GetToggleKeyHeld());

    if (Config::GetSingleton().GetToggleKeyHeld()) {
        logger::info("istoggled {} and is not running {}", Config::GetSingleton().GetToggleKeyHeld(),
                     !playerActor->IsRunning());
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
    }

    LogAllActiveEffectsOfSpell(spellItem);  // Log all active effects of the spell
    LogKeywords(spellItem, "      ");  // Pass mgef and appropriate indent

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
                if(!spellRule.isPermanentEnabled && !spellRule.toggleable) {
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
        taskInterface->AddUITask([info]() {
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
