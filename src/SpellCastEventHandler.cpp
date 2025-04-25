#include "SpellCastEventHandler.h"

#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"

/**
 * @brief Applies the active effects associated with a given spell to the player character.
 *
 * This function simulates casting the spell on the player, causing the game's magic system
 * to create and manage the corresponding ActiveEffect instance(s) on the player.
 * This is generally safer and more robust than manually creating ActiveEffect objects.
 *
 * Note: This works best for spells intended to be cast on 'Self' or Ability-type spells.
 * The casting source is set to kSelf. Casting might fail based on game conditions
 * (resistances, effect conditions, etc.).
 *
 * @param a_spell A pointer to the SpellItem whose effects should be applied. Must not be null.
 */
void ApplySpellEffectsToPlayer(RE::SpellItem* a_spell)
{
    // 1. Validate the input spell
    if (!a_spell) {
        SKSE::log::warn("ApplySpellEffectsToPlayer: Received null spell pointer.");
        // Or use _DMESSAGE, _MESSAGE etc. depending on your logging setup
        return;
    }

    // 2. Get the Player Character singleton
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("ApplySpellEffectsToPlayer: Could not get PlayerCharacter singleton.");
        return;
    }

    // 3. Get a MagicCaster instance from the player.
    //    Using kSelf is appropriate for applying effects directly to the player,
    //    as if it were an innate ability or a self-targeted cast.
    RE::MagicCaster* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
    if (!magicCaster) {
        SKSE::log::error("ApplySpellEffectsToPlayer: Failed to get MagicCaster(kSelf) for player.");
        return;
    }

    magicCaster->CastSpellImmediate(a_spell, true, player, 100.0f, true, false, nullptr);

    // 4. Cast the spell *from* the player *onto* the player.
    //    Parameters for Cast(MagicItem*, isDualCasting, target, effectiveness, isConcentration, magnitudeOverride, originator):
    //    - a_spell: The spell to cast.
    //    - false: Not dual casting.
    //    - player: The target of the spell.
    //    - 1.0f: Default effectiveness (optional, depends on specific Cast overload).
    //    - false: Not a concentration spell (usually; the system might handle this based on spell type).
    //    We use a simpler overload here if available: Cast(SpellItem*, bool bIsDualCasting, Actor* target)
    
    // magicCaster->Cast(a_spell,    // The spell containing the effect(s)
    //                   false,      // Not dual casting
    //                   player);    // Target is the player themselves

    // Optional: Log that the cast attempt was made
    SKSE::log::info("ApplySpellEffectsToPlayer: Attempted to cast spell '{}' (FormID: {:X}) on player.", a_spell->GetName(), a_spell->GetFormID());

    // Note: The actual application of the ActiveEffect happens internally within the Cast function
    // and subsequent game updates. Success isn't guaranteed (e.g., conditions on the MGEF).
    // You could check player->GetActiveEffectList() afterwards if needed, but that's more complex.
}

SpellCastEventHandler::SpellCastEventHandler() {
    RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
    RE::FormID reserveMagickaFormID = 0x00D64;
    RE::FormID reserveMagickaEffectFormID = 0x00D63;
    _reserveMagickaForm = dataHandler->LookupForm(reserveMagickaFormID, _pluginName);
    _reserveMagickaEffectForm = dataHandler->LookupForm(reserveMagickaEffectFormID, _pluginName);

    logger::info("reserve magicka spell found {}", _reserveMagickaForm->GetName());
    logger::info("reserve magicka effect spell found {}", _reserveMagickaEffectForm->GetName());

    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    RE::SpellItem* spell = _reserveMagickaForm->As<RE::SpellItem>();
    ApplySpellEffectsToPlayer(spell);
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

    if (!Global::generalRule.shoutsEnabled || !Global::generalRule.spellsEnabled) {
        logger::debug("Shouts are enabled in the general rule.");
        auto it = Global::GetShoutSpellMap().find(spellItem->GetFormID());
        if (it != Global::GetShoutSpellMap().end()) {
            RE::TESShout* shoutFound = it->second;
            const char* shoutName = shoutFound->GetName();
            logger::debug("Spell is part of shout:");
            logger::debug("  Shout Name: {}", shoutName ? shoutName : "Unnamed Shout");
            logger::debug("  Shout FormID: {:#010x}", shoutFound->GetFormID());

            isShout = true;
        }
    }

    if (!Global::generalRule.shoutsEnabled && isShout) {
        logger::debug("Shouts are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }
    if (!Global::generalRule.spellsEnabled && !isShout) {
        logger::debug("Spells are disabled in the general rule.");
        return RE::BSEventNotifyControl::kContinue;
    }

    const char* spellName = spellItem->GetName();
    logger::debug("Player casting spell:");
    logger::debug("  Name: {}", spellName);
    logger::debug("  FormID: {:#010x}", spellItem->GetFormID());

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
