#include "SpellLogging.h"

/**
 * @brief Logs details about all spells currently stored in the SpellDataPersistence runtime map.
 * @warning This function performs potentially numerous FormID lookups. Calling it frequently
 *          (e.g., on every event in a high-frequency handler) can impact performance.
 *          Use primarily for debugging or infrequent checks.
 */
// void LogAllSavedSpellData() {
//     logger::info("--- Logging All Spell Data from SpellDataPersistence Runtime Map ---");
//     const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();
//     LogSpellSFromMap(savedSpells);
// }

void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent) {
    if (!keywordForm) return;

    uint32_t numKeywords = keywordForm->GetNumKeywords();
    if (numKeywords > 0) {
        logger::info("{}Keywords:", indent);
        for (uint32_t i = 0; i < numKeywords; ++i) {
            std::optional<RE::BGSKeyword*> optKeyword = keywordForm->GetKeywordAt(i);
            if (optKeyword) {
                RE::BGSKeyword* keyword = *optKeyword;
                const char* keywordStr =
                    keyword && keyword->GetFormEditorID() ? keyword->GetFormEditorID() : "Unnamed Keyword";
                logger::info("{}- {}", indent + "  ", keywordStr);
            } else {
                logger::warn("{}- (Optional keyword was empty for index {})", indent + "  ", i);
            }
        }
    }
}

void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem) {
    if (!spellItem) {
        return;
    }

    if (spellItem->effects.empty()) {
        logger::warn("No active effects found for spell: {}", spellItem->GetName());
        return;
    }
    int effectCount = 0;
    logger::info("       - Associated Effect IDs: {} ({})", spellItem->GetName(), spellItem->effects.size());
    for (RE::Effect* effect : spellItem->effects) {
        if (effect && effect->baseEffect) {
            effectCount++;
            logger::info("        {}. Effect ID: {:#010x} ('{}')", effectCount, effect->baseEffect->GetFormID(), effect->baseEffect->GetName());
        }
    }
}

void LogAllActiveEffectsOnActor(RE::Actor& actor) {
    if (actor.IsDead()) {
        logger::warn("Actor is dead. Cannot log active effects.");
        return;
    }

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

    int activeEffectsCount = 0; 
    logger::info("Active Effects on Actor {}:", actor.GetName());
    // Iterate over the active effects and check for matches in the set
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }
        ++activeEffectsCount;

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        logger::info("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

        // Spell associated with the active effect
        RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
        if (spellItem) {
            logger::info("  - Active effect associated with spell: {:#010x} - {}", spellItem->GetFormID(), spellItem->GetName());
        } else {
            // Check if the active effect is associated with a perk or ability
            RE::TESForm* sourceForm = activeEffect->spell;
            if (sourceForm) {
                if (auto* perk = sourceForm->As<RE::BGSPerk>()) {
                    logger::info("  - Active effect associated with perk: {:#010x} - {}", perk->GetFormID(), perk->GetName());
                } else if (auto* ability = sourceForm->As<RE::SpellItem>()) {
                    logger::info("  - Active effect associated with ability: {:#010x} - {}", ability->GetFormID(), ability->GetName());
                } else {
                    logger::warn("    - No associated spell, perk, or ability found for active effect: {:#010x}", effectFormID);
                }
            } else {
                logger::warn("    - No source form found for active effect: {:#010x}", effectFormID);
            }
        }
    }
    logger::info("active effects size: {}", activeEffectsCount);
    logger::info("Finished logging active effects on Actor {}.", actor.GetName());
    logger::info("--------------------------------------------------");
}