#include "SpellDataPersistence.h"

#include "SpellCastEventHandler.h"

namespace SpellDataPersistence {
    void CacheSpellForSaving(RE::SpellItem* spell) {
        if (!spell) {
            return;
        }

        RE::FormID spellID = spell->GetFormID();
        if (spellID == 0) {
            return;  // Invalid spell ID
        }

        // Lock the mutex before modifying the map
        std::lock_guard lock(g_dataMutex);

        // Check if we already have data for this spell (prevents re-processing)
        if (g_savedSpellData.contains(spellID)) {
            logger::debug("Spell {:#010x} already cached.", spellID);
            return;
        }

        std::vector<RE::FormID> effectIDs;
        for (RE::Effect* effect : spell->effects) {
            if (effect && effect->baseEffect) {
                effectIDs.push_back(effect->baseEffect->GetFormID());
            }
        }

        g_savedSpellData[spellID] = std::move(effectIDs);  // Use move for efficiency
        logger::debug("Cached spell {:#010x} ('{}') with {} effects for saving.", spellID, spell->GetName(),
                        g_savedSpellData[spellID].size());
    }

    const SpellEffectsMap& GetAllSavedSpells() {
        // Simply return a const reference to the internal map.
        // The mutex protects writes (Load, Revert, Cache), making reads
        // generally safe *if* they don't happen precisely concurrently
        // with a write without external synchronization.
        // For typical usage (reading after load), this is fine.
        return g_savedSpellData;
    }

    bool IsSpellSaved(RE::FormID spellFormID) {
        std::lock_guard lock(g_dataMutex);
        return g_savedSpellData.contains(spellFormID);
    }

    bool RemoveSpellFromSave(RE::FormID spellFormID) {
        std::lock_guard lock(g_dataMutex);
        return g_savedSpellData.erase(spellFormID) > 0;
    }

    // --- Serialization Callbacks ---

    // Called when the game saves
    void SaveCallback(SKSE::SerializationInterface* skse) {
        logger::debug("SpellDataPersistence: SaveCallback triggered.");

        std::lock_guard lock(g_dataMutex);  // Lock for reading the map

        if (!skse->OpenRecord(kDataKey, kDataVersion)) {
            logger::error("SpellDataPersistence: Failed to open record for saving.");
            return;
        }

        // 1. Write the number of spells we are saving
        const size_t numSpells = g_savedSpellData.size();
        if (!skse->WriteRecordData(&numSpells, sizeof(numSpells))) {
            logger::error("SpellDataPersistence: Failed to write spell count.");
            return;
        }
        logger::debug("SpellDataPersistence: Saving {} spells.", numSpells);

        // 2. Iterate and write each spell and its effects
        for (const auto& [spellID, effectIDs] : g_savedSpellData) {
            // Write Spell FormID
            if (!skse->WriteRecordData(&spellID, sizeof(spellID))) {
                logger::error("SpellDataPersistence: Failed to write spell ID {:#010x}.", spellID);
                continue;  // Skip this spell if write fails
            }

            logger::debug("SpellDataPersistence: Saving spell {:#010x}.", spellID);

            // Write number of effects for this spell
            const size_t numEffects = effectIDs.size();
            if (!skse->WriteRecordData(&numEffects, sizeof(numEffects))) {
                logger::error("SpellDataPersistence: Failed to write effect count for spell {:#010x}.", spellID);
                continue;  // Skip this spell
            }

            // Write each effect FormID
            for (const auto& effectID : effectIDs) {
                if (!skse->WriteRecordData(&effectID, sizeof(effectID))) {
                    logger::error("SpellDataPersistence: Failed to write effect ID {:#010x} for spell {:#010x}.",
                                     effectID, spellID);
                    // Maybe stop saving this spell's effects? Or just log and continue? Let's continue.
                }
            }
        }
        LogSpellSFromMap(g_savedSpellData);
        logger::debug("SpellDataPersistence: Finished saving spell data.");
    }

