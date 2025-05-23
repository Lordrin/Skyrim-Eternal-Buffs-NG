#include "SpellUtilities.h"

namespace SpellUtilities {

    bool IsSummon(RE::SpellItem* spellItem) {
        if (!spellItem) {
            logger::warn("IsSummon: SpellItem is null.");
            return false;
        }

        bool isSummon = false;
        if (spellItem->effects.empty()) {
            logger::debug("Spell {} has no effects.", spellItem->GetName());
        } else {
            logger::debug("Checking effects for spellItem {} ({:#010x}):", spellItem->GetName(),
                          spellItem->GetFormID());
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
                            logger::debug("    Found Summon Creature effect: {} ({:#010x})", mgefName,
                                          mgef->GetFormID());
                            isSummon = true;
                            break;

                        case RE::EffectSetting::Archetype::kReanimate:
                            logger::debug("    Found Reanimate effect: {} ({:#010x})", mgefName, mgef->GetFormID());
                            isSummon = true;
                            break;

                        case RE::EffectSetting::Archetype::kCommandSummoned:
                            logger::debug("    Found Command Summon effect: {} ({:#010x})", mgefName,
                                          mgef->GetFormID());
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
        return spellItem->GetFormType() == RE::FormType::Spell &&
               spellItem->data.spellType == RE::MagicSystem::SpellType::kSpell;
    }

    bool IsScroll(RE::SpellItem* spellItem) {
        if (!spellItem) {
            logger::warn("IsScroll: SpellItem is null.");
            return false;
        }
        return spellItem->data.spellType == RE::MagicSystem::SpellType::kScroll ||
               spellItem->data.castingType == RE::MagicSystem::CastingType::kScroll;
    }

    bool IsConcentration(RE::SpellItem* spellItem) {
        if (!spellItem) {
            logger::warn("IsConcentration: SpellItem is null.");
            return false;
        }
        return spellItem->data.castingType == RE::MagicSystem::CastingType::kConcentration;
    }

    bool IsNonRecastable(RE::SpellItem* spellItem) {
        if (!spellItem) {
            logger::warn("isRecastable: SpellItem is null.");
            return false;
        }
        for (auto effect : spellItem->effects) {
            if (effect->baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kNoRecast)) {
                return true;
            }
        }
        return false;
    }

    bool IsNotCastOnSelf(RE::SpellItem* spellItem) {
        if (!spellItem) {
            logger::warn("IsCastOnSelf: SpellItem is null.");
            return false;
        }
        // if(IsSummon(spellItem)) return false;
        return spellItem->data.delivery != RE::MagicSystem::Delivery::kSelf;
    }

    bool IsTemporaryEffect(RE::ActiveEffect* activeEffect) {
        if (!activeEffect) {
            logger::warn("IsTemporaryEffect: ActiveEffect is null.");
            return false;
        }
        return activeEffect->duration > 0.0f &&
               activeEffect->duration < Config::GetSingleton().GetPermanentSpellDuration();
    }

    std::string GetSpellSourcePluginName(RE::SpellItem* spellItem) {
        if (!spellItem) {
            SKSE::log::warn("GetSpellSourcePluginName: spellItem is null.");
            return "[Error: Null Spell]";
        }

        // --- Method 1: Preferred - Using TESForm::GetFile() ---
        // This is the most straightforward and usually the best way.
        const RE::TESFile* sourceFileFromForm = spellItem->GetFile(0);
        if (sourceFileFromForm) {
            return std::string(sourceFileFromForm->GetFilename());  // Or sourceFileFromForm->fileName
        }
        // If GetFile(0) returns null, it could be a dynamic form or an issue.
        // Manual lookup as a fallback.
        SKSE::log::warn(
            "GetSpellSourcePluginName: spellItem->GetFile(0) returned null for FormID {:08X}. Attempting manual "
            "lookup.",
            spellItem->GetFormID());

        // --- Method 2: Manual Lookup using DataHandler and FormID components ---
        RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error("GetSpellSourcePluginName (Manual): TESDataHandler is null.");
            return "[Error: No DataHandler]";
        }

        RE::FormID fullFormID = spellItem->GetFormID();
        uint8_t compileIndexByte = static_cast<uint8_t>(fullFormID >> 24);  // This is the 00-FF byte

        const RE::TESFile* foundFile = nullptr;

        if (compileIndexByte == 0xFE) {
            // It's a light plugin. Extract the 12-bit light slot index.
            uint16_t lightSlotIndex = static_cast<uint16_t>((fullFormID >> 12) & 0x0FFF);
            foundFile = dataHandler->LookupLoadedLightModByIndex(lightSlotIndex);
            if (!foundFile) {
                SKSE::log::warn(
                    "GetSpellSourcePluginName (Manual): LookupLoadedLightModByIndex for light index {:03X} (FormID "
                    "{:08X}) returned null.",
                    lightSlotIndex, fullFormID);
                return "[Error: Unknown Light Plugin]";
            }
        } else if (compileIndexByte == 0xFF) {
            // Dynamic/temporary form
            SKSE::log::info("GetSpellSourcePluginName (Manual): FormID {:08X} is dynamic (index FF).", fullFormID);
            return "[Dynamic Form]";
        } else {
            // Regular plugin (00-FD)
            foundFile = dataHandler->LookupLoadedModByIndex(compileIndexByte);
            if (!foundFile) {
                SKSE::log::warn(
                    "GetSpellSourcePluginName (Manual): LookupLoadedModByIndex for index {:02X} (FormID {:08X}) "
                    "returned null.",
                    compileIndexByte, fullFormID);
                return "[Error: Unknown Regular Plugin]";
            }
        }

        if (foundFile) {
            return std::string(foundFile->GetFilename());  // Or foundFile->fileName
        }

        // Should ideally not be reached if logic is correct and GetFile(0) also failed.
        SKSE::log::error(
            "GetSpellSourcePluginName (Manual): Failed to identify plugin for FormID {:08X} through all manual checks.",
            fullFormID);
        return "[Error: Unidentified Plugin Source]";
    }

}
