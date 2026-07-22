#include "SpellLogging.h"

#include <vector>

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

void LogAllSpellsOnActor(RE::Actor& actor) {
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

    std::map<RE::FormID, RE::SpellItem*> activeSpells;
    logger::debug("Active Spells on Actor {}:", actor.GetName());
    // Iterate over the active effects and check for matches in the set
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        // Check if the effect is "Inactive" (Suppressed)
        bool isInactive = activeEffect->flags.all(RE::ActiveEffect::Flag::kInactive);
        
        // Check if it's been dispelled (waiting to be deleted)
        bool isDispelled = activeEffect->flags.all(RE::ActiveEffect::Flag::kDispelled);

        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() || isInactive || isDispelled) {
            continue;
        }

        RE::FormID spellFormID = activeEffect->spell->GetFormID();
        if (activeSpells.find(spellFormID) == activeSpells.end()) {
            activeSpells.insert({spellFormID, activeEffect->spell->As<RE::SpellItem>()});
        }
    }
    logger::info("Active Spells on Actor {}:", actor.GetName());
    // iterate over map and log spell
    int spellCount = 0;
    for (auto& [spellFormID, spellItem] : activeSpells) {
        ++spellCount;
        logger::debug("  {}. Spell ID: {:#010x} ('{}')", spellCount, spellFormID, spellItem->GetName());
    }
}



void CheckEffectStatus(RE::Actor& actor) {
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

    for (auto* effect : *activeEffects) {
        if (!effect) continue;

        // Get the base spell/magic item associated with this effect
        auto* magicItem = effect->spell;
        if (!magicItem) continue;

        // Check if the effect is "Inactive" (Suppressed)
        bool isInactive = effect->flags.all(RE::ActiveEffect::Flag::kInactive);
        
        // Check if it's been dispelled (waiting to be deleted)
        bool isDispelled = effect->flags.all(RE::ActiveEffect::Flag::kDispelled);

        if (isInactive) {
            SKSE::log::info("Spell: '{}' is CURRENTLY SUPPRESSED (Lower rank or overridden)", magicItem->GetName());
        } else if (isDispelled) {
            SKSE::log::info("Spell: '{}' is DISPELLED", magicItem->GetName());
        } else {
            SKSE::log::info("Spell: '{}' is ACTIVE and EFFECTIVE", magicItem->GetName());
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
                    logger::warn("    - No associated spell, perk, found for active effect: {:#010x} - {:#010x}",
                                 effectFormID, sourceForm->GetFormID());
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

void LogSpellItemDetails(RE::SpellItem* spellItem, const std::string& indent, const std::string& prefix) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }

    if (!spellItem) {
        return;
    }

    logger::debug("{}", indent + prefix);
    logger::debug("{}- Spell Name: {}", indent, spellItem->GetName());
    logger::debug("{}- Spell FormID: {:#010x}", indent, spellItem->GetFormID());
    logger::debug("{}  Spell Type: {}", indent, static_cast<int>(spellItem->data.spellType));
    logger::debug("{}  Spell Delivery: {}", indent, static_cast<int>(spellItem->data.delivery));
    logger::debug("{}  Spell Casting Type: {}", indent, static_cast<int>(spellItem->data.castingType));
    logger::debug("{}  Spell is non-recastable: {}", indent, SpellUtilities::IsNonRecastable(spellItem));
    for (auto effect : spellItem->effects) {
        logger::debug("{}  Spell Effect flags: {}", indent, SpellUtilities::ToString(effect->baseEffect->data.flags));
        }
    
#ifdef _DEBUG
    logger::debug("{}  Spell Type: {}", indent, SpellUtilities::ToString(spellItem->data.spellType));
    logger::debug("{}  Spell Delivery: {}", indent, SpellUtilities::ToString(spellItem->data.delivery));
    logger::debug("{}  Spell Casting Type: {}", indent, SpellUtilities::ToString(spellItem->data.castingType));
    logger::debug("{}  Spell flags: {}", indent, SpellUtilities::ToString(spellItem->data.flags));
#endif  // DEBUG
}

// Function to log active effects for debugging
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }
    if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
        return;
    }

    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    RE::Effect* spellEffectEntry = activeEffect->effect;
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";
    }

    logger::debug("  -> Applied Effect Found:");
    logger::debug("      Name: {}", mgefName);
    logger::debug("      MGEF ID: {:#010x}", mgef->GetFormID());
    logger::debug("      Spell Duration: {}", spellEffectEntry->effectItem.duration);
    logger::debug("      Spell Magnitude: {}", spellEffectEntry->effectItem.magnitude);
    logger::debug("      Spell Area: {}", spellEffectEntry->effectItem.area);
    logger::debug("      Active Duration (Remaining): {:.2f}", activeEffect->duration);
    logger::debug("      Active Magnitude: {:.2f}", activeEffect->magnitude);
    logger::debug("      Elapsed Time: {:.2f}s", activeEffect->elapsedSeconds);
}