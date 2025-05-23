#include "ConfigLoader.h"

ConfigLoader::ConfigLoader() { RegisterParsers(); }

void ConfigLoader::RegisterParsers() {
    sections["config"] = Parser::PopulateParseConfigSection();
    sections["general"] = Parser::PopulateParseGeneralSection();
    sections["spells"]["spell"] = Parser::ParseSpellRule;

}

void ConfigLoader::LoadConfigFile(const std::filesystem::path& filePath) {
    logger::info("--- Loading config file: {} ---", filePath.string());
    try {
        std::ifstream configFile(filePath);
        std::string line;
        std::string currentSection = "spells";
        int lineNum = 0;

        while (std::getline(configFile, line)) {
            lineNum++;
            std::string trimmedLine = StringUtilities::TrimString(line);

            // Skip comments and empty lines
            if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#') {
                continue;
            }

            // Basic section handling (optional, could be used for context)
            if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
                currentSection =
                    StringUtilities::ToLower(StringUtilities::TrimString(trimmedLine.substr(1, trimmedLine.length() - 2)));
                logger::debug("Entering section: [{}]", currentSection);
                continue;
            }

            // Find the '=' separator for Keyword = Value
            auto equalsPos = trimmedLine.find('=');
            if (equalsPos == std::string::npos) {
                logger::warn("Malformed line {} in {}: No '=' found. Line: '{}'", lineNum, filePath.filename().string(),
                             trimmedLine);
                continue;
            }

            std::string keyword = StringUtilities::ToLower(StringUtilities::TrimString(trimmedLine.substr(0, equalsPos)));
            std::string value = StringUtilities::TrimString(trimmedLine.substr(equalsPos + 1));

            try {
                // Remove comment from value if present
                auto commentPos = value.find(';');
                if (commentPos != std::string::npos) {
                    value = value.substr(0, commentPos);
                }
            } catch (const std::exception& e) {
                logger::error("Exception parsing rule on line {} in {}: {}. Rule: '{} = {}'", lineNum,
                              filePath.filename().string(), e.what(), keyword, value);
                continue;
            }

            if (value.empty()) {
                logger::warn("Skipping rule with empty value for keyword '{}' on line {} in {}", keyword, lineNum,
                             filePath.filename().string());
                continue;
            }

            auto currentKeyWordParsers = sections.find(currentSection);
            if (currentKeyWordParsers == sections.end()) {
                logger::warn("Unknown section '{}' on line {} in {}. Skipping.", currentSection, lineNum,
                             filePath.filename().string());
                continue;
            }

            // --- Find and call the registered parser for this keyword ---
            auto it = currentKeyWordParsers->second.find(keyword);
            if (it != currentKeyWordParsers->second.end()) {
                // Call the associated function (e.g., ParseSpellRule)
                try {
                    it->second(value, filePath.filename().string());  // Pass value string and filename
                } catch (const std::exception& e) {
                    logger::error("Exception parsing rule on line {} in {}: {}. Rule: '{} = {}'", lineNum,
                                  filePath.filename().string(), e.what(), keyword, value);
                } catch (...) {
                    logger::error("Unknown exception parsing rule on line {} in {}", lineNum,
                                  filePath.filename().string());
                }
            } else {
                logger::warn("Unknown keyword '{}' on line {} in {}. Skipping.", keyword, lineNum,
                             filePath.filename().string());
            }
        }
    } catch (const std::exception& e) {
        logger::error("Failed to load config file {}: {}", filePath.string(), e.what());
        return;
    }
    logger::info("--- Finished loading config file: {} ---", filePath.string());
}

std::vector<std::string> ConfigLoader::GetConfigFileNames() {
    std::vector<std::string> configFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".ini" && entry.is_regular_file() &&
            entry.path().filename().string().ends_with("_EBUFFS.ini")) {
            configFiles.push_back(entry.path().filename().string());
        }
    }
    logger::info("Found {} config files.", configFiles.size());
    for (const auto& configFile : configFiles) {
        logger::info("Found config file: {}", configFile);
    }
    return configFiles;
}
