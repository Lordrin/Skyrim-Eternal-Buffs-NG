#pragma once

#include <string>
#include <variant>
#include <functional>
#include <sstream>

#include "OrderedMap.h"
#include "StringUtilities.h"

// Pointers here so it can be changed later
using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*, std::vector<std::string>*>;

// Shared Base Structure
struct BaseRule {
    std::string sourceFile;
    RE::TESForm* resolvedForm = nullptr;
    std::string nameFilter;
    bool isPermanentEnabled = true; 
    std::vector<std::string> keywordFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    void Log() const {
        logger::info("BaseRule: sourceFile = {}, nameFilter = {}, isPermanentEnabled = {}, keywordFilter = {}",
                     sourceFile, nameFilter, isPermanentEnabled, Utilities::Join(keywordFilter, ", "));
    }
};

// SpellRule Structure
struct SpellRule : BaseRule {
    uint32_t durationFilter;
    uint32_t minDurationFilter;
    uint32_t magnitudeFilter;

    OrderedMap<std::string, RuleVariant> GetFields();
    OrderedMap<std::string, std::function<void(const std::string&)>> GetParsers();
    void Log() const {
        BaseRule::Log();
        logger::info("SpellRule: durationFilter = {}, minDurationFilter = {}, magnitudeFilter = {}",
                     durationFilter, minDurationFilter, magnitudeFilter);
    }
};

extern std::unordered_map<std::string, SpellRule> spellRules;