#include "ConfigRules.h"

#include "ConfigParser.h"
#include "SpellApplication.h"

OrderedMap<std::string, RuleVariant> BaseRule::GetFields() {
    return {{"sourceFile", &sourceFile},       {"resolvedForm", resolvedForm},
            {"nameFilter", &nameFilter},       {"isPermanentEnabled", &isPermanentEnabled},
            {"keywordFilter", &keywordFilter}, {"toggleable", &toggleable},
            {"pluginFilter", &pluginFilter}};
}

// Parsers that convert a std::string to the appropriate type
// and assign it to the corresponding member variable.
OrderedMap<std::string, std::function<void(const std::string&)>> BaseRule::GetParsers() {
    return {
        {"identifier",
         [this](const std::string& value) {
             RuleIdentifierResult result = Parser::ResolveIdentifier(value, sourceFile);
             switch (result.identifierType) {
                 case RuleIdentifierType::kForm:
                     resolvedForm = std::get<RE::TESForm*>(result.variantResult);
                     nameFilter = resolvedForm->GetName();
                     break;
                 case RuleIdentifierType::kPlugin:
                     pluginFilter = std::get<std::string>(result.variantResult);
                     nameFilter = pluginFilter;
                     break;
                 case RuleIdentifierType::kName:
                     nameFilter = std::get<std::string>(result.variantResult);
                     break;
                 case RuleIdentifierType::kInvalid:
                     logger::error("Invalid form identifier: {}", value);
                     break;
                 default:
                     logger::error("Error parsing form identifier: {}", value);
                     break;
             }
         }},
        {"isPermanentEnabled",
         [this](const std::string& value) { std::istringstream(value) >> std::boolalpha >> isPermanentEnabled; }},
        {"toggleable", [this](const std::string& value) { std::istringstream(value) >> std::boolalpha >> toggleable; }},
        {"keywordFilter", [this](const std::string& value) { keywordFilter = StringUtilities::SplitString(value, ','); }}};
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
    // const auto fields = GetFields();
    return fmt::format(
        "BaseRule: sourceFile = {}, FormName = {}, nameFilter = {}, isPermanentEnabled = {}, keywordFilter = {}, "
        "pluginFilter = {}, toggleable = {}",
        sourceFile, resolvedForm ? resolvedForm->GetName() : "nullptr", nameFilter, isPermanentEnabled,
        StringUtilities::Join(keywordFilter, ", "), pluginFilter, toggleable);
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
        {{"shouldReserveMagicka",
          [this](const std::string& value) { shouldReserveMagicka = Parser::ParseBoolString(value); }},
         {"durationFilter",
          [this](const std::string& value) {
              durationFilter = Parser::ParseBoolString(value);
              ;
          }},
         {"minDurationFilter",
          [this](const std::string& value) {
              minDurationFilter = Parser::ParseBoolString(value);
              ;
          }},
         {"magnitudeFilter", [this](const std::string& value) {
              magnitudeFilter = Parser::ParseBoolString(value);
              ;
          }}});
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

    // // TODO: refactor this and change to debug
    // if (!shouldReserveMagicka.has_value()) {
    //     if (Config::GetSingleton().GetGeneralRule().reserveMagickaEnabled) {
    //         logger::info("ApplyRulesToSpell: Should reserve magicka cost global");
    //         if (auto spellItem = activeEffect->spell->As<RE::SpellItem>()) {
    //             ApplyReserveSpellToPlayer(spellItem);
    //         }
    //     }
    // } else if (shouldReserveMagicka.value()) {
    //     logger::info("ApplyRulesToSpell: Should reserve magicka: cost");
    //     if (auto spellItem = activeEffect->spell->As<RE::SpellItem>()) {
    //         ApplyReserveSpellToPlayer(spellItem);
    //     }
    // }
}

std::string SpellRule::ToString() const {
    std::string toLog = BaseRule::ToString();
    return fmt::format("{} -- SpellRule: durationFilter = {}, minDurationFilter = {}, magnitudeFilter = {}", toLog,
                       durationFilter, minDurationFilter, magnitudeFilter, sourceFile,
                       resolvedForm ? resolvedForm->GetName() : "nullptr", nameFilter, isPermanentEnabled,
                       StringUtilities::Join(keywordFilter, ", "));
}

