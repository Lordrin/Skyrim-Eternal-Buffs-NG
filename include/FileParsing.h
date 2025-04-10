#pragma once

#include <StringUtilities.h>

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

// --- Shared Base Structure ---
struct BaseRule {
    std::string  sourceFile;
    RE::TESForm* resolvedForm = nullptr; // Common base pointer!
    std::string nameFilter;
    bool isPermanentEnabled = true;
    std::string keywordFilter;
};

// --- EffectRule Structure ---
struct EffectRule : BaseRule {
    std::string durationFilter;
    std::string minDurationFilter;
    std::string magnitudeFilter;
};

// Specific structure for Spell rules
struct SpellRule : BaseRule {
    std::string durationFilter;
    std::string minDurationFilter;
    std::string magnitudeFilter;
};

namespace DataStore {
    extern std::vector<EffectRule> effectRules;
    extern std::unordered_map<std::string, SpellRule> spellRules;
}


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