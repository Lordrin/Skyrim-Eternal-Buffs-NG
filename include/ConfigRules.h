#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <variant>

#include "OrderedMap.h"
#include "StringUtilities.h"

// Pointers here so it can be changed later
using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*, float*>;
const float permanentSpellDuration = 86313600.0f; // 999 days

struct BaseRule {
    std::string sourceFile;
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;
    bool isPermanentEnabled = true;
    std::vector<std::string> keywordFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool ShouldApplyRuleToForm(RE::TESForm* form) const;

    void Log() const;
};

struct SpellRule : BaseRule {
    float durationFilter = -1.0f; // Default value for duration filter
    uint32_t minDurationFilter = 0;
    float magnitudeFilter = -1.0f; // Default value for magnitude filter
    
    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    bool ShouldApplyRuleToSpell(RE::SpellItem* spellItem) const;

    void ApplySpellRulesToActiveEffect(RE::ActiveEffect* activeEffect) const;
    void Log() const;
};

struct GeneralRule {
    bool enabled = true;
    bool shoutsEnabled = true;
    bool spellsEnabled = true;
};

namespace GBL {
    extern std::unordered_map<std::string, SpellRule> spellRules;
    std::unordered_map<std::string, SpellRule>& GetSpellRules();
    extern GeneralRule generalRule;
    extern std::unordered_map<RE::FormID, RE::TESShout*> shoutSpellMap;
    std::unordered_map<RE::FormID, RE::TESShout*>& GetShoutSpellMap();

    void InitializeShoutSpellMap();
}

namespace Parser {
    /**
     * @brief Resolves an identifier to a Skyrim form.
     * @param identifier The identifier string (e.g., FormID~Plugin, EditorID).
     * @param configFileName  The name of the configuration file for logging purposes.
     * @return A pointer to the resolved form, or nullptr if resolution fails.
     **/
    RE::TESForm* ResolveIdentifier(const std::string& identifier, const std::string& configFileName);
    RE::TESForm* TryResolveFormIDPlugin(const std::string& identifier, const std::string& configFileName,
                                        RE::TESDataHandler* dataHandler);
    RE::TESForm* TryResolveEditorID(const std::string& identifier, const std::string& configFileName);
    RE::TESForm* TryResolveHexFormID(const std::string& identifier, RE::TESDataHandler* dataHandler);
    /**
     * @brief Parses a line split into parts and applies the appropriate parsers.
     * @param SplitLine The split line parts. From the config file.
     * @param parserOrder The order of parsers to apply. The order of the keys in the map and the config variables.
     * @param parserMap The map of parsers to apply.
     **/
    void ParseSplitLine(const std::vector<std::string>& SplitLine, const std::vector<std::string>& parserOrder,
                        const std::unordered_map<std::string, std::function<void(const std::string&)>>& parserMap);

    SpellRule ParseSpellRule(const std::string& configLine, const std::string& configFileName);


    void ParseEnableRule(const std::string& value, const std::string& configFileName);
    void ParseShoutsEnabledRule(const std::string& value, const std::string& configFileName);
    void ParseSpellsEnabledRule(const std::string& value, const std::string& configFileName);
}