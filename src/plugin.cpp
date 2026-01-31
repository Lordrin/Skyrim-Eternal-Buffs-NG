#include "Plugin.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "ConfigLoader.h"
#include "SpellApplication.h"
#include "SpellCastEventHandler.h"
#include "SpellDataPersistence.h"
#include "TPPlayerInputEventHandler.h"
#include <MagicApplyEventHandler.h>

// void ApplyTemporaryDebuffToPlayer(RE::SpellItem* spellToApply) {
//     auto datahandler = RE::TESDataHandler::GetSingleton();
//     if (!datahandler) {
//         SKSE::log::error("DataHandler is null.");
//     }
//     auto form = datahandler->LookupForm(0x00000D66, "EternalBuffsNG.esp");
//     if (!form) {
//         SKSE::log::error("Custom spell is null. Returning early.");
//         return;
//     }

//     RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
//     if (!player || !spellToApply) {
//         SKSE::log::warn("Player or Spell not found for temporary debuff application.");
//         return;
//     }

//     // Get the player's MagicCaster component for the desired hand (e.g., kRightHand)
//     // Or if the spell is 'Self' delivery, a 'self' caster might be more appropriate,
//     // but often using a hand caster works well for Fire and Forget spells.
//     RE::MagicCaster* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
//     if (!magicCaster) {
//         // Try other hand or a default caster if right hand fails
//         magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
//         if (!magicCaster) {
//             SKSE::log::warn("Could not get a valid MagicCaster for the player.");
//             return;
//         }
//     }

//     auto effects = spellToApply->effects;
//     if (effects.empty()) {
//         SKSE::log::warn("Spell {} has no effects.", spellToApply->GetName());
//         return;
//     }

//     // Get the first effect in the spell
//     auto* effect = effects[0];
//     if (!effect || !effect->baseEffect) {
//         SKSE::log::warn("Effect for spell {} is null.", spellToApply->GetName());
//         return;
//     }

//     // Get the MGEF from the effect
//     auto* mgef = effect->baseEffect;
//     if (!mgef) {
//         SKSE::log::warn("MGEF for effect of spell {} is null.", spellToApply->GetName());
//         return;
//     }

//     // mgef->data.

//     // --- Method 1: Using magicCaster->CastSpellImmediate (Often preferred for direct application) ---
//     // This method is generally good for fire-and-forget spells applied directly.
//     // The bools are:
//     //   arg2: (bool) noHitEffectArt (true = no visual effect like shaders on target)
//     //   arg3: (bool) noHitSound (true = no sound on target)
//     // You might want these to be false if your MGEF has hit shaders/sounds you want to see.
//     // For a silent debuff, true is good.
//     bool noHitEffectArt = true;
//     bool noHitSound = true;

//     RE::BSString out;
//     spellToApply->GetDescription(out, spellToApply);

//     // New description string
//     std::string newDescriptionStr =
//         "This spell now has a dynamically updated description! Reduces max Magicka by 50 for 1 minute.";

//     logger::info("New description: {}", out);
//     logger::info("test {}", spellToApply->descriptionText.id);

//     RE::ConcreteFormFactory<RE::SpellItem, RE::FormType::Spell>* formFactory =
//         RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
//     RE::BSString newBSDescription(newDescriptionStr.c_str());

//     RE::SpellItem* dynamicCarrierSpell = nullptr;
//     if (formFactory) {
//         dynamicCarrierSpell = formFactory->Create();  // This gets an FFxxxxxx FormID
//     } else {
//         SKSE::log::error("Failed to get spell factory!");
//         return;  // or handle error
//     }

//     if (!dynamicCarrierSpell) {
//         SKSE::log::error("Failed to create dynamic carrier spell instance!");
//         return;  // or handle error
//     }

//     // Configure this dynamic spell:
//     dynamicCarrierSpell->data.spellType = RE::MagicSystem::SpellType::kSpell;  // Or kLesserPower, etc.
//     dynamicCarrierSpell->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
//     dynamicCarrierSpell->data.delivery = RE::MagicSystem::Delivery::kSelf;
//     // Set a unique (even if internal) name for debugging if you want
//     dynamicCarrierSpell->fullName =
//         RE::BSFixedString("DynamicCarrierInstance_XYZ");  // XYZ could be a counter or timestamp'
//     // dynamicCarrierSpell->descriptionText = spellToApply->descriptionText;

//     logger::info("Form Type: {}", form->GetFormType());
//     RE::EffectSetting* customMagicEffect = form->As<RE::EffectSetting>();
//     // RE::Effect* effect123 = customMagicEffect->As<RE::MagicEffect>();
//     // auto effectSetting = dynamicCarrierSpell->avEffectSetting;
//     // effectSetting->
//     if (!customMagicEffect) {
//         SKSE::log::error("Spell is null.");
//     }

