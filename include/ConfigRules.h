#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <variant>

#include "OrderedMap.h"
#include "StringUtilities.h"
// #include "ConfigParser.h"

using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*, float*>;

struct BaseRule {
    std::string sourceFile;
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;
    bool isPermanentEnabled = true;
    std::vector<std::string> keywordFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool ShouldApplyRuleToForm(RE::TESForm* form) const;
    std::string ToString() const;
};

struct SpellRule : BaseRule {
    float durationFilter = -1.0f;
    uint32_t minDurationFilter = 0;
    float magnitudeFilter = -1.0f;
    
    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool ShouldApplyRuleToSpell(RE::SpellItem* spellItem) const;
    void ApplySpellRulesToActiveEffect(RE::ActiveEffect* activeEffect) const;
    std::string ToString() const;
};

struct GeneralRule {
    bool enabled = true;
    bool shoutsEnabled = true;
    bool spellsEnabled = true;
    bool summonsEnabled = true;
};

// namespace Config {
//     const float permanentSpellDuration = 86313600.0f; // 999 days
//     extern std::unordered_map<std::string, SpellRule> spellRules;
//     std::unordered_map<std::string, SpellRule>& GetSpellRules();
//     extern GeneralRule generalRule;
//     extern std::unordered_map<RE::FormID, RE::TESShout*> shoutSpellMap;
//     std::unordered_map<RE::FormID, RE::TESShout*>& GetShoutSpellMap();
//     extern uint32_t keyBinding = 43;
//     extern bool toggleKeyHeld = false;
//     void InitializeShoutSpellMap();
// }