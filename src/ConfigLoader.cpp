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
                currentSection = StringUtilities::ToLower(
                    StringUtilities::TrimString(trimmedLine.substr(1, trimmedLine.length() - 2)));
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

            std::string keyword =
                StringUtilities::ToLower(StringUtilities::TrimString(trimmedLine.substr(0, equalsPos)));
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

bool ConfigLoader::UpdateConfigValue(const std::filesystem::path& filePath, const std::string& section,
                                     const std::string& keyword, const std::string& value) {
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        logger::error("UpdateConfigValue: could not open {} for reading.", filePath.string());
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inFile, line)) {
        lines.push_back(line);
    }
    inFile.close();

    const std::string targetSection = StringUtilities::ToLower(section);
    const std::string targetKeyword = StringUtilities::ToLower(keyword);

    std::string currentSection;
    int sectionStartLine = -1;  // line index of "[section]" if found
    int keywordLine = -1;       // line index of "keyword = ..." if found within that section
    int sectionEndLine = -1;    // line index right after the section's last line (insertion point)

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string trimmed = StringUtilities::TrimString(lines[i]);

        if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
            // entering a new section
            if (currentSection == targetSection && sectionEndLine == -1) {
                sectionEndLine = static_cast<int>(i);  // previous section just ended here
            }
            currentSection =
                StringUtilities::ToLower(StringUtilities::TrimString(trimmed.substr(1, trimmed.length() - 2)));
            if (currentSection == targetSection) {
                sectionStartLine = static_cast<int>(i);
            }
            continue;
        }

        if (currentSection == targetSection && keywordLine == -1) {
            auto equalsPos = trimmed.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = StringUtilities::ToLower(StringUtilities::TrimString(trimmed.substr(0, equalsPos)));
                if (key == targetKeyword) {
                    keywordLine = static_cast<int>(i);
                }
            }
        }
    }
    if (currentSection == targetSection && sectionEndLine == -1) {
        sectionEndLine = static_cast<int>(lines.size());  // section ran to EOF
    }

    if (keywordLine != -1) {
        // Keyword exists: replace just its value, preserve any trailing comment.
        std::string& targetLine = lines[keywordLine];
        auto equalsPos = targetLine.find('=');
        auto commentPos = targetLine.find(';', equalsPos);
        std::string comment = (commentPos != std::string::npos) ? targetLine.substr(commentPos) : "";
        targetLine = keyword + " = " + value + (comment.empty() ? "" : "  " + comment);
    } else if (sectionStartLine != -1) {
        // Section exists but keyword doesn't: insert just before the section ends.
        lines.insert(lines.begin() + sectionEndLine, keyword + " = " + value);
    } else {
        // Section doesn't exist at all: append a new section + keyword at EOF.
        if (!lines.empty() && !lines.back().empty()) {
            lines.push_back("");
        }
        lines.push_back("[" + section + "]");
        lines.push_back(keyword + " = " + value);
    }

    std::ofstream outFile(filePath, std::ios::trunc);
    if (!outFile.is_open()) {
        logger::error("UpdateConfigValue: could not open {} for writing.", filePath.string());
        return false;
    }
    for (const auto& l : lines) {
        outFile << l << "\n";
    }

    logger::info("Updated [{}] {} = {} in {}", section, keyword, value, filePath.filename().string());
    return true;
}
