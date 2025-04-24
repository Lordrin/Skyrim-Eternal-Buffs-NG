#pragma once

class SpellCastEventHandler : public RE::BSTEventSink<RE::TESSpellCastEvent> {
private:

public:
    SpellCastEventHandler() = default;
    static SpellCastEventHandler& GetSingleton() {
        static SpellCastEventHandler instance;
        return instance;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event,
                                          RE::BSTEventSource<RE::TESSpellCastEvent>* source);
    static void Register();
};