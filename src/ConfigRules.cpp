#include "ConfigRules.h"

#include "ConfigParser.h"
// namespace Global {
//     // The key is the name of the spell
//     std::unordered_map<std::string, SpellRule> spellRules;

//     std::unordered_map<std::string, SpellRule>& GetSpellRules() { return spellRules; }

//     std::unordered_map<RE::FormID, RE::TESShout*> shoutSpellMap;

//     std::unordered_map<RE::FormID, RE::TESShout*>& GetShoutSpellMap() { return shoutSpellMap; }

//     void InitializeShoutSpellMap() {
//         auto dataHandler = RE::TESDataHandler::GetSingleton();
//         if (!dataHandler) {
//             logger::error("Failed to get TESDataHandler.");
//             return;
//         }

//         for (auto* shout : dataHandler->GetFormArray<RE::TESShout>()) {
//             if (!shout) {
//                 continue;
//             }

//             for (const auto& word : shout->variations) {
//                 if (word.spell) {
//                     shoutSpellMap[word.spell->GetFormID()] = shout;
//                 }
//             }
//         }
//     }

//     GeneralRule generalRule;

// }

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
        activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();  // Set the duration to permanentSpellDuration;
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
