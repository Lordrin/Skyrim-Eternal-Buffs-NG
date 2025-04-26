#pragma once

#include <string>
#include <unordered_map>

#include "ConfigRules.h"

class Config {
private:
    GeneralRule generalRule;
    const float permanentSpellDuration = 86313600.0f;  // 999 days
    std::unordered_map<std::string, SpellRule> spellRules;
    std::unordered_map<RE::FormID, RE::TESShout*> shoutSpellMap;
    uint32_t keyBinding = 43;
    bool toggleKeyHeld = false;
    void InitializeShoutSpellMap();

public:
    Config() = default;
    static Config& GetSingleton() {
        static Config instance;
        return instance;
    }
    std::unordered_map<std::string, SpellRule>& GetSpellRules() { return spellRules; }
    std::unordered_map<RE::FormID, RE::TESShout*>& GetShoutSpellMap() { return shoutSpellMap; }
    uint32_t GetKeyBinding() { return keyBinding; }
    bool GetToggleKeyHeld() { return toggleKeyHeld; }
    const float GetPermanentSpellDuration() { return permanentSpellDuration; }
    GeneralRule& GetGeneralRule() { return generalRule; }
    void Initialize();
};