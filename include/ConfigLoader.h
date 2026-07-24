#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ConfigParser.h"
#include "ConfigRules.h"
#include "StringUtilities.h"

class ConfigLoader {
public:
    ConfigLoader();
    static ConfigLoader& GetSingleton() {
        static ConfigLoader instance;
        return instance;
    }

    using KeywordParser = std::function<void(const std::string&, const std::string&)>;

    // ConfigLoader();

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
    static inline const std::filesystem::path directory = "Data/SKSE/Plugins/";



private:
    std::unordered_map<std::string, std::unordered_map<std::string, KeywordParser>> sections;
};