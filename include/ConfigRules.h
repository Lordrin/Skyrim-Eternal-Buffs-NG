#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <variant>

#include "OrderedMap.h"
#include "StringUtilities.h"

// Pointers here so it can be changed later
using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*>;

struct BaseRule {
    std::string sourceFile;
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;
    bool isPermanentEnabled = true;
    std::vector<std::string> keywordFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    void Log() const;
};

struct SpellRule : BaseRule {
    uint32_t durationFilter;
    uint32_t minDurationFilter;
    uint32_t magnitudeFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    void Log() const;
};

extern std::unordered_map<std::string, SpellRule> spellRules;

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
}