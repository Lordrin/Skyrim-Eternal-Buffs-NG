#include "ReserveMagicka.h"

float CalculateMagickaForReserveSpell(RE::SpellItem* spellItem, RE::PlayerCharacter* player) {
    float spellCost = spellItem->CalculateMagickaCost(player);
    // TODO expand this with config settings

    return spellCost;
}

ReserveMagicka* AlreadyReservedSpell(RE::SpellItem* spellItem, RE::PlayerCharacter* player, float spellCost) {
    std::unordered_map<RE::FormID, ReserveMagicka>& reservedSpells = Config::GetSingleton().GetReservedSpells();

    auto AlreadyReservedspell = reservedSpells.find(spellItem->GetFormID());

    if (AlreadyReservedspell != reservedSpells.end()) {
        logger::info("Spell {} is already reserved. With cost {}", spellItem->GetName(), spellCost);
        logger::info("Already reserved spell cost {}", AlreadyReservedspell->second.reserveCost);
        return &AlreadyReservedspell->second;
        // if (AlreadyReservedspell->second.spellCost == spellCost) {
        //     SKSE::log::info("Spell {} is already reserved. Skipping.", spellItem->GetName());
        //     return;
        // } else {
        //     DispelSpellItemFromActor(player, AlreadyReservedspell->second.reserveSpellitem);
        // }
    }
    return nullptr;
}

bool HandleAlreadyReservedSpell(ReserveMagicka* AlreadyReservedspell, RE::SpellItem* spellItem,
                                RE::PlayerCharacter* player, float spellCost, bool isSuppressed) {
    if (AlreadyReservedspell->reserveCost == spellCost) {
        SKSE::log::info("Spell {} is already reserved. Skipping.", spellItem->GetName());
        // Should return early
        return true;
    } else {
        SpellUtilities::DispelSpellItemFromActor(player, AlreadyReservedspell->reserveSpellitem);
        return false;
    }
}

RE::TESForm& GetReserveSpellForm() {
    RE::FormID reserveEffectFormID = Config::GetSingleton().GetReserveEffectFormID();
    if (reserveEffectFormID == 0) {
        logger::warn("ReserveEffectFormID is not set. Returning early.");
        throw std::runtime_error("ReserveEffectFormID is not set.");
        // return;
    }

    auto datahandler = RE::TESDataHandler::GetSingleton();
    if (!datahandler) {
        SKSE::log::error("DataHandler is null.");
    }

    RE::TESForm* form = datahandler->LookupForm(reserveEffectFormID, "EternalBuffsNG.esp");
    if (!form) {
        SKSE::log::error("Custom spell was not found with FormID {:#010x}.", reserveEffectFormID);
        throw std::runtime_error("Custom spell was not found.");
        // TODO throw error here and catch later
        // return;
    }

    return *form;
}

RE::EffectSetting& GetReserveSpellEffectSetting() {
    RE::TESForm& form = GetReserveSpellForm();
    RE::EffectSetting* customMagicEffect = form.As<RE::EffectSetting>();
    if (!customMagicEffect) {
        SKSE::log::error("Spell is null.");
        throw std::runtime_error("Spell is null.");
        // return;  // TODO throw error here
    }
    return *customMagicEffect;
}

