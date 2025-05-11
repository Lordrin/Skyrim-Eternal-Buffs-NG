#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <variant>

#include "OrderedMap.h"
#include "SpellUtilities.h"
#include "StringUtilities.h"

using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*, float*>;

struct BaseRule {
    std::string sourceFile;
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;
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
    bool isDisabledInConfig;                      // Condition based on config
    std::function<bool(RE::SpellItem*)> checkFn;  // Function to check the spell property
    std::string_view description;                 // Description for logging
};

struct GeneralRule {
    bool enabled = true;
    bool shoutsEnabled = true;
    bool spellsEnabled = true;
    bool summonsEnabled = true;
    bool lesserPowersEnabled = true;
    bool greaterPowersEnabled = true;
    bool scrollsEnabled = true;

    OrderedMap<std::string, bool*> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> GetParsers();
    const std::vector<SpellDisableCheck> checks = {
        {!&shoutsEnabled, IsShout, "Shouts"},
        {!&lesserPowersEnabled, IsLesserPower, "Lesser Powers"},
        {!&greaterPowersEnabled, IsGreaterPower, "Greater Powers"},
        {!&summonsEnabled, IsSummon, "Summons"},
        {!&spellsEnabled, IsSpell, "Spells"},  // Make sure IsSpell correctly identifies *only* regular spells if needed
        {!&scrollsEnabled, IsScroll, "Scrolls"},
        {true, IsConcentration, "Concentration spells"},  // Always disabled if concentration
        {true,
         [](RE::SpellItem* si) {  // Lambda for flags
             return si && (si->data.flags & RE::SpellItem::SpellFlag::kFoodItem);
         },
         "Food items"}  // Always disabled if food flag is set
    };

    std::string ToString() const;
};

SpellRule GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect);
bool GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect, SpellRule& spellRule);

bool GetSpellRuleForSpellItem(RE::SpellItem* spellItem, SpellRule& spellRule);