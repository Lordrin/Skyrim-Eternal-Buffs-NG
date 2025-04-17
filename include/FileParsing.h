#pragma once

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

#include "StringUtilities.h"
#include "ConfigRules.h"
// #include "SpellDataPersistence.h"

namespace Parser {
    /**
     * @brief Resolves an identifier to a Skyrim form.
     *
     * @tparam T The type of form to resolve (e.g., RE::SpellItem).
     * @param identifier The identifier string (e.g., FormID~Plugin, EditorID).
     * @param configFileName The name of the configuration file for logging purposes.
     * @return A pointer to the resolved form, or nullptr if resolution fails.
     */
    template <typename T>
    T* ResolveIdentifier(const std::string& identifier, const std::string& configFileName);

    /**
     * @brief Parses an "Effect = ..." rule from a configuration file.
     *
     * @param valueString The value string containing the rule details.
     * @param configFileName The name of the configuration file for logging purposes.
     */
    void ParseEffectRule(const std::string& valueString, const std::string& configFileName);

    /**
     * @brief Parses a "Spell = ..." rule from a configuration file.
     *
     * @param valueString The value string containing the rule details.
     * @param configFileName The name of the configuration file for logging purposes.
     */
    void ParseSpellRule(const std::string& valueString, const std::string& configFileName);
}


class ConfigLoader {
public:
    using KeywordParser = std::function<void(const std::string&, const std::string&)>;

    ConfigLoader();

    /**
     * @brief Registers keyword-to-parser associations.
     */
    void RegisterParsers();

    /**
     * @brief Loads a single configuration file.
     *
     * @param filePath The path to the configuration file.
     */
    void LoadConfigFile(const std::filesystem::path& filePath);

private:
    std::map<std::string, KeywordParser> keywordParsers;
};