SpellRule GetSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(StringUtilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
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
bool FindSpellRuleForActiveEffect(RE::ActiveEffect* activeEffect, SpellRule& spellRule) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(StringUtilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        spellRule = spellRuleIt->second;
        RE::SpellItem* spellItem = (activeEffect->spell)->As<RE::SpellItem>();
        if (spellItem && spellRule.IsCorrectRuleToSpell(spellItem)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Finds the SpellRule for a spell by plugin name.
 * @param pluginName The plugin name to find the SpellRule for.
 * @param spellRule The SpellRule to populate if found.
 * @return True if the SpellRule was found, false otherwise.
 * @note This function searches for the SpellRule by the plugin name. If found, the SpellRule is copied into the
 * spellRule parameter.
 */
bool FindSpellRuleForSpellByPluginName(const std::string& pluginName, SpellRule& spellRule) {
    auto spellRuleIt = Config::GetSingleton().GetSpellRules().find(pluginName);
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        spellRule = spellRuleIt->second;
        return true;
    }
    return false;
}

bool FindSpellRuleForSpellItem(RE::SpellItem* spellItem, SpellRule& spellRule) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(StringUtilities::RemoveWhitespace(spellItem->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        spellRule = spellRuleIt->second;
        if (spellRule.IsCorrectRuleToSpell(spellItem)) {
            return true;
        }
    }
    return false;
}

bool IsSpellRuleDefined(RE::SpellItem* spellItem) {
    auto spellRuleIt =
        Config::GetSingleton().GetSpellRules().find(StringUtilities::RemoveWhitespace(spellItem->GetFullName()));
    if (spellRuleIt != Config::GetSingleton().GetSpellRules().end()) {
        if (spellRuleIt->second.IsCorrectRuleToSpell(spellItem)) {
            return true;
        }
    }
    return false;
}

OrderedMap<std::string, bool*> GeneralRule::GetFields() {
    return {
        {"enabled", &enabled},
        {"shoutsEnabled", &shoutsEnabled},
        {"spellsEnabled", &spellsEnabled},
        {"summonsEnabled", &summonsEnabled},
        {"lesserPowersEnabled", &lesserPowersEnabled},
        {"greaterPowersEnabled", &greaterPowersEnabled},
        {"scrollsEnabled", &scrollsEnabled},
        {"recastableEnabled", &recastableEnabled},
        {"reserveMagickaEnabled", &reserveMagickaEnabled},
    };
}

OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> GeneralRule::GetParsers() {
    return {
        {"enable", [this](const std::string& value, const std::string&) { enabled = Parser::ParseBoolString(value); }},
        {"shouts",
         [this](const std::string& value, const std::string&) { shoutsEnabled = Parser::ParseBoolString(value); }},
        {"spells",
         [this](const std::string& value, const std::string&) { spellsEnabled = Parser::ParseBoolString(value); }},
        {"summons",
         [this](const std::string& value, const std::string&) { summonsEnabled = Parser::ParseBoolString(value); }},
        {"lesserpowers", [this](const std::string& value,
                                const std::string&) { lesserPowersEnabled = Parser::ParseBoolString(value); }},
        {"greaterpowers", [this](const std::string& value,
                                 const std::string&) { greaterPowersEnabled = Parser::ParseBoolString(value); }},
        {"scrolls",
         [this](const std::string& value, const std::string&) { scrollsEnabled = Parser::ParseBoolString(value); }},
        {"recastable",
         [this](const std::string& value, const std::string&) { recastableEnabled = Parser::ParseBoolString(value); }},
        {"reservemagicka", [this](const std::string& value,
                                  const std::string&) { reserveMagickaEnabled = Parser::ParseBoolString(value); }},
    };
}

std::optional<std::string_view> GeneralRule::ShouldReturnEarly(RE::SpellItem* spellItem) const {
    if (!enabled) {           // Check if the entire rule set is disabled
        return std::nullopt;  // Not disabled by this rule set
    }
    if (!spellItem) {
        return std::nullopt;  // Cannot check a null spell
    }

    for (const auto& check : checks) {
        if (check.checkFn(spellItem)) {
            bool categoryIsEnabled = true;

            if (check.isEnabledConfig != nullptr) {
                categoryIsEnabled = *check.isEnabledConfig;
            } else {
                // This is an unconditional check (like Concentration, FoodItem).
                // These *always* disable if the checkFn returns true.
                logger::debug("Spell [{}] matches unconditional disable rule: {}", spellItem->GetName(),
                              check.description);
                return check.description;  // Disable because it matched an unconditional rule
            }

            // If the category IS disabled and the spell matched...
            if (!categoryIsEnabled) {
                logger::debug("Spell [{}] disabled because category '{}' is disabled.", spellItem->GetName(),
                              check.description);
                return check.description;  // Disable
            }
            // If we get here: spell matched, but the corresponding category is enabled,
            // so this specific check doesn't disable the spell. Continue to the next check.
        }
    }

    // If no check resulted in disabling the spell
    return std::nullopt;
}

std::string GeneralRule::ToString() const {
    return fmt::format(
        "GeneralRule: enabled = {}, shoutsEnabled = {}, spellsEnabled = {}, summonsEnabled = {}, "
        "lesserPowersEnabled = {}, greaterPowersEnabled = {}, scrollsEnabled = {}, recastableEnabled = {}",
        enabled, shoutsEnabled, spellsEnabled, summonsEnabled, lesserPowersEnabled, greaterPowersEnabled,
        scrollsEnabled, recastableEnabled);
}
