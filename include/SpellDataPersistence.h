      
// SpellDataPersistence.h
#pragma once // Use #pragma once for modern header guards

#include <map>
#include <vector>
#include <algorithm>  // For std::copy
#include <unordered_set>
#include <SpellLogging.h>

// Rules from the config file
// --- Shared Base Structure ---
struct BaseRule {
    std::string  sourceFile;
    RE::TESForm* resolvedForm = nullptr; // Common base pointer!
    std::string nameFilter;
    bool isPermanentEnabled = true;
    std::string keywordFilter;
};

// --- EffectRule Structure ---
struct EffectRule : BaseRule {
    std::string durationFilter;
    std::string minDurationFilter;
    std::string magnitudeFilter;
};

// Specific structure for Spell rules
struct SpellRule : BaseRule {
    std::string durationFilter;
    std::string minDurationFilter;
    std::string magnitudeFilter;
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

    