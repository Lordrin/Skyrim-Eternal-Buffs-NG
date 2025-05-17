#pragma once

#include <string>
#include <vector>

#include "ConfigRules.h"

// This is meant to parse lines and resolve complete form IDs, EditorIDs, or plugin names
namespace Parser {
    /**
     * @brief Resolves an identifier to a Skyrim form.
     * @param identifier The identifier string (e.g., FormID~Plugin, EditorID).
     * @param configFileName  The name of the configuration file for logging purposes.
     * @return A pointer to the resolved form, or nullptr if resolution fails.
     **/
    RuleIdentifierResult ResolveIdentifier(const std::string& identifier, const std::string& configFileName);
    RE::TESForm* TryResolveFormIDPlugin(const std::string& identifier, const std::string& configFileName,
                                        RE::TESDataHandler* dataHandler);
    RE::TESForm* TryResolveEditorID(const std::string& identifier, const std::string& configFileName);
    RE::TESForm* TryResolveHexFormID(const std::string& identifier, RE::TESDataHandler* dataHandler);
    std::optional<std::string> TryResolvePluginName(const std::string& identifier, const std::string& configFileName);
    /**
     * @brief Parses a line split into parts and applies the appropriate parsers.
     * @param SplitLine The split line parts. From the config file.
     * @param parserOrder The order of parsers to apply. The order of the keys in the map and the config variables.
     * @param parserMap The map of parsers to apply.
     **/
    void ParseSplitLine(const std::vector<std::string>& SplitLine, const std::vector<std::string>& parserOrder,
                        const std::unordered_map<std::string, std::function<void(const std::string&)>>& parserMap);

    SpellRule ParseSpellRule(const std::string& configLine, const std::string& configFileName);

    std::unordered_map<std::string, std::function<void(const std::string&, const std::string&)>> PopulateParseGeneralSection();
    std::unordered_map<std::string, std::function<void(const std::string&, const std::string&)>> PopulateParseConfigSection();

    bool ParseBoolString(const std::string& value);
}