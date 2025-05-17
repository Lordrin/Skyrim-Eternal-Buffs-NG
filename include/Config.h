#pragma once

#include <string>
#include <unordered_map>

#include "ConfigRules.h"

struct ReserveMagicka {
    // RE::FormID spellID = 0;
    // RE::FormID spellCastID = 0;
    // uint32_t magicka = 0;
    RE::SpellItem* spellItem = nullptr;
    RE::SpellItem* reserveSpellitem = nullptr;
    // uint32_t magnitude = 0;
    // uint32_t cost = 0;
};


class Config {
private:
    GeneralRule generalRule;
    const float permanentSpellDuration = 86313600.0f;  // 999 days
    std::unordered_map<std::string, SpellRule> spellRules;
    std::unordered_map<std::string, ReserveMagicka> reservedSpells;
    uint32_t keyBinding = 42;
    bool toggleKeyHeld = false;
    RE::FormID reserveEffectFormID = 0;

public:
    Config() = default;
    static Config& GetSingleton() {
        static Config instance;
        return instance;
    }
    std::unordered_map<std::string, SpellRule>& GetSpellRules() { return spellRules; }
    std::unordered_map<std::string, ReserveMagicka>& GetReservedSpells() { return reservedSpells; }
    uint32_t& GetKeyBinding() { return keyBinding; }
    bool& GetToggleKeyHeld() { return toggleKeyHeld; }
    const float GetPermanentSpellDuration() { return permanentSpellDuration; }
    GeneralRule& GetGeneralRule() { return generalRule; }
    RE::FormID& GetReserveEffectFormID() { return reserveEffectFormID; }
    OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> GetParsers();
};