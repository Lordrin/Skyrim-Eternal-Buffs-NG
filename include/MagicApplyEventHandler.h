#pragma once

#include "SpellUtilities.h"
#include "SpellLogging.h"

class MagicApplyEventHandler : public RE::BSTEventSink<RE::TESMagicEffectApplyEvent> {
private:

public:
    MagicApplyEventHandler() = default;
    static MagicApplyEventHandler& GetSingleton() {
        static MagicApplyEventHandler instance;
        return instance;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event,
                                          RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* source);
    static void Register();
};