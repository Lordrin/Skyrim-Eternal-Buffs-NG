#include "Plugin.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "ConfigLoader.h"
#include "SpellApplication.h"
#include "SpellCastEventHandler.h"
#include "SpellDataPersistence.h"

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

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            logger::debug("DataLoaded event received, starting SpellCastDetector...");
            ConfigLoader().LoadConfigFile("Data/SKSE/Plugins/InfinityBuffsNG.ini");
            Global::InitializeShoutSpellMap();
        }
        if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
            if (!Global::generalRule.enabled ||
                (!Global::generalRule.shoutsEnabled && !Global::generalRule.spellsEnabled)) {
                logger::debug("PostLoadGame event received, but shouts and spells are disabled in the general rule.");
                return;
            }
            logger::debug("PostLoadGame event received, start applying Permanent Spells...");
            SpellCastEventHandler::Register();
            SpellDataPersistence::LogSpellSFromMap(SpellDataPersistence::GetAllSavedSpells());  // Log all saved spells
            ApplyAllSavedPermanentSpellsToPlayer();
        }
    });

    SpellDataPersistence::RegisterSerializationCallbacks();

    logger::info("{} initialization complete.", "Infinity Buffs");
    return true;
}
