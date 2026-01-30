#include "ReserveMagicka.h"

float CalculateMagickaForReserveSpell(RE::SpellItem* spellItem){
    if (!spellItem) {
        SKSE::log::error("Spell not found for temporary debuff application.");
        return;
    }

    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("Player not found for temporary debuff application.");
        return;
    }

    float spellCost = spellItem->CalculateMagickaCost(player);
    //TODO expand this with config settings

    return spellCost;

}

void ApplyReserveSpellToPlayer(RE::SpellItem* spellItem){

}

RE::Effect& CreateReserveSpellEffect(RE::SpellItem* spellItem){
    auto datahandler = RE::TESDataHandler::GetSingleton();
    if (!datahandler) {
        SKSE::log::error("DataHandler is null.");
    }

    RE::FormID reserveEffectFormID = Config::GetSingleton().GetReserveEffectFormID();
    if (reserveEffectFormID == 0) {
        logger::warn("ReserveEffectFormID is not set. Returning early.");
        return;
    }

    auto form = datahandler->LookupForm(reserveEffectFormID, "EternalBuffsNG.esp");
    if (!form) {
        SKSE::log::error("Custom spell was not found with FormID {:#010x}.", reserveEffectFormID);
        return;
    }

    // Create a dynamic spell instance
    RE::ConcreteFormFactory<RE::SpellItem, RE::FormType::Spell>* formFactory =
        RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();

    RE::SpellItem* dynamicCarrierSpell = nullptr;
    if (formFactory) {
        dynamicCarrierSpell = formFactory->Create();  // This gets an FFxxxxxx FormID
    } else {
        SKSE::log::error("Failed to get spell factory!");
        return;  // or handle error
    }

    if (!dynamicCarrierSpell) {
        SKSE::log::error("Failed to create dynamic carrier spell instance!");
        return;  // or handle error
    }

    // Configure this dynamic spell:
    dynamicCarrierSpell->data.spellType = RE::MagicSystem::SpellType::kSpell;  // Or kLesserPower, etc.
    dynamicCarrierSpell->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
    dynamicCarrierSpell->data.delivery = RE::MagicSystem::Delivery::kSelf;
    // std::string reserveSpellName = "";
    // std::string reserveSpellName = "Reserve Magicka - ";
    // reserveSpellName = spellItem->GetName();
    dynamicCarrierSpell->fullName =
        RE::BSFixedString(spellItem->GetName());  // Set a unique (even if internal) name for debugging if you want
    RE::EffectSetting* customMagicEffect = form->As<RE::EffectSetting>();
    if (!customMagicEffect) {
        SKSE::log::error("Spell is null.");
    }

    logger::info("Effect123: {}", customMagicEffect->fullName);
    // std::string newFullName = std::string(customMagicEffect->fullName.c_str()) + " - " +
    // spellToReserve->GetName(); customMagicEffect->fullName = RE::BSFixedString(newFullName.c_str());

    // Create a new RE::Effect instance.
    // IMPORTANT: Memory management for this RE::Effect object is crucial.
    // If the spell takes ownership, great. If not, you might need to manage it.
    // Often, when added to the spell's list and the spell is used, the game manages it.
    RE::Effect* newEffectItem =
        new RE::Effect();  // Allocate on the heap // The RE::SpellItem destructor iterates through its effects
    // array and deletes each RE::Effect* it contains.
    if (!newEffectItem) {
        SKSE::log::error("Failed to allocate RE::Effect item!");
        // Potentially delete dynamicSpell if it's not yet fully integrated
        delete newEffectItem;
        delete dynamicCarrierSpell;
        return;
    }

    return *newEffectItem;

}