//     logger::info("Effect123: {}", customMagicEffect->fullName);

//     // Create a new RE::Effect instance.
//     // IMPORTANT: Memory management for this RE::Effect object is crucial.
//     // If the spell takes ownership, great. If not, you might need to manage it.
//     // Often, when added to the spell's list and the spell is used, the game manages it.
//     RE::Effect* newEffectItem =
//         new RE::Effect();  // Allocate on the heap // The RE::SpellItem destructor iterates through its effects array
//                            // and deletes each RE::Effect* it contains.

//     // TODO delete newEffectItem on error
//     if (!newEffectItem) {
//         SKSE::log::error("Failed to allocate RE::Effect item!");
//         // Potentially delete dynamicSpell if it's not yet fully integrated
//         return;
//     }

//     // Configure the RE::Effect item

//     // Set the effect's parameters for this spell
//     newEffectItem->effectItem.magnitude = -50.0f;   // Example: Reduce max Magicka by 50
//     newEffectItem->effectItem.duration = 60;        // Example: 60 seconds
//     newEffectItem->effectItem.area = 0;             // Example: 0 area for a self-target effect
//     newEffectItem->baseEffect = customMagicEffect;  // ** This is where you link your MGEF **

//     dynamicCarrierSpell->effects.push_back(newEffectItem);
//     float cost = dynamicCarrierSpell->CalculateMagickaCost(player);

//     dynamicCarrierSpell->GetDescription(out, dynamicCarrierSpell);
//     // dynamicCarrierSpell->descriptionText = spellToApply->descriptionText;

//     logger::info("Cost: {}", cost);
//     logger::info("Description: {}", out);

//     // // Create the Effect entry for this spell
//     // RE::Effect* effectEntry = new RE::Effect();  // Needs proper memory management!
//     // effectEntry->baseEffect = yourStaticMGEF;    // Pointer to your MyPlugin_TempReduceMaxMagickaMGEF
//     // effectEntry->effectItem.magnitude = -50.0f;
//     // effectEntry->effectItem.duration = 60;  // 1 minute
//     // effectEntry->effectItem.area = 0;
//     // ... set other RE::Effect::EffectItem members

//     // RE::BSString newBSDescription(newDescriptionStr.c_str());

//     // RE::BGSLocalizedStringDL xl(newDescriptionStr.c_str());

//     // spellToApply->descriptionText = newBSDescription;

//     // // Method 1: Using SetDescription (Often preferred as it handles BGSLocalizedString internally)
//     // // The second parameter 'generateName' is usually true if you also want to potentially
//     // // regenerate a name based on the description or other factors, but for just setting
//     // // the description text, it might not be strictly necessary to be true.
//     // // However, for descriptions specifically, it's usually about setting the text.
//     // // CommonLibSSE/RE might expose this as:
//     // if (spellToApply->descriptionText ||
//     //     spellToApply->descriptionText.c_str() != newDescriptionStr) {  // Avoid unnecessary updates
//     //     RE::BSString newBSDescription(newDescriptionStr.c_str());
//     //     mySpell->SetDescription(newBSDescription, nullptr);  // The second param is often a TESFile for some
//     //     contexts,
//     //                                                          // nullptr is usually fine for runtime changes.
//     //     SKSE::log::info(FMT_STRING("Updated spell description for '{}'"), mySpell->GetFullName());
//     // }

//     // The target needs to be a TESObjectREFR. The player is one.
//     // The 'false' for a_ responsabile relates to crime reporting, usually false for self-applied.
//     // The 'true' for a_blockEquip is probably not relevant here but often set true.
//     // The final nullptr is for a_extraList, not usually needed.
//     // The exact signature and best way to call can vary subtly with game versions/SKSE helpers.
//     // This is a common pattern:
//     magicCaster->CastSpellImmediate(dynamicCarrierSpell, true, player, 1.0f, false, -50.0f, player);
//     // A simpler overload if available for self-casting:
//     // magicCaster->CastSpellImmediate(spellToApply, false, player); // This might be what you look for in
//     CommonLibSSE

//     // Note: CastSpellImmediate might have different overloads. You'll need to find the one that suits
//     // casting a spell from source (caster) onto a target (player).
//     // CommonLibSSE might offer helper functions that simplify this.

//     // --- Method 2: More involved casting setup (gives more control but is more complex) ---
//     /*
//     // This is a more manual way, sometimes needed for specific behaviors.
//     // It's often overkill if CastSpellImmediate works.

//     magicCaster->SetCastingSpell(spellToApply); // Tell the caster what spell it's holding
//     magicCaster->currentSpellcost = 0;         // Optional: if you want to bypass magicka cost for this cast

//     // For Fire and Forget, you'd then typically call something like:
//     // magicCaster->Fire(); // Or a similar function that releases the spell
//     // The target is often implicitly the caster's target or self if delivery is self.
//     // This part is very dependent on the exact SKSE/CommonLibSSE functions available
//     // and the desired nuance (e.g., do you want casting animations, sounds, etc.?)
//     // For a silent, immediate application, CastSpellImmediate is usually better.
//     */

