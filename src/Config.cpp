#include "Config.h"

OrderedMap<std::string, std::function<void(const std::string&, const std::string&)>> Config::GetParsers() {
    return {{"keybinding",
             [this](const std::string& value, const std::string&) {
                 try {
                     uint32_t key = std::stoul(value, nullptr, 10);
                     logger::info("Keybinding set to {}", key);
                     Config::GetSingleton().GetKeyBinding() = key;
                 } catch (std::exception& e) {
                     logger::warn("Failed to parse keybinding value '{}': {}", value, e.what());
                 }
             }},
            {"logginglevel",
             [this](const std::string& value, const std::string&) {
                 try {
                     std::string loggingLevel = Utilities::ToLower(Utilities::TrimString(value));
                     spdlog::level::level_enum level = spdlog::level::from_str(loggingLevel);
                     if (level == spdlog::level::n_levels) {
                         logger::warn("Unknown logging level '{}'. Defaulting to 'info'.", loggingLevel);
                         level = spdlog::level::info;
                     }
                     spdlog::set_level(level);
                 } catch (std::exception& e) {
                     logger::warn("Failed to parse logginglevel value '{}': {}", value, e.what());
                 }
             }},
            {"reserveeffectformid", [this](const std::string& value, const std::string&) {
                 try {
                     uint32_t formID = std::stoul(value, nullptr, 16);
                     Config::GetSingleton().GetReserveEffectFormID() = formID;
                 } catch (std::exception& e) {
                     logger::warn("Failed to parse Reserveeffectformid value '{}': {}", value, e.what());
                 }
             }}};
}