RE::SpellItem* CreateReserveSpellCarrier(RE::SpellItem* spellItem) {
    // Create a dynamic spell instance
    RE::ConcreteFormFactory<RE::SpellItem, RE::FormType::Spell>* formFactory =
        RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();

    RE::SpellItem* dynamicCarrierSpell = nullptr;
    if (formFactory) {
        dynamicCarrierSpell = formFactory->Create();  // This gets an FFxxxxxx FormID
    } else {
        SKSE::log::error("Failed to get spell factory!");
        throw std::runtime_error("Failed to get spell factory!");
        // return;  // or handle error
    }

    if (!dynamicCarrierSpell) {
        SKSE::log::error("Failed to create dynamic carrier spell instance!");
        throw std::runtime_error("Failed to create dynamic carrier spell instance!");
        // return;  // or handle error
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

    return dynamicCarrierSpell;
}

RE::Effect* CreateReserveSpellEffect() {
    // Create a new RE::Effect instance.
    // IMPORTANT: Memory management for this RE::Effect object is crucial.
    // If the spell takes ownership, great. If not, you might need to manage it.
    // Often, when added to the spell's list and the spell is used, the game manages it.
    RE::Effect* newEffectItem =
        new RE::Effect();  // Allocate on the heap // The RE::SpellItem destructor iterates through its effects
    // array and deletes each RE::Effect* it contains.
    if (!newEffectItem) {
        SKSE::log::error("Failed to allocate RE::Effect item!");
        // TODO throw error here and catch later
        //  Potentially delete dynamicSpell if it's not yet fully integrated
        // delete newEffectItem;
        // delete dynamicCarrierSpell;
        return nullptr;
    }

    return newEffectItem;
}
void LinkReserveSpellToEffect(RE::SpellItem* dynamicCarrierSpell, RE::Effect* effect, RE::Actor* player,
                              RE::SpellItem* spellItem, float reserveCost, bool isSuppressed) {
    try {
        RE::EffectSetting& customMagicEffect = GetReserveSpellEffectSetting();

        // Get the player's MagicCaster component for the desired hand (e.g., kRightHand)
        // Or if the spell is 'Self' delivery, a 'self' caster might be more appropriate,
        // but often using a hand caster works well for Fire and Forget spells.
        RE::MagicCaster* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
        if (!magicCaster) {
            // Try other hand or a default caster if right hand fails
            magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
            if (!magicCaster) {
                SKSE::log::warn("Could not get a valid MagicCaster for the player.");
                return;
            }
        }

        RE::MagicTarget* magicTarget = player->GetMagicTarget();
        if (!magicTarget) {
            SKSE::log::warn("Could not get a valid MagicTarget for the player.");
            return;
        }
        // TODO delete newEffectItem on error

        // Set the effect's parameters for this spell
        // newEffectItem->effectItem.magnitude = spellCost * -1.0f;
        effect->effectItem.magnitude = reserveCost;
        effect->effectItem.duration = static_cast<uint32_t>(Config::GetSingleton().GetPermanentSpellDuration());
        effect->effectItem.area = 0;              // 0 area for a self-target effect
        effect->baseEffect = &customMagicEffect;  // ** This is where you link your MGEF **

        dynamicCarrierSpell->effects.push_back(effect);
        // float cost = dynamicCarrierSpell->CalculateMagickaCost(player);

        magicCaster->CastSpellImmediate(dynamicCarrierSpell, true, player, 1.0f, false, effect->effectItem.magnitude,
                                        nullptr);

        logger::info("Reserved spell: {} applied with: {} - FormID {:#010x}", spellItem->GetName(),
                     dynamicCarrierSpell->GetName(), dynamicCarrierSpell->GetFormID());

        // Config::GetSingleton().GetReservedSpells().insert({spellToReserve, dynamicCarrierSpell});
        // auto& reservedSpells = Config::GetSingleton().GetReservedSpells();
        std::unordered_map<RE::FormID, ReserveMagicka>& reservedSpells = Config::GetSingleton().GetReservedSpells();
        reservedSpells.insert({spellItem->GetFormID(), {spellItem, dynamicCarrierSpell, reserveCost}});


        logger::info("reserved Spell Inserted");
        logger::info("Reserved Spells:");
        for (const auto& pair : reservedSpells) {
            logger::info("  {:#010x}:", pair.first);
            logger::info("    Spell Item: {}", pair.second.spellItem ? pair.second.spellItem->GetName() : "nullptr");
            logger::info("    Reserve Spell Item: {}",
                         pair.second.reserveSpellitem ? pair.second.reserveSpellitem->GetName() : "nullptr");
            logger::info("    Reserve Spell FormID: {:#010x}", pair.second.reserveSpellitem->GetFormID());
            logger::info("    Reserve Cost: {}", pair.second.reserveCost);
        }

    } catch (std::exception& e) {
        delete effect;
        delete dynamicCarrierSpell;
        SKSE::log::error("Failed to apply spell: {}", e.what());
    }

    SKSE::log::info("Attempted to apply temporary debuff spell to player.");
}

bool IsSpellSuppressed(RE::SpellItem* spellItem, RE::PlayerCharacter* player) {
    if (player->IsDead()) {
        logger::warn("Actor is dead. Cannot log active effects.");
        return;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        logger::warn("ApplyAllSavedSpellsToActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        logger::debug("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    std::set<RE::FormID> activeSpells;
    // std::map<RE::FormID, RE::SpellItem*> activeSpells;
    // Iterate over the active effects and check for matches in the set
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        // Check if the effect is "Inactive" (Suppressed)
        bool isInactive = activeEffect->flags.all(RE::ActiveEffect::Flag::kInactive);
        // Check if it's been dispelled (waiting to be deleted)
        bool isDispelled = activeEffect->flags.all(RE::ActiveEffect::Flag::kDispelled);

        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() || isInactive || isDispelled) {
            continue;
        }

        RE::FormID spellFormID = activeEffect->spell->GetFormID();
        activeSpells.insert(spellFormID);
    }

    return activeSpells.find(spellItem->GetFormID()) != activeSpells.end();
}

void setSpellItemActiveEffectsToInactive(RE::SpellItem* spellItem, RE::PlayerCharacter* player) {
    std::vector<RE::ActiveEffect*> activeEffects = SpellUtilities::GetActiveEffectsOnActorFromSpellItem(player, spellItem);

    for (RE::ActiveEffect* activeEffect : activeEffects) {
        activeEffect->flags.set(RE::ActiveEffect::Flag::kInactive);
    }
}

void setSpellItemActiveEffectsToActive(RE::SpellItem* spellItem, RE::PlayerCharacter* player) {
    std::vector<RE::ActiveEffect*> activeEffects = SpellUtilities::GetActiveEffectsOnActorFromSpellItem(player, spellItem);

    for (RE::ActiveEffect* activeEffect : activeEffects) {
        activeEffect->flags.reset(RE::ActiveEffect::Flag::kInactive);
    }
}

void ApplyReserveSpellToPlayer2(RE::SpellItem* spellItem) {
    try {
        if (!spellItem) {
            SKSE::log::error("Spell not found for temporary debuff application.");
            return;
        }

        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            SKSE::log::error("Player not found for temporary debuff application.");
            return;
        }

        float reserveCost = CalculateMagickaForReserveSpell(spellItem, player);
        ReserveMagicka* alreadyReservedSpell = AlreadyReservedSpell(spellItem, player, reserveCost);

        bool isSupressed = IsSpellSuppressed(spellItem, player);

        // If it already exists and has the same cost, then skip
        if (alreadyReservedSpell && HandleAlreadyReservedSpell(alreadyReservedSpell, spellItem, player, reserveCost, isSupressed)) {
            return;
        }
        

        RE::SpellItem* spellReserveCarrier = CreateReserveSpellCarrier(spellItem);
        RE::Effect* spellReserveEffect = CreateReserveSpellEffect();

        LinkReserveSpellToEffect(spellReserveCarrier, spellReserveEffect, player, spellItem, reserveCost, isSupressed);
    } catch (std::exception& e) {
        SKSE::log::error("Failed to apply reserve spell: {}", e.what());
    }
}