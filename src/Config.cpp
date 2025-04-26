#include "Config.h"

void Config::InitializeShoutSpellMap() {
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        logger::error("Failed to get TESDataHandler.");
        return;
    }

    for (auto* shout : dataHandler->GetFormArray<RE::TESShout>()) {
        if (!shout) {
            continue;
        }

        for (const auto& word : shout->variations) {
            if (word.spell) {
                shoutSpellMap[word.spell->GetFormID()] = shout;
            }
        }
    }
}

void Config::Initialize() {
    InitializeShoutSpellMap();
}
