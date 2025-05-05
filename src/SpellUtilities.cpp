#include "SpellUtilities.h"

bool IsSummon(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsSummon: SpellItem is null.");
        return false;
    }

    bool isSummon = false;
    if (spellItem->effects.empty()) {
        logger::debug("Spell {} has no effects.", spellItem->GetName());
    } else {
        logger::debug("Checking effects for spellItem {} ({:#010x}):", spellItem->GetName(), spellItem->GetFormID());
        for (auto* effect : spellItem->effects) {
            if (effect && effect->baseEffect) {
                auto* mgef = effect->baseEffect;
                RE::EffectSetting::Archetype archetype = mgef->data.archetype;
                const char* mgefName = mgef->GetName();
                if (!mgefName || mgefName[0] == '\0') mgefName = "Unnamed MGEF";

                logger::trace("  - Effect: {} ({:#010x}), Archetype: {}", mgefName, mgef->GetFormID(),
                              static_cast<int>(archetype));  // Log archetype value

                switch (archetype) {
                    case RE::EffectSetting::Archetype::kSummonCreature:
                        logger::debug("    Found Summon Creature effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                        isSummon = true;
                        break;

                    case RE::EffectSetting::Archetype::kReanimate:
                        logger::debug("    Found Reanimate effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                        isSummon = true;
                        break;

                    case RE::EffectSetting::Archetype::kCommandSummoned:
                        logger::debug("    Found Command Summon effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                        isSummon = true;
                        break;

                    default:
                        break;
                }
            } else {
                logger::warn("  - Found null effect or null baseEffect in spell {:#010x}", spellItem->GetFormID());
            }
        }
    }

    return isSummon;
}

bool IsSummonSpell(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsSummonSpell: SpellItem is null.");
        return false;
    }
    if (spellItem->GetFormType() != RE::FormType::Spell) {
        logger::warn("IsSummonSpell: SpellItem is not a spell.");
        return false;
    }
    return IsSummon(spellItem);
}

bool IsLesserPower(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsLesserPower: SpellItem is null.");
        return false;
    }
    return spellItem->data.spellType == RE::MagicSystem::SpellType::kLesserPower;
}

bool IsGreaterPower(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsLesserPower: SpellItem is null.");
        return false;
    }
    return spellItem->data.spellType == RE::MagicSystem::SpellType::kPower;
}

bool IsShout(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsShout2: SpellItem is null.");
        return false;
    }
    return spellItem->data.spellType == RE::MagicSystem::SpellType::kVoicePower;
}

bool IsSpell(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsSpell: SpellItem is null.");
        return false;
    }
    return spellItem->GetFormType() == RE::FormType::Spell && spellItem->data.spellType == RE::MagicSystem::SpellType::kSpell;
}

bool IsScroll(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsScroll: SpellItem is null.");
        return false;
    }
    return spellItem->data.spellType == RE::MagicSystem::SpellType::kScroll || spellItem->data.castingType == RE::MagicSystem::CastingType::kScroll;
}

bool IsConcentration(RE::SpellItem* spellItem) {
    if (!spellItem) {
        logger::warn("IsConcentration: SpellItem is null.");
        return false;
    }
    return spellItem->data.castingType == RE::MagicSystem::CastingType::kConcentration;
}