    // Called when the game loads a save (before RevertCallback)
    void LoadCallback(SKSE::SerializationInterface* skse) {
        logger::debug("SpellDataPersistence: LoadCallback triggered.");

        uint32_t type;
        uint32_t version;
        uint32_t length;

        // Clear old data first
        {
            std::lock_guard lock(g_dataMutex);
            g_savedSpellData.clear();
        }

        // Look for our record type
        while (skse->GetNextRecordInfo(type, version, length)) {
            if (type == kDataKey) {
                logger::debug("SpellDataPersistence: Found our data record (Version {}).", version);

                if (version != kDataVersion) {
                    logger::error(
                        "SpellDataPersistence: Found data with incompatible version {}. Expected {}. Cannot load.",
                        version, kDataVersion);
                    // Optionally, add code here to handle loading older versions if needed later.
                    continue;  // Skip this record
                }

                // Lock for writing to the map
                std::lock_guard lock(g_dataMutex);

                // 1. Read the number of spells
                size_t numSpells = 0;
                if (!skse->ReadRecordData(&numSpells, sizeof(numSpells))) {
                    logger::error("SpellDataPersistence: Failed to read spell count.");
                    g_savedSpellData.clear();  // Ensure map is empty on error
                    return;                    // Stop loading
                }
                logger::debug("SpellDataPersistence: Loading {} spells.", numSpells);

                // 2. Iterate and read each spell and its effects
                for (size_t i = 0; i < numSpells; ++i) {
                    RE::FormID baseSpellId = 0;
                    // RE::FormID currentSpellID = 0;
                    size_t numEffects = 0;
                    std::vector<RE::FormID> currentEffectIDs;

                    // Read Spell FormID
                    if (!skse->ReadRecordData(&baseSpellId, sizeof(baseSpellId))) {
                        logger::error("SpellDataPersistence: Failed to read spell ID for spell #{}.", i + 1);
                        g_savedSpellData.clear();  // Abort loading
                        return;
                    }

                    logger::debug("SpellDataPersistence: Loading spell {:#010x}.", baseSpellId);

                    // Read number of effects
                    if (!skse->ReadRecordData(&numEffects, sizeof(numEffects))) {
                        logger::error("SpellDataPersistence: Failed to read effect count for spell {:#010x}.",
                                         baseSpellId);
                        g_savedSpellData.clear();  // Abort loading
                        return;
                    }

                    currentEffectIDs.reserve(numEffects);  // Pre-allocate vector space

                    // Read each effect FormID
                    for (size_t j = 0; j < numEffects; ++j) {
                        RE::FormID currentEffectID = 0;
                        if (!skse->ReadRecordData(&currentEffectID, sizeof(currentEffectID))) {
                            logger::error("SpellDataPersistence: Failed to read effect ID #{} for spell {:#010x}.",
                                             j + 1, currentEffectID);
                            g_savedSpellData.clear();  // Abort loading
                            return;
                        }
                        currentEffectIDs.push_back(currentEffectID);
                    }

                    // Store the loaded data
                    g_savedSpellData[baseSpellId] = std::move(currentEffectIDs);
                }
                LogSpellSFromMap(g_savedSpellData);
                logger::debug("SpellDataPersistence: Finished loading {} spells.", g_savedSpellData.size());
                return;
            } else {
                // This record is not ours, skip its data block
                logger::debug("SpellDataPersistence: Skipping unknown record type {:#010x}", type);
                if (!skse->ReadRecordData(nullptr, length)) {  // Read and discard
                    logger::error("SpellDataPersistence: Failed to skip unknown record data.");
                    // This might indicate save corruption, but we can try to continue.
                }
            }
        }
        logger::debug("SpellDataPersistence: No data record found in this save.");
    }

    // Called when the game reverts to a previous save (e.g., Load -> Load older)
    void RevertCallback(SKSE::SerializationInterface* /*skse*/) {
        logger::debug("SpellDataPersistence: RevertCallback triggered. Clearing cached spell data.");
        std::lock_guard lock(g_dataMutex);
        g_savedSpellData.clear();
    }

    // --- Registration Function ---
    // Call this from your main plugin load function or message listener
    bool RegisterSerializationCallbacks() {
        auto* skse = SKSE::GetSerializationInterface();
        if (!skse) {
            logger::critical("SpellDataPersistence: Failed to get SerializationInterface.");
            return false;
        }

        // Use the plugin's unique ID assigned by SKSE as the save signature
        skse->SetUniqueID(kDataKey);
        skse->SetSaveCallback(SaveCallback);
        skse->SetLoadCallback(LoadCallback);
        skse->SetRevertCallback(RevertCallback);

        logger::debug("SpellDataPersistence: Serialization callbacks registered with key {:#010x}.", kDataKey);
        return true;
    }

    const std::vector<RE::FormID> FlattenSpellEffectsMap(const SpellEffectsMap& spellEffectsMap) {
        std::vector<RE::FormID> flattened;

        for (const auto& [spellID, effectIDs] : spellEffectsMap) {
            flattened.insert(flattened.end(), effectIDs.begin(), effectIDs.end());
        }

        return flattened;
    }

    const std::unordered_set<RE::FormID> FlattenSpellEffectsMapToSet(const SpellEffectsMap& spellEffectsMap) {
        std::unordered_set<RE::FormID> flattened;

        for (const auto& [spellID, effectIDs] : spellEffectsMap) {
            flattened.insert(effectIDs.begin(), effectIDs.end());
        }

        return flattened;
    }
}

void SpellDataPersistence::LogSpellSFromMap(const SpellEffectsMap& spellEffectsMap) {
    if(spdlog::get_level() < spdlog::level::debug) {
        return;
    }
    
    if (spellEffectsMap.empty()) {
        logger::debug(" SpellEffectsMap map is currently empty. No data loaded or cached.");
        logger::debug("--- Finished Logging Spell Data ---");
        return;
    }

    logger::debug("  Found data for {} spells:", spellEffectsMap.size());

    int spellCount = 0;
    for (const auto& [spellID, effectIDs] : spellEffectsMap) {
        spellCount++;
        RE::TESForm* spellForm = RE::TESForm::LookupByID(spellID);
        RE::SpellItem* spellItem = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
        std::string spellName = spellItem && spellItem->GetName() ? spellItem->GetName() : "Unknown/Lookup Failed";

        logger::debug("  {}. Spell ID: {:#010x} ('{}')", spellCount, spellID, spellName);

        if (effectIDs.empty()) {
            logger::debug("      - No associated effect IDs recorded.");
        } else {
            logger::debug("      - Associated Effect IDs ({}):", effectIDs.size());
            int effectCount = 0;
            for (RE::FormID effectID : effectIDs) {
                effectCount++;
                RE::EffectSetting* mgef = RE::TESForm::LookupByID<RE::EffectSetting>(effectID);
                std::string effectName = mgef && mgef->GetName() ? mgef->GetName() : "Unknown/Lookup Failed";
                logger::debug("        {}. Effect ID: {:#010x} ('{}')", effectCount, effectID, effectName);
            }
        }
    }

    logger::debug("--- Finished Logging Spell Data ({} spells processed) ---", spellEffectsMap.size());
}