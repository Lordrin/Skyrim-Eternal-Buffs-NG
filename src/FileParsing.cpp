#include "FileParsing.h"

// --- DataStore Namespace ---
namespace DataStore {
    std::vector<EffectRule> effectRules;
    // std::vector<SpellRule> spellRules;
    std::unordered_map<std::string, SpellRule> spellRules;
    // std::vector<std::string> blacklistedNames;
    // std::vector<RE::FormID> blacklistedFormIDs;
}

// Logs the attributes of BaseRule
void LogBaseRule(const BaseRule& rule) {
    logger::info("BaseRule:");
    logger::info("  - Source File: {}", rule.sourceFile);
    logger::info("  - Resolved Form: {}", rule.resolvedForm ? rule.resolvedForm->GetName() : "nullptr");
    logger::info("  - Name Filter: {}", rule.nameFilter);
    logger::info("  - Is Permanent Enabled: {}", rule.isPermanentEnabled ? "true" : "false");
    logger::info("  - Keyword Filter: {}", rule.keywordFilter);
}

// Logs the attributes of EffectRule
void LogEffectRule(const EffectRule& rule) {
    LogBaseRule(rule);  // Log BaseRule attributes
    logger::info("EffectRule:");
    logger::info("  - Duration Filter: {}", rule.durationFilter);
    logger::info("  - Min Duration Filter: {}", rule.minDurationFilter);
    logger::info("  - Magnitude Filter: {}", rule.magnitudeFilter);
}

// Logs the attributes of SpellRule
void LogSpellRule(const SpellRule& rule) {
    LogBaseRule(rule);  // Log BaseRule attributes
    logger::info("SpellRule:");
    logger::info("  - Duration Filter: {}", rule.durationFilter);
    logger::info("  - Min Duration Filter: {}", rule.minDurationFilter);
    logger::info("  - Magnitude Filter: {}", rule.magnitudeFilter);
}

// --- 3. Parser Namespace ---
namespace Parser {

    // --- Helper: Resolve Identifier (Generalized) ---
    // Template allows resolving different form types
    template <typename T>
    T* ResolveIdentifier(const std::string& identifier, const std::string& configFileName) {
        T* resolvedForm = nullptr;
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("TESDataHandler not found.");
            return nullptr;
        }

        // 1. Try FormID~Plugin
        auto formPluginPair = Utilities::SplitString(identifier, '~');
        if (formPluginPair.size() == 2) {
            try {
                RE::FormID localFormID = std::stoul(Utilities::TrimString(formPluginPair[0]), nullptr, 16);
                std::string pluginName = Utilities::TrimString(formPluginPair[1]);
                resolvedForm = dataHandler->LookupForm<T>(localFormID, pluginName);
                // RE::FormID resolvedForm1 = dataHandler->LookupFormID(localFormID, pluginName);
                // resolvedForm = resolvedForm1.As<T>();
                if (resolvedForm) {
                    logger::info("Resolved FormID {:X} to '{}' (config: {})", localFormID, resolvedForm->GetFullName(),
                                 configFileName);
                    return resolvedForm;
                } else {
                    logger::warn("FormID {:X} not found in plugin '{}' (config: {})", localFormID, pluginName,
                                 configFileName);
                }
            } catch (...) { /* ignore format errors, try next method */
            }
        }

        // 2. Try EditorID
        // Note: LookupFormByEditorID is often case-sensitive in base game.
        // You might need custom logic for case-insensitivity if required.
        try {
            resolvedForm =
                static_cast<T*>(RE::TESForm::LookupByID(std::stoul(Utilities::TrimString(identifier), nullptr, 16)));
            if (resolvedForm) {
                logger::info("Resolved EditorID '{}' to FormID {:X} (config: {})", identifier,
                             resolvedForm->GetFormID(), configFileName);
                return resolvedForm;
            }
        } catch (...) { /* ignore format errors, try next method */
        }

        // 3. Try Hex FormID (direct lookup, primarily for base game/masters)
        if ((identifier.starts_with("0x") || identifier.starts_with("0X")) &&
            identifier.find('~') == std::string::npos) {
            try {
                RE::FormID globalFormID = std::stoul(identifier, nullptr, 16);
                resolvedForm = RE::TESForm::LookupByID<T>(globalFormID);
                if (resolvedForm)
                    return resolvedForm;
                else {
                    // Optional: Explicitly check common masters
                    resolvedForm = dataHandler->LookupForm<T>(globalFormID, "Skyrim.esm");
                    if (resolvedForm) return resolvedForm;
                    resolvedForm = dataHandler->LookupForm<T>(globalFormID, "Update.esm");
                    if (resolvedForm) return resolvedForm;
                    // Add DLC checks if needed...
                }
            } catch (...) { /* Ignore parse errors */
            }
        }

