#include "FileParsing.h"

#include <sstream>

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

    void ParseBaseRule(BaseRule& rule, const std::vector<std::string>& parts) {
        logger::info("Parsing BaseRule from parts: {}", parts.size());
        // // 4. Assign as Name
        // if (rule.resolvedForm == nullptr) {  // if FormID is not resolved, treat it as the name
        //     // Filter 0: Name
        //     if (parts.size() > 0) rule.nameFilter = Utilities::TrimString(parts[0]);
        //     logger::info("Parsed BaseRule name: {}", rule.nameFilter);
        // }
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
        // // Filter 4: Duration
        // if (parts.size() > 3) rule.durationFilter = Utilities::TrimString(parts[3]);
        // // Filter 5: MinDuration
        // if (parts.size() > 4) rule.minDurationFilter = Utilities::TrimString(parts[4]);
        // // Filter 6: Magnitude
        // if (parts.size() > 5) rule.magnitudeFilter = Utilities::TrimString(parts[5]);
        // // ... etc for other filters ...

        SpellDataPersistence::effectRules.push_back(std::move(rule));
        logger::info("Parsed Effect rule for '{}' [{:X}] from '{}'", identifier, rule.resolvedForm->GetFormID(),
                     configFileName);
    }

    // --- Specific Parsing Function for "Spell = ..." rules ---
    void ParseSpellRule(const std::string& valueString, const std::string& configFileName) {
        auto parts = Utilities::SplitString(valueString, '|');
        if (parts.empty()) { /* log */
            return;
        }

        // SpellRule rawRule;

        // auto fields = rawRule.GetFields();
        // for (const auto& fieldKey : fields.GetOrder()) {
        //     logger::info("Field: {}", fieldKey);
        // }

        // for (const auto& item : fields.GetMap()){
        //     logger::info("Field from map: {}", item.first);
        //     std::visit(
        //         [](const auto& value) {
        //             using T = std::decay_t<decltype(value)>;  // Get the actual type
        //             // print the type of the value
        //             // logger::info("Type: {}", typeid(value).name());

        //             if constexpr (std::is_same_v<T, bool>) {
        //                 logger::info("bool: {}", value);
        //             } else if constexpr (std::is_same_v<T, RE::TESForm*>) {
        //                 logger::info("TESForm*");
        //             } else if constexpr (std::is_same_v<T, std::string*>) {
        //                 logger::info("String: \"{}\"", *value);
        //             }
        //         },
        //         item.second);  // Pass the variant item to std::visit
        // }

        // for (const auto& item : rawRule.GetFields().GetMap()) {
        //     // Use std::visit to safely access the value based on its current type
        //     std::visit([](const auto& value) {
        //         // 'value' will have the actual type (int, float, or string)
        //         // inside this lambda
        //         using T = std::decay_t<decltype(value)>; // Get the actual type

        //         if constexpr (std::is_same_v<T, bool>) {
        //             std::cout << "bool: " << value << std::endl;
        //         } else if constexpr (std::is_same_v<T, RE::TESForm*>) {
        //             std::cout << "Float: " << value << std::endl;
        //         } else if constexpr (std::is_same_v<T, std::string*>) {
        //             std::cout << "String: \"" << value << "\"" << std::endl;
        //         }
        //     }, item.second); // Pass the variant item to std::visit
        // }

        std::string identifier = Utilities::TrimString(parts[0]);
        // *** Resolve as the SPECIFIC type ***
        RE::SpellItem* resolvedSpell = ResolveIdentifier<RE::SpellItem>(identifier, configFileName);
        SpellRule rule;
        std::vector<std::string> ruleParts = rule.GetFields().GetOrder();
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

        // ParseBaseRule(rule, parts);  // Parse common filters
        // Filter 3: Duration
        // if (parts.size() > 3) rule.durationFilter = Utilities::TrimString(parts[3]);
        // Filter 4: MinDuration
        // if (parts.size() > 4) rule.minDurationFilter = Utilities::TrimString(parts[4]);
        // Filter 5: Magnitude
        // if (parts.size() > 5) rule.magnitudeFilter = Utilities::TrimString(parts[5]);

        auto fields = rule.GetFields();
        auto fieldOrder = fields.GetOrder();
        std::vector<std::string> trimmedFieldOrder(fieldOrder.begin() + 2, fieldOrder.end());

        std::ostringstream oss;
        for (size_t i = 0; i < trimmedFieldOrder.size(); ++i) {
            if (i != 0) {
                oss << ", ";
            }
            oss << trimmedFieldOrder[i];
        }
        logger::info("Field order: {}", oss.str());

        auto fieldMap = fields.GetMap();

        for (size_t i = 1; i < parts.size() && i < trimmedFieldOrder.size(); i++) {
            auto& matchingRuleKey = trimmedFieldOrder[i];

            auto& fieldValue = fieldMap[matchingRuleKey];

            std::visit(
                [&parts, i, matchingRuleKey](auto& value) {
                    using T = std::decay_t<decltype(value)>;  // Get the actual type

                    try {
                        std::string trimmedValue = Utilities::TrimString(parts[i]);
                        if (trimmedValue == "NONE") {
                            return;
                        }

                        if constexpr (std::is_same_v<T, std::string*>) {
                            *value = Utilities::TrimString(parts[i]);
                            logger::info("Field: {} set to {}", matchingRuleKey, *value);
                        } else if constexpr (std::is_same_v<T, bool*>) {
                            std::istringstream(parts[i]) >> std::boolalpha >> *value;
                            logger::info("Field: {} set to {}", matchingRuleKey, *value);
                        } else if constexpr (std::is_same_v<T, uint32_t*>) {
                            *value = std::stoul(trimmedValue, nullptr, 10);
                            logger::info("Field: {} set to {}", matchingRuleKey, *value);
                        }
                    } catch (const std::invalid_argument& e) {
                        logger::error("Invalid argument for field '{}': {}", matchingRuleKey, e.what());
                    } catch (const std::out_of_range& e) {
                        logger::error("Out of range for field '{}': {}", matchingRuleKey, e.what());
                    } catch (...) {
                        logger::error("Unknown error for field '{}'", matchingRuleKey);
                    }
                },
                fieldValue);  // Pass the variant item to std::visit
        }

        // for(const auto& fieldKey : fields.GetOrder()){
        //     if (fieldMap.find(fieldName) != fieldMap.end()) {
        //         logger::info("Field: {}", fieldKey);
        //     } else {
        //         logger::warn("Field not found: {}", fieldKey);
        //     }
        //     logger::info("Field: {}", fieldKey);
        // }
        // for (const auto& item : fields.GetMap()) {
        //     logger::info("Field from map: {}", item.first);
        //     std::visit(
        //         [](const auto& value) {
        //             using T = std::decay_t<decltype(value)>;  // Get the actual type

        //             if constexpr (std::is_same_v<T, bool>) {
        //                 logger::info("bool: {}", value);
        //             } else if constexpr (std::is_same_v<T, RE::TESForm*>) {
        //                 logger::info("TESForm*: {:#010x}", value->GetFormID());
        //             } else if constexpr (std::is_same_v<T, std::string*>) {
        //                 logger::info("String: \"{}\"", *value);
        //             }
        //         },
        //         item.second);  // Pass the variant item to std::visit
        // }

        // try {
        //     for (const auto& item : rule.GetFields().GetMap()) {
        //         logger::info("Field: {}", item.first);
        //         // Check if the variant is valid
        //         if (item.second.valueless_by_exception()) {
        //             logger::warn("RuleVariant is valueless for field: {}", item.first);
        //             continue;
        //         }
        //         // Use std::visit to safely access the value based on its current type
        //         std::visit(
        //             [](const auto& value) {
        //                 // 'value' will have the actual type (int, float, or string)
        //                 // inside this lambda
        //                 using T = std::decay_t<decltype(value)>;  // Get the actual type

        //                 if constexpr (std::is_same_v<T, bool>) {
        //                     SKSE::log::info("bool: {}", value);
        //                 } else if constexpr (std::is_same_v<T, RE::TESForm*>) {
        //                     SKSE::log::info("TESForm*");
        //                 } else if constexpr (std::is_same_v<T, std::string*>) {
        //                     SKSE::log::info("String: \"{}\"", *value);
        //                 }
        //             },
        //             item.second);  // Pass the variant item to std::visit
        //     }
        // } catch (const std::bad_variant_access& e) {
        //     logger::error("Error accessing variant: {}", e.what());
        // } catch (...) {
        //     logger::error("Unknown error accessing variant");
        // }

        SpellDataPersistence::LogSpellRule(rule);  // Log the parsed rule for debugging
        if (rule.resolvedForm) {
            // Attempt to cast the resolved form to a SpellItem
            if (auto* spellItem = rule.resolvedForm->As<RE::SpellItem>()) {
                // Store the rule in the map using the spell's full name as the key
                SpellDataPersistence::spellRules.emplace(spellItem->GetFullName(), std::move(rule));
                logger::info("Parsed Spell rule for '{}' [{:X}] from '{}'", identifier, spellItem->GetFormID(),
                             configFileName);
            } else {
                logger::warn("Parsed Spell rule for '{}' but resolved form is not a SpellItem", identifier);
            }
        } else {
            // Handle the case where the resolved form is nullptr
            if (!rule.nameFilter.empty()) {
                SpellDataPersistence::spellRules.emplace(rule.nameFilter, std::move(rule));
                logger::info("Parsed Spell rule for '{}' as name from '{}'", identifier, configFileName);
            }
            logger::warn("Parsed Spell rule for '{}' but resolved form is nullptr", identifier);
        }
        // TODO see if there are any spells that have no full name+
        // SpellDataPersistence::spellRules.push_back(std::move(rule));
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
