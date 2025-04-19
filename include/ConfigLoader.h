#pragma once

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

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
     * @brief Loads a single configuration file.
     *
     * @param filePath The path to the configuration file.
     */
    void LoadConfigFile(const std::filesystem::path& filePath);

private:
    std::map<std::string, KeywordParser> keywordParsers;
};