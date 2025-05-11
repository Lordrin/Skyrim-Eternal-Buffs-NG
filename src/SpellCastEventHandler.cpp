#include "SpellCastEventHandler.h"

#include "SpellApplication.h"
#include "SpellDataPersistence.h"
#include "SpellLogging.h"
#include "SpellUtilities.h"
#include "ConfigRules.h"

// // Define the static structure (only function and description)
// struct SpellCheckDefinition {
//     std::function<bool(RE::SpellItem*)> checkFn;
//     std::string_view description;
// };

// // Define the list of check definitions ONCE (static or global)
// // This is efficient - created only once.
// static const std::vector<SpellCheckDefinition> g_spellCheckDefinitions = {
//     { IsShout,         "Shouts" },
//     { IsLesserPower,   "Lesser Powers" },
//     { IsGreaterPower,  "Greater Powers" },
//     { IsSummon,        "Summons" },
//     { IsSpell,         "Spells" },
//     { IsScroll,        "Scrolls" },
//     { IsConcentration, "Concentration spells" },
//     { [](RE::SpellItem* si) {
//           return si && (si->data.flags & RE::SpellItem::SpellFlag::kFoodItem);
//       }, "Food items" }
// };

// // Map the definitions to the config getter function pointers (or similar mechanism)
// // This MUST correspond ORDER-WISE to g_spellCheckDefinitions
// // Assumes Config::GeneralRule is the struct type returned by GetGeneralRule()
// using ConfigRuleGetter = bool(Config::GeneralRule::*)() const; // Example using pointer-to-member function
// // OR if they are public members:
// using ConfigRuleMember = bool Config::GeneralRule::*;

// static const std::array<ConfigRuleMember, g_spellCheckDefinitions.size()> g_configRuleMapping = {
//     &Config::GeneralRule::shoutsEnabled,       // Maps to IsShout
//     &Config::GeneralRule::lesserPowersEnabled, // Maps to IsLesserPower
//     &Config::GeneralRule::greaterPowersEnabled,// Maps to IsGreaterPower
//     &Config::GeneralRule::summonsEnabled,      // Maps to IsSummon
//     &Config::GeneralRule::spellsEnabled,       // Maps to IsSpell
//     &Config::GeneralRule::scrollsEnabled,      // Maps to IsScroll
//     nullptr, // No specific config flag for Concentration (always check if true)
//     nullptr  // No specific flag for Food Item (always check if true)
// };

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

    // auto& config = Config::GetSingleton().GetGeneralRule();

    logger::debug("Spell Type: {}", static_cast<int>(spellItem->data.spellType));
    logger::debug("Spell Delivery: {}", static_cast<int>(spellItem->data.delivery));
    logger::debug("Spell Casting Type: {}", static_cast<int>(spellItem->data.castingType));

    // // Create a list of checks to perform
    // static const std::vector<SpellDisableCheck> checks = {
    //     {!Config::GetSingleton().GetGeneralRule().shoutsEnabled, IsShout, "Shouts"},
    //     {!Config::GetSingleton().GetGeneralRule().lesserPowersEnabled, IsLesserPower, "Lesser Powers"},
    //     {!Config::GetSingleton().GetGeneralRule().greaterPowersEnabled, IsGreaterPower, "Greater Powers"},
    //     {!Config::GetSingleton().GetGeneralRule().summonsEnabled, IsSummon, "Summons"},
    //     {!Config::GetSingleton().GetGeneralRule().spellsEnabled, IsSpell,
    //      "Spells"},  // Make sure IsSpell correctly identifies *only* regular spells if needed
    //     {!Config::GetSingleton().GetGeneralRule().scrollsEnabled, IsScroll, "Scrolls"},
    //     {true, IsConcentration, "Concentration spells"},  // Always disabled if concentration
    //     {true,
    //      [](RE::SpellItem* si) {  // Lambda for flags
    //          return si && (si->data.flags & RE::SpellItem::SpellFlag::kFoodItem);
    //      },
    //      "Food items"}  // Always disabled if food flag is set
    // };

    const auto checks = Config::GetSingleton().GetGeneralRule().checks;

    // Iterate through the checks
    for (const auto& check : checks) {
        // If the category is disabled AND the spell matches the check function
        if (check.isDisabledInConfig && check.checkFn(spellItem)) {
            logger::debug("{} are disabled. Skipping spell: {}", check.description, spellItem->GetName());
            return RE::BSEventNotifyControl::kContinue;  // Skip processing this spell
        }
    }

    // if (!config.shoutsEnabled && IsShout(spellItem)) {
    //     logger::debug("Shouts are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if (!config.lesserPowersEnabled && IsLesserPower(spellItem)) {
    //     logger::debug("Lesser Powers are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if (!config.greaterPowersEnabled && IsGreaterPower(spellItem)) {
    //     logger::debug("Greater Powers are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if (!config.summonsEnabled && IsSummon(spellItem)) {
    //     logger::debug("Summons are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if (!config.spellsEnabled && IsSpell(spellItem)) {
    //     logger::debug("Spells are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if(!config.scrollsEnabled && IsScroll(spellItem)) {
    //     logger::debug("Scrolls are disabled in the general rule. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if(IsConcentration(spellItem)) {
    //     logger::debug("Concentrations are disabled. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }
    // if(spellItem->data.flags & RE::SpellItem::SpellFlag::kFoodItem) {
    //     logger::debug("Food Items are disabled. {}", spellItem->GetName());
    //     return RE::BSEventNotifyControl::kContinue;
    // }

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
