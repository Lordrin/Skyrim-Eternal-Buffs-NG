#include "SpellLogging.h"

void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }
    
    if (!keywordForm) return;

    uint32_t numKeywords = keywordForm->GetNumKeywords();
    if (numKeywords > 0) {
        logger::debug("{}Keywords:", indent);
        for (uint32_t i = 0; i < numKeywords; ++i) {
            std::optional<RE::BGSKeyword*> optKeyword = keywordForm->GetKeywordAt(i);
            if (optKeyword) {
                RE::BGSKeyword* keyword = *optKeyword;
                const char* keywordStr =
                    keyword && keyword->GetFormEditorID() ? keyword->GetFormEditorID() : "Unnamed Keyword";
                logger::debug("{}- {}", indent + "  ", keywordStr);
            } else {
                logger::warn("{}- (Optional keyword was empty for index {})", indent + "  ", i);
            }
        }
    }
}

void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }

    if (!spellItem) {
        return;
    }

    if (spellItem->effects.empty()) {
        logger::warn("No active effects found for spell: {}", spellItem->GetName());
        return;
    }
    int effectCount = 0;
    logger::debug("       - Associated Effect IDs: {} ({})", spellItem->GetName(), spellItem->effects.size());
    for (RE::Effect* effect : spellItem->effects) {
        if (effect && effect->baseEffect) {
            effectCount++;
            logger::debug("        {}. Effect ID: {:#010x} ('{}')", effectCount, effect->baseEffect->GetFormID(),
                          effect->baseEffect->GetName());
        }
    }
}

void LogAllActiveEffectsOnActor(RE::Actor& actor) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }

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
        logger::debug("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    int activeEffectsCount = 0;
    logger::debug("Active Effects on Actor {}:", actor.GetName());
    // Iterate over the active effects and check for matches in the set
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }
        ++activeEffectsCount;

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        logger::trace("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

        // Spell associated with the active effect
        RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
        if (spellItem) {
            logger::debug("  - Active effect associated with spell: {:#010x} - {}", spellItem->GetFormID(),
                          spellItem->GetName());
        } else {
            // Check if the active effect is associated with a perk or ability
            RE::TESForm* sourceForm = activeEffect->spell;
            if (sourceForm) {
                if (auto* perk = sourceForm->As<RE::BGSPerk>()) {
                    logger::debug("  - Active effect associated with perk: {:#010x} - {}", perk->GetFormID(),
                                  perk->GetName());
                } else if (auto* ability = sourceForm->As<RE::SpellItem>()) {
                    logger::debug("  - Active effect associated with ability: {:#010x} - {}", ability->GetFormID(),
                                  ability->GetName());
                } else {
                    logger::warn("    - No associated spell, perk, or ability found for active effect: {:#010x}",
                                 effectFormID);
                }
            } else {
                logger::warn("    - No source form found for active effect: {:#010x}", effectFormID);
            }
        }
    }
    logger::debug("active effects size: {}", activeEffectsCount);
    logger::debug("Finished logging active effects on Actor {}.", actor.GetName());
    logger::debug("--------------------------------------------------");
}