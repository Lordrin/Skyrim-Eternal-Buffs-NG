#include "ConfigRules.h"

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
    if (auto resolvedForm = TryResolveHexFormID(identifier, configFileName, dataHandler)) {
        return resolvedForm;
    }

    logger::info("Could not resolve identifier '{}' (config: {}). Treating identifier as a name.",
                 identifier, configFileName);
    return nullptr;
}

RE::TESForm* TryResolveFormIDPlugin(const std::string& identifier, const std::string& configFileName, RE::TESDataHandler* dataHandler) {
    auto formPluginPair = Utilities::SplitString(identifier, '~');
    if (formPluginPair.size() == 2) {
        try {
            RE::FormID localFormID = std::stoul(Utilities::TrimString(formPluginPair[0]), nullptr, 16);
            std::string pluginName = Utilities::TrimString(formPluginPair[1]);
            RE::TESForm* resolvedForm = dataHandler->LookupForm(localFormID, pluginName);
            if (resolvedForm) {
                logger::info("Resolved FormID {:X} to '{}' (config: {})", localFormID, resolvedForm->GetName(),
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
        RE::TESForm* resolvedForm = RE::TESForm::LookupByID(std::stoul(Utilities::TrimString(identifier), nullptr, 16));
        if (resolvedForm) {
            logger::info("Resolved EditorID '{}' to FormID {:X} (config: {})", identifier,
                         resolvedForm->GetFormID(), configFileName);
            return resolvedForm;
        }
    } catch (...) { /* ignore format errors, try next method */
    }
    return nullptr;
}

RE::TESForm* TryResolveHexFormID(const std::string& identifier, const std::string& configFileName, RE::TESDataHandler* dataHandler) {
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
                // Add DLC checks if needed...
            }
        } catch (...) { /* Ignore parse errors */
        }
    }
    return nullptr;
}

// Implementation of BaseRule::GetFields
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
         [this](const std::string& value) { resolvedForm = ResolveIdentifier(value, sourceFile); }},
        {"nameFilter", [this](const std::string& value) { nameFilter = value; }},
        {"isPermanentEnabled",
         [this](const std::string& value) { std::istringstream(value) >> std::boolalpha >> isPermanentEnabled; }},
        {"keywordFilter", [this](const std::string& value) { keywordFilter = Utilities::SplitString(value, ','); }}
    };
}

// Implementing GetFields for SpellRule 
OrderedMap<std::string, RuleVariant> SpellRule::GetFields() {
    auto baseFields = BaseRule::GetFields();
    baseFields.Concatenate({{"durationFilter", &durationFilter},
                            {"minDurationFilter", &minDurationFilter},
                            {"magnitudeFilter", &magnitudeFilter}});
    return baseFields;
}

OrderedMap<std::string, std::function<void(const std::string&)>> SpellRule::GetParsers() {
    auto baseParsers = BaseRule::GetParsers();
    baseParsers.Concatenate({{"durationFilter", [this](const std::string& value) { std::istringstream(value) >> durationFilter; }},
                             {"minDurationFilter", [this](const std::string& value) { std::istringstream(value) >> minDurationFilter; }},
                             {"magnitudeFilter", [this](const std::string& value) { std::istringstream(value) >> magnitudeFilter; }}});
    return baseParsers;
}