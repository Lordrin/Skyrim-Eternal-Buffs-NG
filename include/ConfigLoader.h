#pragma once

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <sstream>

#include "StringUtilities.h"
#include "ConfigRules.h"
// #include "SpellDataPersistence.h"

class ConfigLoader {
public:
    using KeywordParser = std::function<void(const std::string&, const std::string&)>;

    ConfigLoader();

    /**
     * @brief Registers keyword-to-parser associations.
     */
    void RegisterParsers();


    /**
     * @brief Registers a parser for a specific section and keyword.
     *
     * @param section The section name in the configuration file.
     * @param keyword The keyword to be parsed.
     * @param parser The function that will handle the parsing.
     */
    void RegisterParser(const std::string& section, const std::string& keyword, KeywordParser parser);

    /**
     * @brief Loads a single configuration file.
     *
     * @param filePath The path to the configuration file.
     */
    void LoadConfigFile(const std::filesystem::path& filePath);

private:
    std::unordered_map<std::string, KeywordParser> keywordParsers;
    std::unordered_map<std::string, std::unordered_map<std::string, KeywordParser>> sections;
};