#pragma once

#include "SpellApplication.h"
#include "SpellLogging.h"

class ActiveEffectEventHandler : public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent> {
public:
    static ActiveEffectEventHandler& GetSingleton() {
        static ActiveEffectEventHandler singleton;
        return singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActiveEffectApplyRemoveEvent* event,
                                          RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* source) override;

    static void Register();

private:
    ActiveEffectEventHandler() = default;
    ActiveEffectEventHandler(const ActiveEffectEventHandler&) = delete;
    ActiveEffectEventHandler(ActiveEffectEventHandler&&) = delete;

    ~ActiveEffectEventHandler() = default;

    ActiveEffectEventHandler& operator=(const ActiveEffectEventHandler&) = delete;
    ActiveEffectEventHandler& operator=(ActiveEffectEventHandler&&) = delete;
};