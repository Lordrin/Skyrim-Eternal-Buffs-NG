#include "ConfigRules.h"

namespace Global {
    // The key is the name of the spell
    std::unordered_map<std::string, SpellRule> spellRules;

    std::unordered_map<std::string, SpellRule>& GetSpellRules() { return spellRules; }

    std::unordered_map<RE::FormID, RE::TESShout*> shoutSpellMap;

    std::unordered_map<RE::FormID, RE::TESShout*>& GetShoutSpellMap() { return shoutSpellMap; }

    void InitializeShoutSpellMap() {
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("Failed to get TESDataHandler.");
            return;
        }

        for (auto* shout : dataHandler->GetFormArray<RE::TESShout>()) {
            if (!shout) {
                continue;
            }

            for (const auto& word : shout->variations) {
                if (word.spell) {
                    shoutSpellMap[word.spell->GetFormID()] = shout;
                }
            }
        }
    }

    GeneralRule generalRule;

}

namespace Parser {

    RE::TESForm* ResolveIdentifier(const std::string& identifier, const std::string& configFileName) {
        RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("TESDataHandler not found.");
            return nullptr;
        }

        // Step 1: Try FormID~Plugin
        if (auto resolvedForm = TryResolveFormIDPlugin(identifier, configFileName, dataHandler)) {
            return resolvedForm;
        }

        // Step 2: Try EditorID
        if (auto resolvedForm = TryResolveEditorID(identifier, configFileName)) {
            return resolvedForm;
        }

        // Step 3: Try Hex FormID
        if (auto resolvedForm = TryResolveHexFormID(identifier, dataHandler)) {
            return resolvedForm;
        }

        logger::info("Could not resolve identifier '{}' (config: {}). Treating identifier as a name.", identifier,
                     configFileName);
        return nullptr;
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
                    logger::debug("Resolved FormID {:#010x} to '{}' (config: {})", localFormID, resolvedForm->GetName(),
                                  configFileName);
                    return resolvedForm;
                } else {
                    logger::warn("FormID {:X} not found in plugin '{}' (config: {})", localFormID, pluginName,
                                 configFileName);
                }
            } catch (...) { /* ignore format errors, try next method */
            }
        }
        return nullptr;
    }

    RE::TESForm* TryResolveEditorID(const std::string& identifier, const std::string& configFileName) {
        try {
            RE::TESForm* resolvedForm =
                RE::TESForm::LookupByID(std::stoul(Utilities::TrimString(identifier), nullptr, 16));
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
                    // Optional: Explicitly check common masters
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
            return {};  // Return an empty SpellRule if no parts are found
        }

        // Construct the vector in the correct order
        std::vector<std::string> orderedParts;
        orderedParts.reserve(parts.size() + 2);  // Reserve space for configFileName and identifier
        orderedParts.push_back(configFileName);  // Add configFileName as the first part
        orderedParts.push_back(parts[0]);        // Add the identifier as the second part
        orderedParts.insert(orderedParts.end(), std::make_move_iterator(parts.begin()),
                            std::make_move_iterator(parts.end()));  // Move the rest of the parts

        logger::debug("valueString: {}", configLine);
        logger::debug("parts: {}", Utilities::Join(orderedParts, "|"));

        SpellRule spellRule;
        ParseSplitLine(orderedParts, spellRule.GetParsers().GetOrder(), spellRule.GetParsers().GetMap());

        if (spellRule.resolvedForm) {
            logger::info("Resolved FormID {:#010x} to '{}' (config: {})", spellRule.resolvedForm->GetFormID(),
                         spellRule.resolvedForm->GetName(), configFileName);
            spellRule.nameFilter = spellRule.resolvedForm->GetName();  // Set the nameFilter to the resolved form's name
        }

        Global::spellRules.insert({Utilities::RemoveWhitespace(spellRule.nameFilter), spellRule});

        auto to_print = spellRule.ToString();
        logger::debug("{}", to_print);
        return spellRule;
    }

    void ParseEnableRule(const std::string& value, const std::string& /*configFileName*/) {
        Global::generalRule.enabled = Utilities::ToLower(value) == "true" || value == "1";
    }
    void ParseShoutsEnabledRule(const std::string& value, const std::string& /*configFileName*/) {
        Global::generalRule.shoutsEnabled = Utilities::ToLower(value) == "true" || value == "1";
    }
    void ParseSpellsEnabledRule(const std::string& value, const std::string& /*configFileName*/) {
        Global::generalRule.spellsEnabled = Utilities::ToLower(value) == "true" || value == "1";
    }

    void ParseLoggingLevelRule(const std::string& value, const std::string& configFileName) {
        auto loggingLevel = Utilities::ToLower(Utilities::TrimString(value));
        if (loggingLevel == "trace") {
            spdlog::set_level(spdlog::level::trace);
        } else if (loggingLevel == "off") {
            spdlog::set_level(spdlog::level::off);
        } else if (loggingLevel == "critical") {
            spdlog::set_level(spdlog::level::critical);
        } else if (loggingLevel == "debug") {
            spdlog::set_level(spdlog::level::debug);
        } else if (loggingLevel == "info") {
            spdlog::set_level(spdlog::level::info);
        } else if (loggingLevel == "warn") {
            spdlog::set_level(spdlog::level::warn);
        } else if (loggingLevel == "error") {
            spdlog::set_level(spdlog::level::err);
        } else {
            logger::warn("Unknown logging level '{}' in config file '{}'. Defaulting to 'info'.", loggingLevel,
                         configFileName);
            spdlog::set_level(spdlog::level::info);
        }
        logger::info("Logging level set to '{}'", loggingLevel);
    }
}

