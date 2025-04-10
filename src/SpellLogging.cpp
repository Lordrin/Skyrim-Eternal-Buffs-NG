#include "SpellLogging.h"

void LogSpellSFromMap(const SpellEffectsMap& spellEffectsMap) {
    if (spellEffectsMap.empty()) {
        SKSE::log::info(" SpellEffectsMap map is currently empty. No data loaded or cached.");
        SKSE::log::info("--- Finished Logging Spell Data ---");
        return;
    }

    SKSE::log::info("  Found data for {} spells:", spellEffectsMap.size());

    int spellCount = 0;
    for (const auto& [spellID, effectIDs] : spellEffectsMap) {
        spellCount++;
        RE::TESForm* spellForm = RE::TESForm::LookupByID(spellID);
        RE::SpellItem* spellItem = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
        std::string spellName = spellItem && spellItem->GetName() ? spellItem->GetName() : "Unknown/Lookup Failed";

        SKSE::log::info("  {}. Spell ID: {:#010x} ('{}')", spellCount, spellID, spellName);

        if (effectIDs.empty()) {
            SKSE::log::info("      - No associated effect IDs recorded.");
        } else {
            SKSE::log::info("      - Associated Effect IDs ({}):", effectIDs.size());
            int effectCount = 0;
            for (RE::FormID effectID : effectIDs) {
                effectCount++;
                RE::EffectSetting* mgef = RE::TESForm::LookupByID<RE::EffectSetting>(effectID);
                std::string effectName = mgef && mgef->GetName() ? mgef->GetName() : "Unknown/Lookup Failed";
                SKSE::log::info("        {}. Effect ID: {:#010x} ('{}')", effectCount, effectID, effectName);
            }
        }
    }

    SKSE::log::info("--- Finished Logging Spell Data ({} spells processed) ---", spellEffectsMap.size());
}

/**
 * @brief Logs details about all spells currently stored in the SpellDataPersistence runtime map.
 * @warning This function performs potentially numerous FormID lookups. Calling it frequently
 *          (e.g., on every event in a high-frequency handler) can impact performance.
 *          Use primarily for debugging or infrequent checks.
 */
// void LogAllSavedSpellData() {
//     SKSE::log::info("--- Logging All Spell Data from SpellDataPersistence Runtime Map ---");
//     const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();
//     LogSpellSFromMap(savedSpells);
// }

void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent) {
    if (!keywordForm) return;

    uint32_t numKeywords = keywordForm->GetNumKeywords();
    if (numKeywords > 0) {
        SKSE::log::info("{}Keywords:", indent);
        for (uint32_t i = 0; i < numKeywords; ++i) {
            std::optional<RE::BGSKeyword*> optKeyword = keywordForm->GetKeywordAt(i);
            if (optKeyword) {
                RE::BGSKeyword* keyword = *optKeyword;
                const char* keywordStr =
                    keyword && keyword->GetFormEditorID() ? keyword->GetFormEditorID() : "Unnamed Keyword";
                SKSE::log::info("{}- {}", indent + "  ", keywordStr);
            } else {
                SKSE::log::warn("{}- (Optional keyword was empty for index {})", indent + "  ", i);
            }
        }
    }
}

void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem) {
    if (!spellItem) {
        return;
    }

    if (spellItem->effects.empty()) {
        SKSE::log::warn("No active effects found for spell: {}", spellItem->GetName());
        return;
    }
    int effectCount = 0;
    SKSE::log::info("       - Associated Effect IDs: {} ({})", spellItem->GetName(), spellItem->effects.size());
    for (RE::Effect* effect : spellItem->effects) {
        if (effect && effect->baseEffect) {
            effectCount++;
            SKSE::log::info("        {}. Effect ID: {:#010x} ('{}')", effectCount, effect->baseEffect->GetFormID(), effect->baseEffect->GetName());
        }
    }
}

void LogAllActiveEffectsOnActor(RE::Actor& actor) {
    if (actor.IsDead()) {
        SKSE::log::warn("Actor is dead. Cannot log active effects.");
        return;
    }

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        SKSE::log::warn("ApplyAllSavedSpellsToActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        SKSE::log::info("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    int activeEffectsCount = 0; 
    logger::info("Active Effects on Actor {}:", actor.GetName());
    // Iterate over the active effects and check for matches in the set
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }
        ++activeEffectsCount;

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        SKSE::log::info("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());
        //spell acosiated with the active effect
        RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
        if (spellItem) {
            logger::info("  - Active effect associated with spell: {:#010x} - {}", spellItem->GetFormID(), spellItem->GetName());
        } else {
            SKSE::log::warn("    - No associated spell found for active effect: {:#010x}", effectFormID);
        }
    }
    logger::info("active effects size: {}", activeEffectsCount);
    logger::info("Finished logging active effects on Actor {}.", actor.GetName());
    SKSE::log::info("--------------------------------------------------");
}