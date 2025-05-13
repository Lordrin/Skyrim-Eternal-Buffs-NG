#include "ConfigParser.h"

namespace Parser {
    RuleIdentifierResult ResolveIdentifier(const std::string& identifier, const std::string& configFileName) {
        RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
        RuleIdentifierResult result;
        if (!dataHandler) {
            logger::error("TESDataHandler not found.");
            return {RuleIdentifierType::kInvalid, nullptr};
        }

        // Step 1: Try FormID~Plugin
        if (auto resolvedForm = TryResolveFormIDPlugin(identifier, configFileName, dataHandler)) {
            return {RuleIdentifierType::kForm, resolvedForm};
        }

        // Step 2: Try EditorID
        if (auto resolvedForm = TryResolveEditorID(identifier, configFileName)) {
            return {RuleIdentifierType::kForm, resolvedForm};
        }

        // Step 3: Try Hex FormID
        if (auto resolvedForm = TryResolveHexFormID(identifier, dataHandler)) {
            return {RuleIdentifierType::kForm, resolvedForm};
        }

        // Step 4: Check if it's a plugin name
        if (auto pluginName = TryResolvePluginName(identifier, configFileName)) {
            return {RuleIdentifierType::kPlugin, pluginName.value()};
        }

        // Step 5: Treat as a name
        if (!identifier.empty()) {
            return {RuleIdentifierType::kName, identifier};
        }

        logger::info("Could not resolve identifier '{}' (config: {}). Treating identifier as a name.", identifier,
                     configFileName);
        return {RuleIdentifierType::kInvalid, nullptr};
    }

    RE::TESForm* TryResolveFormIDPlugin(const std::string& identifier, const std::string& configFileName,
                                        RE::TESDataHandler* dataHandler) {
        auto formPluginPair = Utilities::SplitString(identifier, '~');
        if (formPluginPair.size() == 2) {
            try {
                RE::FormID localFormID = std::stoul(Utilities::TrimString(formPluginPair[0]), nullptr, 16);
                std::string pluginName = Utilities::TrimString(formPluginPair[1]);
                RE::TESForm* resolvedForm = dataHandler->LookupForm(localFormID, pluginName);
                if (resolvedForm) {
                    logger::debug("[TryResolveFormIDPlugin] Resolved FormID {:#010x} to '{}' (config: {})", localFormID, resolvedForm->GetName(),
                                  configFileName);
                    return resolvedForm;
                } else {
                    logger::warn("[TryResolveFormIDPlugin] FormID {:#010x} not found in plugin '{}' (config: {})", localFormID, pluginName,
                                 configFileName);
                }
            } catch (...) { /* ignore format errors, try next method */
            }
        }
        return nullptr;
    }

    RE::TESForm* TryResolveEditorID(const std::string& identifier, const std::string& configFileName) {
        try {
            std::string_view sv(Utilities::TrimString(identifier));
            RE::TESForm* resolvedForm = RE::TESForm::LookupByEditorID(sv);
            if (resolvedForm) {
                logger::debug("Resolved EditorID '{}' to FormID {:X} (config: {})", identifier,
                              resolvedForm->GetFormID(), configFileName);
                return resolvedForm;
            }
        } catch (...) { /* ignore format errors, try next method */
        }
        return nullptr;
    }

    RE::TESForm* TryResolveHexFormID(const std::string& identifier, RE::TESDataHandler* dataHandler) {
        if ((identifier.starts_with("0x") || identifier.starts_with("0X")) &&
            identifier.find('~') == std::string::npos) {
            try {
                RE::FormID globalFormID = std::stoul(identifier, nullptr, 16);
                RE::TESForm* resolvedForm = RE::TESForm::LookupByID(globalFormID);
                if (resolvedForm) {
                    return resolvedForm;
                } else {
                    // Explicitly check common masters
                    resolvedForm = dataHandler->LookupForm(globalFormID, "Skyrim.esm");
                    if (resolvedForm) return resolvedForm;
                    resolvedForm = dataHandler->LookupForm(globalFormID, "Update.esm");
                    if (resolvedForm) return resolvedForm;
                }
            } catch (...) { /* Ignore parse errors */
            }
        }
        return nullptr;
    }