OrderedMap<std::string, RuleVariant> BaseRule::GetFields() {
    return {{"sourceFile", &sourceFile},
            {"resolvedForm", resolvedForm},
            {"nameFilter", &nameFilter},
            {"isPermanentEnabled", &isPermanentEnabled},
            {"keywordFilter", &keywordFilter}};
}

// Parsers that convert a std::string to the appropriate type
// and assign it to the corresponding member variable.
OrderedMap<std::string, std::function<void(const std::string&)>> BaseRule::GetParsers() {
    return {
        {"sourceFile", [this](const std::string& value) { sourceFile = value; }},
        {"resolvedForm",
         [this](const std::string& value) { resolvedForm = Parser::ResolveIdentifier(value, sourceFile); }},
        {"nameFilter", [this](const std::string& value) { nameFilter = value; }},
        {"isPermanentEnabled",
         [this](const std::string& value) { std::istringstream(value) >> std::boolalpha >> isPermanentEnabled; }},
        {"keywordFilter", [this](const std::string& value) { keywordFilter = Utilities::SplitString(value, ','); }}};
}

bool BaseRule::ShouldApplyRuleToForm(RE::TESForm* form) const {
    if (resolvedForm == nullptr) {
        logger::debug("ShouldApplyRuleToForm: resolvedForm is null. Checking by name");
        return nameFilter == form->GetName();
    } else {
        return (resolvedForm == form || form->GetFormID() == resolvedForm->GetFormID());
    }
}

std::string BaseRule::ToString() const {
    return fmt::format(
        "BaseRule: sourceFile = {}, FormName = {}, nameFilter = {}, isPermanentEnabled = {}, keywordFilter = {}",
        sourceFile, resolvedForm ? resolvedForm->GetName() : "nullptr", nameFilter, isPermanentEnabled,
        Utilities::Join(keywordFilter, ", "));
}

OrderedMap<std::string, RuleVariant> SpellRule::GetFields() {
    auto baseFields = BaseRule::GetFields();
    baseFields.Concatenate_fast({{"durationFilter", &durationFilter},
                                 {"minDurationFilter", &minDurationFilter},
                                 {"magnitudeFilter", &magnitudeFilter}});
    return baseFields;
}

OrderedMap<std::string, std::function<void(const std::string&)>> SpellRule::GetParsers() {
    auto baseParsers = BaseRule::GetParsers();
    baseParsers.Concatenate_fast(
        {{"durationFilter", [this](const std::string& value) { std::istringstream(value) >> durationFilter; }},
         {"minDurationFilter", [this](const std::string& value) { std::istringstream(value) >> minDurationFilter; }},
         {"magnitudeFilter", [this](const std::string& value) { std::istringstream(value) >> magnitudeFilter; }}});
    return baseParsers;
}

bool SpellRule::ShouldApplyRuleToSpell(RE::SpellItem* spellItem) const {
    if (!spellItem) {
        logger::warn("ShouldApplyRuleToSpell: SpellItem is null.");
        return false;
    }
    if (spellItem->GetFormType() != RE::FormType::Spell) {
        logger::warn("ShouldApplyRuleToSpell: SpellItem is not a spell.");
        return false;
    }
    std::span<RE::BGSKeyword*> keywords = spellItem->GetKeywords();

    // check if keyworkFilter is empty or if any of the keywords match
    bool isRightSpell = BaseRule::ShouldApplyRuleToForm(spellItem);
    return isRightSpell && (keywordFilter.empty() ||
                            std::any_of(keywordFilter.begin(), keywordFilter.end(), [&](const std::string& keyword) {
                                return std::any_of(keywords.begin(), keywords.end(), [&](RE::BGSKeyword* spellKeyword) {
                                    return spellKeyword && spellKeyword->GetName() == keyword;
                                });
                            }));
}

void SpellRule::ApplySpellRulesToActiveEffect(RE::ActiveEffect* activeEffect) const {
    if (!activeEffect) {
        logger::warn("ApplyRulesToSpell: SpellItem is null.");
        return;
    }
    std::span<RE::BGSKeyword*> spellKeywords = activeEffect->spell->GetKeywords();

    // If the duration is less then the minDurationFilter - skip the effect
    if (activeEffect->duration <= minDurationFilter) {
        return;
    }

    if (isPermanentEnabled) {
        logger::debug("ApplyRulesToSpell: Setting duration to permanent for effect: {:#010x} ({})",
                      activeEffect->GetBaseObject()->GetFormID(), activeEffect->GetBaseObject()->GetName());
        activeEffect->duration = Global::permanentSpellDuration;
    }

    // check if durationfilter is not set to default
    if (durationFilter != -1.0f) {
        logger::debug("ApplyRulesToSpell: Setting duration to {} for effect: {:#010x} ({})", durationFilter,
                      activeEffect->GetBaseObject()->GetFormID(), activeEffect->GetBaseObject()->GetName());
        activeEffect->duration = durationFilter;  // Set the duration to the filter value
    }

    // check if minDurationFilter is not set to default
    if (magnitudeFilter != -1.0f) {
        activeEffect->magnitude = magnitudeFilter;  // Set the magnitude to the filter value
    }
}

std::string SpellRule::ToString() const {
    std::string toLog = BaseRule::ToString();
    return fmt::format("{} -- SpellRule: durationFilter = {}, minDurationFilter = {}, magnitudeFilter = {}", toLog,
                       durationFilter, minDurationFilter, magnitudeFilter, sourceFile,
                       resolvedForm ? resolvedForm->GetName() : "nullptr", nameFilter, isPermanentEnabled,
                       Utilities::Join(keywordFilter, ", "));
}
