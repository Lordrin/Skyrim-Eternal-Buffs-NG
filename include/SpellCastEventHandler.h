#pragma once

class SpellCastEventHandler : public RE::BSTEventSink<RE::TESSpellCastEvent> {
private:
    RE::TESForm* _reserveMagickaForm = nullptr;
    RE::TESForm* _reserveMagickaEffectForm = nullptr;
    const std::string _pluginName = "EternalBuffsNG.esp";

public:
    SpellCastEventHandler();
    static SpellCastEventHandler& GetSingleton() {
        static SpellCastEventHandler instance;
        return instance;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event,
                                          RE::BSTEventSource<RE::TESSpellCastEvent>* source);
    static void Register();
};