//     SKSE::log::info("Attempted to apply temporary debuff spell to player.");
// }

struct OnAddHook {
    static void Install() {
        // Index 0x2 is OnAdd (0 is Destructor, 1 is AdjustForPerks)
        REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ActiveEffect[0] };
        _OnAdd = vtbl.write_vfunc(0x2, &Hook_OnAdd);
    }

private:
    static void Hook_OnAdd(RE::ActiveEffect* a_this, RE::MagicTarget* a_target) {
        // TEST: Log to the SKSE log file
        if (a_this->GetBaseObject()) {
            SKSE::log::info("Effect added: {}", a_this->GetBaseObject()->GetName());
        }

        // Always call the original!
        _OnAdd(a_this, a_target);
    }

    static inline REL::Relocation<decltype(&RE::ActiveEffect::OnAdd)> _OnAdd;
};

struct HandleEventHook {
    static void thunk(RE::ActiveEffect* a_this, const RE::BSFixedString& a_eventName) {
        if (a_this->spell) {
            SKSE::log::info("Spell {} received event: {}", a_this->spell->GetName(), a_eventName.c_str());
        }

        // Always call the original!
        func(a_this, a_eventName);
    }

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install() {
        REL::Relocation<std::uintptr_t> vTable{RE::VTABLE_ActiveEffect[0]};
        // 0x0D from your snippet
        func = vTable.write_vfunc(0x0D, thunk);
    }
};

struct CompareHook {
    static std::int32_t thunk(RE::ActiveEffect* a_this, RE::ActiveEffect* a_other) {
        if (a_this->spell && a_other->spell) {
            SKSE::log::info("Comparing Spell: {} vs {}", a_this->spell->GetName(), a_other->spell->GetName());
        }

        // Call the original engine logic
        std::int32_t result = func(a_this, a_other);

        SKSE::log::info("Result: {} ({} wins)", result, result >= 0 ? "Original" : "New");

        return result;
    }

    static inline REL::Relocation<decltype(thunk)> func;

    static void Install() {
        REL::Relocation<std::uintptr_t> vTable{RE::VTABLE_ActiveEffect[0]};
        // 0x0C from your snippet
        func = vTable.write_vfunc(0x0C, thunk);
    }
};

void SetupLog() {
    auto logsFolder = logger::log_directory();
    if (!logsFolder) {
        SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        return;
    }

    auto logFilePath = *logsFolder / std::format("{}.log", Plugin::NAME);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::debug);
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();

    logger::info(("{} v{}"), Plugin::NAME, Plugin::VERSION);
    logger::info("Game version : {}", skse->RuntimeVersion().string());

    SKSE::Init(skse);
    // OnAddHook::Install();
    // Install();
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ConfigLoader configLoader = ConfigLoader();
            std::vector configFiles = configLoader.GetConfigFileNames();
            configLoader.LoadConfigFile("Data/SKSE/Plugins/EternalBuffsNG.ini");
            for (const auto& configFile : configFiles) {
                configLoader.LoadConfigFile(configLoader.directory / configFile);
            }
            auto generalRule = Config::GetSingleton().GetGeneralRule();
            logger::info("Loaded config rules: {}", generalRule.ToString());
            auto spellRules = Config::GetSingleton().GetSpellRules();
            for (auto& spellRule : spellRules) {
                logger::info("Loaded spell rule: {}", spellRule.first);
                logger::info("{}", spellRule.second.ToString());
            }
            // TODO populate form
            //  skyrimHwnd = ::FindWindowA("Skyrim Special Edition", nullptr);
            //  if (!skyrimHwnd) skyrimHwnd = ::FindWindowA("Skyrim", nullptr);
            //  if (!skyrimHwnd) skyrimHwnd = ::FindWindowA("SkyrimSE", nullptr);  // rare, but some mods use this
        }
        if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
            if (!Config::GetSingleton().GetGeneralRule().enabled) {
                logger::debug("PostLoadGame event received, but shouts and spells are disabled in the generalrule.");
                return;
            }
            SpellCastEventHandler::Register();
            TPPlayerInputEventHandler::Register();
            MagicApplyEventHandler::Register();
            ApplyAllSavedPermanentSpellsToPlayer();
            SpellDataPersistence::LogSpellSFromMap(SpellDataPersistence::GetAllSavedSpells());  // Log all saved spells
        }
        // if(message->type == SKSE::MessagingInterface::kDataLoaded) {
        //     Install();
        // }
        // if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        //     ResetInput();
        // }
    });

    SpellDataPersistence::RegisterSerializationCallbacks();

    logger::info("{} initialization complete.", "Infinity Buffs");
    return true;
}