    std::optional<std::string> TryResolvePluginName(const std::string& identifier,
                                                    const std::string& /*configFileName*/) {
        if (!(identifier.ends_with(".esm") || identifier.ends_with(".esp") || identifier.ends_with(".esl"))) {
            return std::nullopt;
        }
        if (identifier.find('~') == std::string::npos) {
            return identifier;
        }
        if (identifier.starts_with("*")) {
            auto formPluginPair = Utilities::SplitString(identifier, '~');
            if (formPluginPair.size() == 2) {
                return formPluginPair[1];
            }
        }
        return std::nullopt;
    }

    void ParseSplitLine(const std::vector<std::string>& SplitLine, const std::vector<std::string>& parserOrder,
                        const std::unordered_map<std::string, std::function<void(const std::string&)>>& parserMap) {
        for (size_t i = 0; i < SplitLine.size() && i < parserOrder.size(); i++) {
            auto& matchingRuleKey = parserOrder[i];
            auto it = parserMap.find(matchingRuleKey);
            if (it != parserMap.end()) {
                it->second(SplitLine[i]);  // Call the parser function
            } else {
                logger::warn("Unknown parser for part {}: {}", i, SplitLine[i]);
            }
        }
    }

    SpellRule ParseSpellRule(const std::string& configLine, const std::string& configFileName) {
        std::vector<std::string> parts = Utilities::SplitString(configLine, '|');

        if (parts.empty()) {
            return {};
        }

        // Construct the vector in the correct order
        // std::vector<std::string> orderedParts;
        // orderedParts.reserve(parts.size() + 2);  // Reserve space for configFileName and identifier
        // orderedParts.push_back(configFileName);  // Add configFileName as the first part
        // orderedParts.push_back(parts[0]);        // Add the identifier as the second part
        // orderedParts.insert(orderedParts.end(), std::make_move_iterator(parts.begin()),
        //                     std::make_move_iterator(parts.end()));  // Move the rest of the parts

        logger::debug("valueString: {}", configLine);
        // logger::debug("parts: {}", Utilities::Join(orderedParts, "|"));

        SpellRule spellRule;
        spellRule.sourceFile = configFileName;
        ParseSplitLine(parts, spellRule.GetParsers().GetOrder(), spellRule.GetParsers().GetMap());

        if (spellRule.resolvedForm) {
            logger::info("Resolved FormID {:#010x} to '{}' (config: {})", spellRule.resolvedForm->GetFormID(),
                         spellRule.resolvedForm->GetName(), configFileName);
            // spellRule.nameFilter = spellRule.resolvedForm->GetName();  // Set the nameFilter to the resolved form's
            // name
        }

        Config::GetSingleton().GetSpellRules().insert({Utilities::RemoveWhitespace(spellRule.nameFilter), spellRule});

        auto to_print = spellRule.ToString();
        logger::debug("{}", to_print);
        return spellRule;
    }

    void ParseLoggingLevelRule(const std::string& value, const std::string& configFileName) {
        std::string loggingLevel = Utilities::ToLower(Utilities::TrimString(value));
        spdlog::level::level_enum level = spdlog::level::from_str(loggingLevel);
        if (level == spdlog::level::n_levels) {
            logger::warn("Unknown logging level '{}' in config file '{}'. Defaulting to 'info'.", loggingLevel,
                         configFileName);
            level = spdlog::level::info;
        }
        spdlog::set_level(level);
    }

    std::unordered_map<std::string, std::function<void(const std::string&, const std::string&)>>
    PopulateParseGeneralSection() {
        auto mergedConfigRules = Config::GetSingleton().GetGeneralRule().GetParsers();
        mergedConfigRules.Concatenate(Config::GetSingleton().GetParsers());
        return mergedConfigRules.GetMap();
    }

    void ParseKeybindingRule(const std::string& value, const std::string& /*configFileName*/) {
        try {
            uint32_t key = std::stoul(value, nullptr, 10);
            logger::info("Keybinding set to {}", key);
            Config::GetSingleton().GetKeyBinding() = key;
        } catch (std::exception& e) {
            logger::warn("Failed to parse keybinding value '{}': {}", value, e.what());
        }
    }

    bool ParseBoolString(const std::string& value) { return Utilities::ToLower(value) == "true" || value == "1"; }
}