#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <variant>

#include "OrderedMap.h"
#include "SpellUtilities.h"
#include "StringUtilities.h"

using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*, float*>;

enum class RuleIdentifierType { kInvalid = -1, kName = 0, kForm = 1, kPlugin = 2 };

struct RuleIdentifierResult {
    RuleIdentifierType identifierType;
    // RE::TESForm* formResult;
    // std::string result;
    std::variant<std::string, RE::TESForm*> variantResult;
};

struct BaseRule {
    std::string sourceFile;  // Not handled by parser
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;    // Handled by resolvedForm parser
    std::string pluginFilter;  // Handled by resolvedForm parser
    bool isPermanentEnabled = true;
    bool toggleable = true;
    std::vector<std::string> keywordFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool IsCorrectRuleToForm(RE::TESForm* form) const;
    std::string ToString() const;
};

struct SpellRule : BaseRule {
    float durationFilter = -1.0f;
    uint32_t minDurationFilter = 0;
    float magnitudeFilter = -1.0f;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool IsCorrectRuleToSpell(RE::SpellItem* spellItem) const;
    void ApplySpellRulesToActiveEffect(RE::ActiveEffect* activeEffect) const;
    std::string ToString() const;
};

struct SpellDisableCheck {
    bool* isEnabledConfig;                        // Condition based on config
    std::function<bool(RE::SpellItem*)> checkFn;  // Function to check the spell property
    std::string_view description;                 // Description for logging
};

struct GeneralRule {
    bool enabled = true;
    bool shoutsEnabled = false;
    bool spellsEnabled = true;
    bool summonsEnabled = true;
    bool lesserPowersEnabled = true;
    bool greaterPowersEnabled = false;
    bool scrollsEnabled = false;
    bool recastableEnabled = false;

    OrderedMap<std::string, bool*> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> GetParsers();
    std::vector<SpellDisableCheck> checks = {
        // checks for early return
        {&shoutsEnabled, IsShout, "Shouts"},
        {&lesserPowersEnabled, IsLesserPower, "Lesser Powers"},
        {&greaterPowersEnabled, IsGreaterPower, "Greater Powers"},
        {&summonsEnabled, IsSummon, "Summons"},
        {&spellsEnabled, IsSpell, "Spells"},
        {&scrollsEnabled, IsScroll, "Scrolls"},
        {nullptr, IsConcentration, "Concentration spells"},  // Always disabled if concentration
        {nullptr,
         [](RE::SpellItem* spellItem) {
             return spellItem && (spellItem->data.flags & RE::SpellItem::SpellFlag::kFoodItem);
         },
         "Food items"},  // Always disabled if food flag is set
        {&recastableEnabled, IsNonRecastable, "Spells that are not recastable"},
    };

    std::optional<std::string_view> ShouldReturnEarly(RE::SpellItem* spellItem) const;
    std::string ToString() const;
};

SpellRule GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect);
bool FindSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect, SpellRule& spellRule);
bool FindSpellRuleForSpellByPluginName(const std::string& pluginName, SpellRule& spellRule);

bool FindSpellRuleForSpellItem(RE::SpellItem* spellItem, SpellRule& spellRule);
bool IsSpellRuleDefined(RE::SpellItem* spellItem);

