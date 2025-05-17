#include "SpellApplication.h"

#include "SpellUtilities.h"

void ApplyTemporaryDebuffToPlayer(float magickaCost) {
    RE::FormID reserveEffectFormID = Config::GetSingleton().GetReserveEffectFormID();
    if (reserveEffectFormID == 0) {
        logger::warn("ReserveEffectFormID is not set. Returning early.");
        return;
    }

    auto datahandler = RE::TESDataHandler::GetSingleton();
    if (!datahandler) {
        SKSE::log::error("DataHandler is null.");
    }

    auto form = datahandler->LookupForm(reserveEffectFormID, "EternalBuffsNG.esp");
    if (!form) {
        SKSE::log::error("Custom spell was not found with FormID {:#010x}.", reserveEffectFormID);
        return;
    }

    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("Player not found for temporary debuff application.");
        return;
    }

    // Get the player's MagicCaster component for the desired hand (e.g., kRightHand)
    // Or if the spell is 'Self' delivery, a 'self' caster might be more appropriate,
    // but often using a hand caster works well for Fire and Forget spells.
    RE::MagicCaster* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
    if (!magicCaster) {
        // Try other hand or a default caster if right hand fails
        magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
        if (!magicCaster) {
            SKSE::log::warn("Could not get a valid MagicCaster for the player.");
            return;
        }
    }

    // auto effects = spellToApply->effects;
    // if (effects.empty()) {
    //     SKSE::log::warn("Spell {} has no effects.", spellToApply->GetName());
    //     return;
    // }

    // // Get the first effect in the spell
    // auto* effect = effects[0];
    // if (!effect || !effect->baseEffect) {
    //     SKSE::log::warn("Effect for spell {} is null.", spellToApply->GetName());
    //     return;
    // }

    // // Get the MGEF from the effect
    // auto* mgef = effect->baseEffect;
    // if (!mgef) {
    //     SKSE::log::warn("MGEF for effect of spell {} is null.", spellToApply->GetName());
    //     return;
    // }

    // Create a dynamic spell instance
    RE::ConcreteFormFactory<RE::SpellItem, RE::FormType::Spell>* formFactory =
        RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();

    RE::SpellItem* dynamicCarrierSpell = nullptr;
    if (formFactory) {
        dynamicCarrierSpell = formFactory->Create();  // This gets an FFxxxxxx FormID
    } else {
        SKSE::log::error("Failed to get spell factory!");
        return;  // or handle error
    }

    if (!dynamicCarrierSpell) {
        SKSE::log::error("Failed to create dynamic carrier spell instance!");
        return;  // or handle error
    }

    // Configure this dynamic spell:
    dynamicCarrierSpell->data.spellType = RE::MagicSystem::SpellType::kSpell;  // Or kLesserPower, etc.
    dynamicCarrierSpell->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
    dynamicCarrierSpell->data.delivery = RE::MagicSystem::Delivery::kSelf;
    dynamicCarrierSpell->fullName = RE::BSFixedString("DynamicCarrierInstance_XYZ");
    RE::EffectSetting* customMagicEffect = form->As<RE::EffectSetting>();
    if (!customMagicEffect) {
        SKSE::log::error("Spell is null.");
    }

    logger::info("Effect123: {}", customMagicEffect->fullName);

    // Create a new RE::Effect instance.
    // IMPORTANT: Memory management for this RE::Effect object is crucial.
    // If the spell takes ownership, great. If not, you might need to manage it.
    // Often, when added to the spell's list and the spell is used, the game manages it.
    RE::Effect* newEffectItem =
        new RE::Effect();  // Allocate on the heap // The RE::SpellItem destructor iterates through its effects
    // array and deletes each RE::Effect* it contains.
    if (!newEffectItem) {
        SKSE::log::error("Failed to allocate RE::Effect item!");
        // Potentially delete dynamicSpell if it's not yet fully integrated
        delete newEffectItem;
        delete dynamicCarrierSpell;
        return;
    }
    try {
        // TODO delete newEffectItem on error

        // Set the effect's parameters for this spell
        newEffectItem->effectItem.magnitude = magickaCost * -1.0f;  // Example: Reduce max Magicka by 50
        newEffectItem->effectItem.duration =
            static_cast<uint32_t>(Config::GetSingleton().GetPermanentSpellDuration());  // Example: 60 seconds
        newEffectItem->effectItem.area = 0;             // Example: 0 area for a self-target effect
        newEffectItem->baseEffect = customMagicEffect;  // ** This is where you link your MGEF **

        dynamicCarrierSpell->effects.push_back(newEffectItem);
        // float cost = dynamicCarrierSpell->CalculateMagickaCost(player);

        magicCaster->CastSpellImmediate(dynamicCarrierSpell, true, player, 1.0f, false,
                                        newEffectItem->effectItem.magnitude, nullptr);
    } catch (std::exception& e) {
        delete newEffectItem;
        delete dynamicCarrierSpell;
        SKSE::log::error("Failed to apply spell: {}", e.what());
    }

    SKSE::log::info("Attempted to apply temporary debuff spell to player.");
}