        logger::info("Could not resolve identifier '{}' for type {} (config: {}). Treating identifier as a name.",
                     identifier, typeid(T).name(), configFileName);
        return nullptr;
    }

    std::string GetIdentifierFromFileString(const std::vector<std::string>& parts, const std::string& configFileName) {
        if (parts.empty()) {
            logger::warn("Skipping empty Effect rule in '{}'", configFileName);
            return "";
        }

        return Utilities::TrimString(parts[0]);
    }

    // void ParseFilters(const std::string& configFileName){
    //     // Create the rule object
    //     EffectRule rule;
    //     rule.sourceFile = configFileName;
    //     rule.resolvedForm = resolvedEffect;

    //     // --- Assign Filters by Position ---
    //     // Use .at() with checks or conditional assignment for safety/extensibility

    //     // Filter 1: Name
    //     if (parts.size() > 1) rule.nameFilter = Utilities::TrimString(parts[1]);
    //     // Filter 2: Keywords
    //     if (parts.size() > 2) rule.keywordFilter = Utilities::TrimString(parts[2]);
    //     // Filter 3: Duration
    //     if (parts.size() > 3) rule.durationFilter = Utilities::TrimString(parts[3]);
    //     // Filter 4: MinDuration
    //     if (parts.size() > 4) rule.minDurationFilter = Utilities::TrimString(parts[4]);
    //     // Filter 5: Magnitude
    //     if (parts.size() > 5) rule.magnitudeFilter = Utilities::TrimString(parts[5]);

    //     // **Extensibility Point:** If you add a 6th filter (e.g., Area) later:
    //     // if (parts.size() > 6) rule.areaFilter = Utilities::TrimString(parts[6]);
    //     // You just need to add the 'areaFilter' field to the EffectRule struct.

    //     // Store the parsed rule
    //     DataStore::effectRules.push_back(std::move(rule));
    //     logger::info("Parsed Effect rule for '{}' [{:X}] from '{}'", identifier, resolvedEffect->GetFormID(),
    //                     configFileName);
    // }

    void ParseBaseRule(BaseRule& rule, const std::vector<std::string>& parts) {
        logger::info("Parsing BaseRule from parts: {}", parts.size());
        // 4. Assign as Name
        if (rule.resolvedForm == nullptr) {  // if FormID is not resolved, treat it as the name
            // Filter 0: Name
            if (parts.size() > 0) rule.nameFilter = Utilities::TrimString(parts[0]);
            logger::info("Parsed BaseRule name: {}", rule.nameFilter);
        }
        // Filter 1: IsPermanent
        if (parts.size() > 1) {
            std::string isPermanentString = Utilities::TrimString(parts[1]);
            if (isPermanentString == "true" || isPermanentString == "1") {
                rule.isPermanentEnabled = true;
            } else if (isPermanentString == "false" || isPermanentString == "0") {
                rule.isPermanentEnabled = false;
            } else {
                logger::warn("Invalid value for IsPermanent in '{}': '{}'. Defaulting to true.", rule.sourceFile,
                             isPermanentString);
            }
        }
        // Filter 2: Keywords
        if (parts.size() > 2) rule.keywordFilter = Utilities::TrimString(parts[2]);
    }

    // --- Specific Parsing Function for "Effect = ..." rules ---
    void ParseEffectRule(const std::string& valueString, const std::string& configFileName) {
        auto parts = Utilities::SplitString(valueString, '|');
        if (parts.empty()) { /* log */
            return;
        }

        std::string identifier = Utilities::TrimString(parts[0]);
        // *** Resolve as the SPECIFIC type expected for this rule ***
        RE::EffectSetting* resolvedEffect = ResolveIdentifier<RE::EffectSetting>(identifier, configFileName);
        if (!resolvedEffect) {
            return;
        }  // Error logged by resolver

        EffectRule rule;

        ParseBaseRule(rule, parts);  // Parse common filters
        // Filter 4: Duration
        if (parts.size() > 3) rule.durationFilter = Utilities::TrimString(parts[3]);
        // Filter 5: MinDuration
        if (parts.size() > 4) rule.minDurationFilter = Utilities::TrimString(parts[4]);
        // Filter 6: Magnitude
        if (parts.size() > 5) rule.magnitudeFilter = Utilities::TrimString(parts[5]);
        // ... etc for other filters ...

        DataStore::effectRules.push_back(std::move(rule));
        logger::info("Parsed Effect rule for '{}' [{:X}] from '{}'", identifier, rule.resolvedForm->GetFormID(),
                     configFileName);
    }

    // --- Specific Parsing Function for "Spell = ..." rules ---
    void ParseSpellRule(const std::string& valueString, const std::string& configFileName) {
        auto parts = Utilities::SplitString(valueString, '|');
        if (parts.empty()) { /* log */
            return;
        }

        std::string identifier = Utilities::TrimString(parts[0]);
        // *** Resolve as the SPECIFIC type ***
        RE::SpellItem* resolvedSpell = ResolveIdentifier<RE::SpellItem>(identifier, configFileName);
        SpellRule rule;
        if (!resolvedSpell) {
            if (parts.size() > 0) rule.nameFilter = Utilities::TrimString(parts[0]);
            logger::info("Parsed BaseRule name: {}", rule.nameFilter);
            // return;
        } else {
            rule.resolvedForm = resolvedSpell;
        }

        rule.sourceFile = configFileName;
        logger::info("SpellRule sourceFile: {}", rule.sourceFile);
        // *** Store in the common base pointer ***

        ParseBaseRule(rule, parts);  // Parse common filters
        // Filter 3: Duration
        if (parts.size() > 3) rule.durationFilter = Utilities::TrimString(parts[3]);
        // Filter 4: MinDuration
        if (parts.size() > 4) rule.minDurationFilter = Utilities::TrimString(parts[4]);
        // Filter 5: Magnitude
        if (parts.size() > 5) rule.magnitudeFilter = Utilities::TrimString(parts[5]);
        // ... etc ...
        // Parse chance from the last part if it's numeric (as shown previously)
        //  if (!parts.empty()) {
        //     std::string lastPart = Utilities::TrimString(parts.back());
        //     try {
        //         size_t charsParsed = 0;
        //         float parsedChance = std::stof(lastPart, &charsParsed);
        //         if (charsParsed == lastPart.length() && parsedChance >= 0.0f && parsedChance <= 100.0f) {
        //             rule.chance = parsedChance;
        //             // Optional: remove chance part from filter processing if needed
        //         }
        //     } catch (...) { /* not a float, treat as filter */ }
        //  }

        LogSpellRule(rule);  // Log the parsed rule for debugging
        if (rule.resolvedForm) {
            // Attempt to cast the resolved form to a SpellItem
            if (auto* spellItem = rule.resolvedForm->As<RE::SpellItem>()) {
                // Store the rule in the map using the spell's full name as the key
                DataStore::spellRules.emplace(spellItem->GetFullName(), std::move(rule));
                logger::info("Parsed Spell rule for '{}' [{:X}] from '{}'", identifier, spellItem->GetFormID(),
                             configFileName);
            } else {
                logger::warn("Parsed Spell rule for '{}' but resolved form is not a SpellItem", identifier);
            }
        } else {
            // Handle the case where the resolved form is nullptr
            if (!rule.nameFilter.empty()) {
                DataStore::spellRules.emplace(rule.nameFilter, std::move(rule));
                logger::info("Parsed Spell rule for '{}' as name from '{}'", identifier, configFileName);
            }
            logger::warn("Parsed Spell rule for '{}' but resolved form is nullptr", identifier);
        }
        // TODO see if there are any spells that have no full name+
        // DataStore::spellRules.push_back(std::move(rule));
        // logger::info("Parsed Spell rule for '{}' [{:X}] from '{}'", identifier, rule.resolvedForm->GetFormID(),
        //              configFileName);
    }

}  // namespace Parser

// --- 4. Config Loader Class ---
// --- LoricaNGConfigLoader Class Implementation ---
ConfigLoader::ConfigLoader() { RegisterParsers(); }

void ConfigLoader::RegisterParsers() {
    // Associate the "Effect" keyword (lowercase) with the ParseEffectRule function
    keywordParsers["effect"] = Parser::ParseEffectRule;
    keywordParsers["spell"] = Parser::ParseSpellRule;  // Alias for "effect" if needed

    // **Extensibility Point:** Add more rules easily
    // keywordParsers["perk"] = Parser::ParsePerkRule;
    // keywordParsers["item"] = Parser::ParseItemRule;
    // keywordParsers["some_new_rule"] = Parser::ParseSomeNewRule;
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
