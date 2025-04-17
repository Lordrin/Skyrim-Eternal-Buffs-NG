#pragma once // Use #pragma once for modern header guards

#include <map>
#include <mutex>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include "SpellLogging.h"
#include "OrderedMap.h"

using SpellEffectsMap = std::map<RE::FormID, std::vector<RE::FormID>>;

// Pointers here so it can be changed later
using RuleVariant = std::variant<std::string*, RE::TESForm*, bool*, uint32_t*>;

// Shared Base Structure
struct BaseRule {
    std::string sourceFile;
    // std::string rawResolvedForm;
    RE::TESForm* resolvedForm = nullptr; // Optional for resolved rules
    std::string nameFilter;
    // std::string rawIsPermanentEnabled;
    bool isPermanentEnabled = true; // Optional for resolved rules
    std::string keywordFilter;

    // // Map of field names to references (for raw rules)
    // OrderedMap<std::string, std::string&> GetFields() {
    //     return {
    //         { "sourceFile", sourceFile },
    //         { "resolvedForm", rawResolvedForm },
    //         { "nameFilter", nameFilter },
    //         { "isPermanentEnabled", rawIsPermanentEnabled },
    //         { "keywordFilter", keywordFilter }
    //     };
    // }
    // Map of field names to references (for raw rules)
    OrderedMap<std::string, RuleVariant> GetFields() {
        return {
            { "sourceFile", &sourceFile },
            { "resolvedForm", resolvedForm },
            { "nameFilter", &nameFilter },
            { "isPermanentEnabled", &isPermanentEnabled },
            { "keywordFilter", &keywordFilter }
        };
    }

    OrderedMap<std::string, std::function<bool(const std::string&)>> configSettings = {
        { "sourceFile", [this](const std::string& value) { sourceFile = Utilities::TrimString(value); return !value.empty(); } },
        { "resolvedForm", [](const std::string& value) { return !value.empty(); } },
        { "nameFilter", [](const std::string& value) { return !value.empty(); } },
        { "isPermanentEnabled", [](const std::string& value) { return value == "true" || value == "false"; } },
        { "keywordFilter", [](const std::string& value) { return !value.empty(); } }
    };
};

// EffectRule Structure
struct EffectRule : BaseRule {
    uint32_t durationFilter;
    uint32_t minDurationFilter;
    uint32_t magnitudeFilter;

    // Map of field names to references (for raw rules)
    OrderedMap<std::string, RuleVariant> GetFields() {
        auto baseFields = BaseRule::GetFields();
        baseFields.Concatenate({
            { "durationFilter", &durationFilter },
            { "minDurationFilter", &minDurationFilter },
            { "magnitudeFilter", &magnitudeFilter }
        });
        return baseFields;
    }
};

// SpellRule Structure
struct SpellRule : BaseRule {
    uint32_t durationFilter;
    uint32_t minDurationFilter;
    uint32_t magnitudeFilter;

    // Map of field names to references (for raw rules)
    OrderedMap<std::string, RuleVariant> GetFields() {
        auto baseFields = BaseRule::GetFields();
        baseFields.Concatenate({
            { "durationFilter", &durationFilter },
            { "minDurationFilter", &minDurationFilter },
            { "magnitudeFilter", &magnitudeFilter }
        });
        return baseFields;
    }
};

namespace SpellDataPersistence {
    // Stores a map where:
    // Key = SpellItem FormID
    // Value = Vector of EffectSetting (MGEF) FormIDs associated with that spell

    // Our runtime storage for the data
    static SpellEffectsMap g_savedSpellData;
    // Mutex to protect access if multiple threads could modify it (safer practice)
    static std::mutex g_dataMutex;

    // --- Constants for Serialization ---
    constexpr uint32_t  kDataKey = 'LRDN'; // plugin's unique ID
    constexpr uint32_t kDataVersion = 1;

    extern std::vector<EffectRule> effectRules;
    extern std::unordered_map<std::string, SpellRule> spellRules;

    // Logs the attributes of BaseRule
    void LogBaseRule(const BaseRule& rule);

    // Logs the attributes of EffectRule
    void LogEffectRule(const EffectRule& rule);

    // Logs the attributes of SpellRule
    void LogSpellRule(const SpellRule& rule);

    // Logs the attributes of SpellEffectsMap
    void LogSpellSFromMap(const SpellEffectsMap& spellEffectsMap);


    /**
     * @brief Caches the given spell and its associated magic effect FormIDs
     *        to be included in the next SKSE co-save.
     *
     * @param spell A pointer to the RE::SpellItem to cache. If null or invalid, the function does nothing.
     */
    void CacheSpellForSaving(RE::SpellItem* spell);

    /**
     * @brief Registers the necessary SKSE Serialization callbacks
     *        to handle saving and loading of cached spell data.
     *
     * Must be called after the SKSE SerializationInterface is available,
     * typically during plugin initialization (e.g., kPostLoad or kDataLoaded message).
     *
     * @return true if registration was successful, false otherwise (e.g., if SKSE interface is unavailable).
     */
    bool RegisterSerializationCallbacks();

    const SpellEffectsMap& GetAllSavedSpells();

    bool IsSpellSaved(RE::FormID spellFormID);

    bool RemoveSpellFromSave(RE::FormID spellFormID);

    const std::vector<RE::FormID> FlattenSpellEffectsMap(const SpellEffectsMap& spellEffectsMap);
    const std::unordered_set<RE::FormID> FlattenSpellEffectsMapToSet(const SpellEffectsMap& spellEffectsMap);

    /**
     * @brief Populates the reversed map (g_reversedSpellData) based on g_savedSpellData.
     */
    // void PopulateReversedSpellData();

} // namespace SpellDataPersistence

