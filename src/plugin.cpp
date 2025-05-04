#include "Plugin.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "ConfigLoader.h"
#include "SpellApplication.h"
#include "SpellCastEventHandler.h"
#include "SpellDataPersistence.h"
#include "TPPlayerInputEventHandler.h"
#include <Hooks.h>

// Template source: https://github.com/SkyrimDev/HelloWorld-using-CommonLibSSE-NG
// See also: https://github.com/CharmedBaryon/CommonLibSSE-NG/wiki

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

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SetupLog();

    logger::info(("{} v{}"), Plugin::NAME, Plugin::VERSION);
    logger::info("Game version : {}", skse->RuntimeVersion().string());

    SKSE::Init(skse);

    // EBuffs::Hooks::Install();
    
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ConfigLoader configLoader= ConfigLoader();
            std::vector configFiles = configLoader.GetConfigFileNames();
            configLoader.LoadConfigFile("Data/SKSE/Plugins/InfinityBuffsNG.ini");
            for (const auto& configFile : configFiles) {
                configLoader.LoadConfigFile(configLoader.directory / configFile);
            }
            Config::GetSingleton().Initialize();
            // EBuffs::Hooks::Install();
            // ActiveEffect::Hooks::Install();
            // ActorMagicCaster::Hooks::Install();
            // ActiveEffectReferenceEffectController::Hooks::Install();
            // Book::Hooks::Install();
        }
        if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
            if (!Config::GetSingleton().GetGeneralRule().enabled ||
                (!Config::GetSingleton().GetGeneralRule().shoutsEnabled && !Config::GetSingleton().GetGeneralRule().spellsEnabled)) {
                logger::debug("PostLoadGame event received, but shouts and spells are disabled in the general rule.");
                return;
            }
            // EBuffs::Hooks::Install();
            SpellCastEventHandler::Register();
            TPPlayerInputEventHandler::Register();
            SpellDataPersistence::LogSpellSFromMap(SpellDataPersistence::GetAllSavedSpells());  // Log all saved spells
            ApplyAllSavedPermanentSpellsToPlayer();
        }
    });

    SpellDataPersistence::RegisterSerializationCallbacks();

    logger::info("{} initialization complete.", "Infinity Buffs");
    return true;
}
