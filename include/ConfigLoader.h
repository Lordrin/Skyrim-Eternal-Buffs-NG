#pragma once

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <sstream>

#include "StringUtilities.h"
#include "ConfigRules.h"
#include "ConfigParser.h"

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

    bool UpdateConfigValue(const std::filesystem::path& filePath, const std::string& section,
                            const std::string& keyword, const std::string& value);

    std::vector<std::string> GetConfigFileNames();
    const std::filesystem::path directory = "Data/SKSE/Plugins/";
    
private:
    std::unordered_map<std::string, std::unordered_map<std::string, KeywordParser>> sections;
};