/**
 * @brief Applies configuration rules to an active effect.
 * @param activeEffect The active effect to apply rules to.
 * @return True if the rules were applied successfully, false otherwise.
 * @note This function checks if the active effect's spell matches any rules in the configuration.
 */
bool ApplyConfigRulesToActiveEffect(RE::ActiveEffect* activeEffect, const std::string& pluginName) {
    if (!activeEffect) {
        logger::warn("ApplyConfigRulesToActiveEffect: activeEffect is null");
        return false;
    }

    SpellRule spellRule;
    // The more specific rule is applied first and overrides the less specific rule
    if (FindSpellRuleForActiveEffect(activeEffect, spellRule)) {
        logger::debug("ApplyConfigRulesToSpell: Found rule for active effect: {:#010x} ({}) from spell: {:#010x} ({})",
                      activeEffect->GetBaseObject()->GetFormID(), activeEffect->GetBaseObject()->GetName(),
                      activeEffect->spell->GetFormID(), activeEffect->spell->GetName());
        spellRule.ApplySpellRulesToActiveEffect(activeEffect);
        return true;
    } else if (!pluginName.empty() && FindSpellRuleForSpellByPluginName(pluginName, spellRule)) {
        logger::debug("ApplyConfigRulesToSpell: Found rule for plugin: {}", pluginName);
        spellRule.ApplySpellRulesToActiveEffect(activeEffect);
        return true;
    } else {
        logger::debug("ApplyConfigRulesToSpell: Spell {} not found in rules.", activeEffect->spell->GetName());
        return false;
    }
}

void DispellAllSavedSpellsFromActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        logger::warn("DispellAllSavedSpellsFromActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        logger::info("DispellAllSavedSpellsFromActor: Actor has no active effects.");
        return;
    }
    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        // Check if the effect is in the saved spell data
        if (flattenedSpellData.find(effectFormID) == flattenedSpellData.end()) {
            continue;
        }

        RE::FormID linkedSpellFormId = activeEffect->spell->GetFormID();
        // If the effect is linked to a saved spell - Reset duration
        if (savedSpells.find(linkedSpellFormId) != savedSpells.end()) {
            logger::debug("Found active effect with form ID: {:#010x}. Dispel it.", effectFormID);
            activeEffect->Dispel(false);  // Remove the effect from the actor
        } else {
            logger::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                         activeEffect->spell->GetName());
        }
    }
}

void DispellAllSavedSpellsFromPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    DispellAllSavedSpellsFromActor(*player);
}

void ApplyAllSavedSpellsToActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        logger::warn("ApplyAllSavedSpellsToActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        logger::info("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    logger::debug("-------------------Active Effects on Player on load complete.-------------------");

    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);
    SpellEffectsMap AllSavedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> appliedSpellsIDs;

    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        logger::trace("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

        // Check if the effect is in the saved spell data
        if (flattenedSpellData.find(effectFormID) == flattenedSpellData.end()) {
            continue;
        }

        RE::FormID linkedSpellFormId = activeEffect->spell->GetFormID();
        RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
        if (spellItem) {
            std::string pluginName = GetSpellSourcePluginName(spellItem);
            bool isRuleApplied =
                ApplyConfigRulesToActiveEffect(activeEffect, pluginName);  // Apply config rules to the spell
            if (isRuleApplied) {
                appliedSpellsIDs.insert(linkedSpellFormId);  // Add to the list of applied spells
                logger::debug("ApplyConfigRulesToSpell: Spell rules applied to active effect: {:#010x} - {}. Spell: {}",
                              effectFormID, activeEffect->GetBaseObject()->GetName(), spellItem->GetName());
                continue;  // Skip to the next effect if rules were applied successfully
            }
        } else {
            logger::warn("ApplyConfigRulesToSpell: MagicItem is not a SpellItem.");
        }

        // If the effect is linked to a saved spell - Reset duration
        if (AllSavedSpells.find(linkedSpellFormId) != AllSavedSpells.end()) {
            logger::debug("Found active effect with form ID: {:#010x}. Resetting duration.", effectFormID);
            activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();  // Set to permanent duration
            activeEffect->elapsedSeconds = 0.0f;                                          // Reset elapsed time
            // activeEffect->GetBaseObject()->magicItemDescription = "This is a test";
            appliedSpellsIDs.insert(linkedSpellFormId);  // Add to the list of applied spells

        } else {
            logger::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                         activeEffect->spell->GetName());
        }
    }

    // If not all saved spells were applied, then clean the saved spells of the ones not found.
    if (appliedSpellsIDs.size() != AllSavedSpells.size()) {
        logger::debug("Not all saved spells were applied to the player. {} out of {} spells applied.",
                      appliedSpellsIDs.size(), AllSavedSpells.size());
        std::vector<RE::FormID> notAppliedSpellsIDs;  // Create a copy of the vector

        // Iterate over the keys in the map
        for (const auto& [key, value] : AllSavedSpells) {
            // Check if the key is not in the set
            if (appliedSpellsIDs.find(key) == appliedSpellsIDs.end()) {
                notAppliedSpellsIDs.push_back(key);  // Add the missing key to the result
            }
        }

        for (const auto& formId : notAppliedSpellsIDs) {
            logger::debug("Spell ({:#010x}) was not applied to the player. Removing from save.", formId);
            SpellDataPersistence::RemoveSpellFromSave(formId);  // Remove the spell from the save
        }
    } else {
        logger::debug("All saved spells were applied to the player.");
    }

    logger::info("Finished applying permanent spells to player.");
}

void ApplyAllSavedPermanentSpellsToPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    ApplyAllSavedSpellsToActor(*player);
}

void HandleSavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo, const char* spellName,
                      bool isSummonSpell, const std::string& pluginName) {
    // --- Initial Setup ---
    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    if (!mgef) {
        logger::error("HandleSavedSpell: ActiveEffect has no base MGEF!");
        return;
    }
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";  // Use placeholder if name is missing
    }

    RE::SpellItem* spellItem = castInfo.spellItem;
    if (!spellItem) {
        logger::error("HandleSavedSpell: castInfo contains null spellItem!");
        return;
    }

    SpellRule spellRule = GetSpellRuleForActiveEffect(activeEffect);  // Get rules specific to this effect

    // --- Determine Required Action ---
    SpellHandlingAction finalAction;
    bool shouldConsiderDispel = castInfo.alreadyOnPlayer;  // Start with whether it was saved previously

    // Specific logic override for Summon Spells based on toggle key
    if (isSummonSpell) {
        bool keyHeld = Config::GetSingleton().GetToggleKeyHeld();
        if (keyHeld) {
            // If it's a summon and the key is held, we INTEND to dispel it, overriding 'alreadyOnPlayer'.
            // The original loop checking effect->baseEffect seems overly complex if we assume
            // activeEffect is indeed an instance of an effect from spellItem.
            logger::debug("Summon spell '{}' ({:#010x}) and toggle key Held. Marking for potential dispel.", spellName,
                          spellItem->GetFormID());
            shouldConsiderDispel = true;
        } else {
            // If it's a summon and the key is NOT held, we explicitly DO NOT want to dispel it.
            logger::debug("Summon spell '{}' ({:#010x}) and toggle key Not Held. Marking to keep.", spellName,
                          spellItem->GetFormID());
            shouldConsiderDispel = false;
        }
    }

    // Final decision: Can we actually dispel based on the rule?
    if (shouldConsiderDispel && spellRule.toggleable) {
        // We want to dispel (either originally on player or summon+key) AND the rule allows toggling.
        finalAction = SpellHandlingAction::kDispel;
        logger::debug("Determined Action for '{}': Dispel (Toggleable: {}, Initially Considered Dispel: {})", spellName,
                      spellRule.toggleable, shouldConsiderDispel);
    } else {
        // We either didn't want to dispel initially, OR we wanted to but the rule prevents toggling it off.
        finalAction = SpellHandlingAction::kApplyConfig;
        if (shouldConsiderDispel && !spellRule.toggleable) {
            logger::debug(
                "Determined Action for '{}': Apply Config (Toggleable: {}, Initially Considered Dispel: {}, Rule "
                "prevents toggle off)",
                spellName, spellRule.toggleable, shouldConsiderDispel);
        } else {
            logger::debug("Determined Action for '{}': Apply Config (Toggleable: {}, Initially Considered Dispel: {})",
                          spellName, spellRule.toggleable, shouldConsiderDispel);
        }
    }

    // --- Execute Action ---
    bool appliedConfig = false;
    switch (finalAction) {
        case SpellHandlingAction::kDispel:
            logger::debug("Executing Dispel for spell '{}' ({:#010x}).", spellName, spellItem->GetFormID());
            activeEffect->Dispel(false);  // false = No Hit Effects/Sound
            // TODO remove reserved mana spell
            SpellDataPersistence::RemoveSpellFromSave(spellItem->GetFormID());
            logger::debug("Spell '{}' ({:#010x}) dispelled and removed from save.", spellName, spellItem->GetFormID());
            break;

        case SpellHandlingAction::kApplyConfig:
            logger::debug("Executing Apply Config / Keep for spell '{}' ({:#010x}).", spellName,
                          spellItem->GetFormID());
            // Try applying specific rules first
            appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect, pluginName);
            // If no specific rule applied, set the default permanent duration
            if (!appliedConfig) {
                activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();
                logger::debug("No specific config rule applied, setting default duration {} for '{}'.",
                              activeEffect->duration, spellName);
                auto cost = activeEffect->spell->CalculateMagickaCost(RE::PlayerCharacter::GetSingleton());
                ApplyTemporaryDebuffToPlayer(cost);
            } else {
                logger::debug("Specific config rule applied to '{}'.", spellName);
            }
            break;

        default:
            logger::debug("Unhandled SpellHandlingAction for spell '{}'", spellName);
            break;
    }
}

void HandleUnsavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo, const std::string& pluginName) {
    bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect, pluginName);
    if (!appliedConfig && IsTemporaryEffect(activeEffect)) {
        activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();
        auto cost = activeEffect->spell->CalculateMagickaCost(RE::PlayerCharacter::GetSingleton());
        ApplyTemporaryDebuffToPlayer(cost);
    }
    SpellDataPersistence::CacheSpellForSaving(castInfo.spellItem);
}

void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo) {
    if (!castInfo.playerHandle) {
        logger::warn("CheckAppliedEffects: Player handle is null.");
        return;
    }

    RE::Actor* player = castInfo.playerHandle.get().get();
    if (!player) {
        logger::warn("CheckAppliedEffects: Player handle invalid on next frame.");
        return;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects) {
        logger::warn("CheckAppliedEffects: Player has no ActiveEffects list.");
        return;
    }

    RE::SpellItem* spellItem = castInfo.spellItem;

    if (!spellItem) {
        logger::warn("CheckAppliedEffects: SpellItem became null during the UI update.");
        return;
    }

    RE::PlayerCharacter* playerCharacter = RE::PlayerCharacter::GetSingleton();
    if (!playerCharacter) {
        logger::warn("CheckAppliedEffects: PlayerCharacter handle invalid.");
        return;
    }

    const char* spellName = spellItem->GetName();
    if (!spellName || spellName[0] == '\0') {
        spellName = "Unnamed Spell";
    }

    logger::debug("Checking applied effects for spell '{}' ({:#010x}) cast last frame...", spellName,
                  spellItem->GetFormID());

    bool isSummonSpell = IsSummonSpell(spellItem);  // Separate check from the Onprocess check
    bool isCastOnSelfSpell = !IsNotCastOnSelf(spellItem);

    if (!isSummonSpell && !isCastOnSelfSpell) {
        logger::debug("Spell '{}' ({:#010x}) is not a summon or cast on self spell. Returning Early", spellName,
                      spellItem->GetFormID());
        return;
    }

    std::string pluginName = GetSpellSourcePluginName(spellItem);
    logger::debug("Spell '{}' ({:#010x}) has plugin '{}'", spellName, spellItem->GetFormID(), pluginName);

    bool hasPluginRule = false;

    auto it = Config::GetSingleton().GetSpellRules().find(pluginName);

    if (it != Config::GetSingleton().GetSpellRules().end()) {
        logger::debug("Spell '{}' ({:#010x}) has a rule for plugin '{}'", spellName, spellItem->GetFormID(),
                      pluginName);
        hasPluginRule = true;
    }

    bool isSpellSaved = SpellDataPersistence::IsSpellSaved(spellItem->GetFormID());
    if (isSpellSaved) {
        logger::debug("Spell '{}' ({:#010x}) is saved.", spellName, spellItem->GetFormID());
    }

    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() ||
            !activeEffect->target || !activeEffect->caster) {
            continue;
        }

        if (activeEffect->spell->GetFormID() == spellItem->GetFormID() &&
            activeEffect->caster == castInfo.playerHandle && activeEffect->target->MagicTargetIsActor() &&
            activeEffect->target == player->GetMagicTarget()) {
            LogActiveEffectDetails(activeEffect);

            if (activeEffect->GetBaseObject()->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kNoRecast)) {
                logger::debug("Spell '{}' ({:#010x}) is not recastable.", spellName, spellItem->GetFormID());
            }

            if (activeEffect->GetBaseObject()->data.flags.any(
                    RE::EffectSetting::EffectSettingData::Flag::kNoDuration)) {
                logger::debug("Spell '{}' ({:#010x}) has no duration.", spellName, spellItem->GetFormID());
                // continue;
            }

            if (isSpellSaved) {
                HandleSavedSpell(activeEffect, castInfo, spellName, isSummonSpell, pluginName);
            } else {
                HandleUnsavedSpell(activeEffect, castInfo, pluginName);
            }
        }
    }
}