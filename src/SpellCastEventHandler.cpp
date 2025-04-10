#include "SpellLogging.h"
#include "SpellApplication.h"
#include "SpellCastEventHandler.h"
#include "SpellDataPersistence.h"

SpellCastEventHandler::SpellCastEventHandler() : reserveSPellId_(0) {
    RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
    uint32_t localFormID = 0x00801;
    RE::TESForm* localForm = dataHandler->LookupForm(localFormID, "BlinkTeleport.esp");
    SKSE::log::info("Local form ID of blink lesser power: {}", localForm->formID);
    reserveSPellId_ = localForm->formID;
}

RE::BSEventNotifyControl SpellCastEventHandler::ProcessEvent(const RE::TESSpellCastEvent* event,
                                                             RE::BSTEventSource<RE::TESSpellCastEvent>* source) {
    // Basic Event Checks
    if (!event || !event->object || !event->spell) {
        return RE::BSEventNotifyControl::kContinue;
    }

    logger::info("SpellCastEventHandler: ProcessEvent called with event: {:#010x}", event->spell);

    // Check if Caster is Player
    if (!event->object->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Get Player Actor (needed for handle later)
    RE::Actor* playerActor = event->object->As<RE::Actor>();
    if (!playerActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // --- Get Spell Item ---
    RE::FormID spellFormID = event->spell;
    RE::SpellItem* spellItem = RE::TESForm::LookupByID<RE::SpellItem>(spellFormID);
    if (!spellItem) {
        // SKSE::log::debug("SpellCastEvent: Failed to lookup SpellItem for FormID {:#010x}", spellFormID);
        return RE::BSEventNotifyControl::kContinue;
    }

    LogSpellSFromMap(SpellDataPersistence::GetAllSavedSpells());  // Log all saved spells

    // LogAllSavedSpellData();

    const char* spellName = spellItem->GetName();
    SKSE::log::info("Player casting spell:");
    SKSE::log::info("  Name: {}", spellName);
    SKSE::log::info("  FormID: {:#010x}", spellItem->GetFormID());  // Log resolved FormID

    LogAllActiveEffectsOfSpell(spellItem);  // Log all active effects of the spell
    // --- Log MGEF Keywords ---
    LogKeywords(spellItem, "      ");  // Pass mgef and appropriate indent

    // --- NEW: Check for Concentration Spell ---
    if (spellItem->data.castingType == RE::MagicSystem::CastingType::kConcentration) {
        if (!spellName || spellName[0] == '\0') spellName = "Unnamed Spell";
        SKSE::log::info("Player casting concentration spell '{}' ({:#010x}). Skipping further checks.", spellName,
                        spellItem->GetFormID());
        return RE::BSEventNotifyControl::kContinue;  // Exit early for concentration spells
    }
    // --- End of Concentration Check ---
    RE::ActorHandle playerHandle = playerActor->GetHandle();

    // Package the data
    SpellCastInfo info{spellItem, playerHandle};

    // Schedule the CheckAppliedEffects function to run on the next UI update cycle
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        taskInterface->AddUITask([info]() {  // Use lambda to capture 'info'
            ConvertToPermanentEffectOnPlayer(info);
        });
    } else {
        SKSE::log::error("Failed to get TaskInterface, cannot schedule effect check.");
    }

    return RE::BSEventNotifyControl::kContinue;
}

void SpellCastEventHandler::Register() {
    auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (eventSource) {
        eventSource->AddEventSink<RE::TESSpellCastEvent>(&GetSingleton());
    }
}
