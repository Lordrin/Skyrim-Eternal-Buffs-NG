#include "ConfigLoader.h"

#include <sstream>

// --- Config Loader Class ---
// --- LoricaNGConfigLoader Class Implementation ---
ConfigLoader::ConfigLoader() { RegisterParsers(); }

void ConfigLoader::RegisterParsers() {
    keywordParsers["spell"] = Parser::ParseSpellRule;

    // **Extensibility Point:** Add more rules easily
    // keywordParsers["perk"] = Parser::ParsePerkRule;
}

void ConfigLoader::LoadConfigFile(const std::filesystem::path& filePath) {
    logger::info("--- Loading config file: {} ---", filePath.string());
    std::ifstream configFile(filePath);
    std::string line;
    std::string currentSection;  // You might use sections later if needed
    int lineNum = 0;

    while (std::getline(configFile, line)) {
        lineNum++;
        std::string trimmedLine = Utilities::TrimString(line);

        // Skip comments and empty lines
        if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#') {
            continue;
        }

        // Basic section handling (optional, could be used for context)
        if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
            currentSection = Utilities::TrimString(trimmedLine.substr(1, trimmedLine.length() - 2));
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

        std::string keyword = Utilities::ToLower(Utilities::TrimString(trimmedLine.substr(0, equalsPos)));
        std::string value = Utilities::TrimString(trimmedLine.substr(equalsPos + 1));

        if (value.empty()) {
            logger::warn("Skipping rule with empty value for keyword '{}' on line {} in {}", keyword, lineNum,
                         filePath.filename().string());
            continue;
        }

        // --- Find and call the registered parser for this keyword ---
        auto it = keywordParsers.find(keyword);
        if (it != keywordParsers.end()) {
            // Call the associated function (e.g., ParseEffectRule)
            try {
                it->second(value, filePath.filename().string());  // Pass value string and filename
            } catch (const std::exception& e) {
                logger::error("Exception parsing rule on line {} in {}: {}. Rule: '{} = {}'", lineNum,
                              filePath.filename().string(), e.what(), keyword, value);
            } catch (...) {
                logger::error("Unknown exception parsing rule on line {} in {}", lineNum, filePath.filename().string());
            }
        } else {
            logger::warn("Unknown keyword '{}' on line {} in {}. Skipping.", keyword, lineNum,
                         filePath.filename().string());
        }
    }
    logger::info("--- Finished loading config file: {} ---", filePath.string());
}
