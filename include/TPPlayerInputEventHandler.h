#pragma once

#include "ConfigRules.h"

class TPPlayerInputEventHandler : public RE::BSTEventSink<RE::InputEvent*> {
private:

public:
    static TPPlayerInputEventHandler& GetSingleton() {
        static TPPlayerInputEventHandler instance;
        return instance;
    }
    virtual RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                                  RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
    static void Register();
};
