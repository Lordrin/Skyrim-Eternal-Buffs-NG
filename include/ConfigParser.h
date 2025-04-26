#pragma once

#include <string>
#include <vector>

#include "ConfigRules.h"

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
    void ParseLoggingLevelRule(const std::string& value, const std::string& configFileName);
}