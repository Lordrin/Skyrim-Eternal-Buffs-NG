#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "ConfigRules.h"
#include "ConfigLoader.h"

struct ReserveMagicka {
    RE::SpellItem* spellItem = nullptr;
    // RE::SpellItem* reserveSpellitem = nullptr;
    // float reserveCost = 0.0f;
};


class Config {
private:
    GeneralRule generalRule;
    const float permanentSpellDuration = 86313600.0f;  // 999 days
    OrderedMap<std::string, SpellRule> spellRules;
    std::unordered_map<RE::FormID, ReserveMagicka> reservedSpells;
    // TODO remove
    // std::vector<std::pair<RE::FormID, ReserveMagicka>> removedReservedSpells;
    uint32_t keyBinding = 42;
    bool toggleKeyHeld = false;
    RE::FormID reserveEffectFormID = 0;
    ConfigLoader configLoader;

public:
    Config() = default;
    static Config& GetSingleton() {
        static Config instance;
        return instance;
    }
    OrderedMap<std::string, SpellRule>& GetSpellRules() { return spellRules; }
    std::unordered_map<RE::FormID, ReserveMagicka>& GetReservedSpells() { return reservedSpells; }
    ConfigLoader &GetConfigLoader() { return configLoader; }
    // void SetConfigLoader(ConfigLoader&& configLoader) { this->configLoader = std::move(configLoader); }
    // TODO remove
    // std::vector<std::pair<RE::FormID, ReserveMagicka>>& GetRemovedReservedSpells() { return removedReservedSpells; }
    uint32_t& GetKeyBinding() { return keyBinding; }
    bool& GetToggleKeyHeld() { return toggleKeyHeld; }
    const float GetPermanentSpellDuration() { return permanentSpellDuration; }
    GeneralRule& GetGeneralRule() { return generalRule; }
    RE::FormID& GetReserveEffectFormID() { return reserveEffectFormID; }
    OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> GetParsers();
    const std::filesystem::path ConfigFilePath = "Data/SKSE/Plugins/EternalBuffsNG.ini";
};