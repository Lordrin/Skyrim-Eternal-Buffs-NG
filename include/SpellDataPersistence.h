#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "ConfigLoader.h"
#include "OrderedMap.h"
#include "SpellLogging.h"
#include "StringUtilities.h"
#include "ConfigRules.h"

// struct SpellData {
//     std::vector<RE::FormID> spellEffects;
//     uint32_t cost = 0;
// };

// Stores a map where:
// Key = SpellItem FormID
// Value = Vector of EffectSetting (MGEF) FormIDs associated with that spell
using SpellEffectsMap = std::map<RE::FormID, std::vector<RE::FormID>>;

namespace SpellDataPersistence {
    // struct SpellData {
    //     SpellEffectsMap spellEffectsMap;
    //     uint32_t cost = 0;
    // };
    // Our runtime storage for the data
    static SpellEffectsMap g_savedSpellData;
    // Mutex to protect access if multiple threads could modify it (safer practice)
    static std::mutex g_dataMutex;

    // --- Constants for Serialization ---
    // constexpr uint32_t kDataKey = 'EBUF';  // plugin's unique ID
    constexpr uint32_t kDataKey = 'LRDN';  // plugin's unique ID
    constexpr uint32_t kDataVersion = 1; 

    // Logs the attributes of SpellEffectsMap
    void LogSpellSFromMap(const SpellEffectsMap& spellEffectsMap);

    /**
     * @brief Caches the given spell and its associated magic effect FormIDs
     *        to be included in the next SKSE co-save.
     *
     * @param spell A pointer to the RE::SpellItem to cache. If null or invalid, the function does nothing.
     */
    void CacheSpellForSaving(RE::SpellItem* spell, uint32_t spellCost = 0);

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

}
