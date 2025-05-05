#include "ConfigRules.h"

#include "ConfigParser.h"

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
        {"toggleable", [this](const std::string& value) { std::istringstream(value) >> std::boolalpha >> toggleable; }},
        {"keywordFilter", [this](const std::string& value) { keywordFilter = Utilities::SplitString(value, ','); }}};
}

/**
 * @brief Determines if a given RE::TESForm should have the rule applied to it.
 * @param form The RE::TESForm to check.
 * @return True if the rule should be applied, false otherwise.
 *
 * This function checks if the given form matches the rule's resolvedForm or nameFilter.
 * If the resolvedForm is null, it uses the nameFilter to check if the form's name matches.
 * If the resolvedForm is not null, it checks if the form is the same as the resolvedForm
 * or if the form's FormID is the same as the resolvedForm's FormID.
 */
bool BaseRule::IsCorrectRuleToForm(RE::TESForm* form) const {
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

bool SpellRule::IsCorrectRuleToSpell(RE::SpellItem* spellItem) const {
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
    bool isRightSpell = BaseRule::IsCorrectRuleToForm(spellItem);
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
        activeEffect->duration =
            Config::GetSingleton().GetPermanentSpellDuration();  // Set the duration to permanentSpellDuration;
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

SpellRule GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(Utilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        RE::SpellItem* spellItem = (activeEffect->spell)->As<RE::SpellItem>();
        if (spellItem && spellRuleIt->second.IsCorrectRuleToSpell(spellItem)) {
            return spellRuleIt->second;
        }
    }
    return SpellRule{};
}

/**
 * @brief Finds the SpellRule for an active effect.
 * @param activeEffect The active effect to find the SpellRule for.
 * @param spellRule The SpellRule to populate if found.
 * @return True if the SpellRule was found, false otherwise.
 * @note This function searches for the SpellRule by the full name of the
 * active effect's spell. If found, the SpellRule is copied into the spellRule
 * parameter.
 */
bool GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect, SpellRule& spellRule) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(Utilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        spellRule = spellRuleIt->second;
        RE::SpellItem* spellItem = (activeEffect->spell)->As<RE::SpellItem>();
        if (spellItem && spellRule.IsCorrectRuleToSpell(spellItem)) {
            return true;
        }
    }
    return false;
}

bool GetSpellRuleForSpellItem(RE::SpellItem* spellItem, SpellRule& spellRule) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(Utilities::RemoveWhitespace(spellItem->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        spellRule = spellRuleIt->second;
        if (spellRule.IsCorrectRuleToSpell(spellItem)) {
            return true;
        }
    }
    return false;
}

std::string GeneralRule::ToString() const {
    return fmt::format(
        "GeneralRule: enabled = {}, shoutsEnabled = {}, spellsEnabled = {}, summonsEnabled = {}, "
        "lesserPowersEnabled = {}, greaterPowersEnabled = {}",
        enabled, shoutsEnabled, spellsEnabled, summonsEnabled, lesserPowersEnabled, greaterPowersEnabled);
}
