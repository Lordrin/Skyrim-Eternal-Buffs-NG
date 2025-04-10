#pragma once

#include "SimpleIni.h"
#include "OrderedMap.h"
#include <functional>

namespace BlinkTeleportConfig {
    class MCM {
    private:
        const char* iniPath_ = "Data/SKSE/Plugins/BlinkTeleport.ini";
        template <typename T>
        bool UpdateSetting(const std::string& value, T& member, std::function<T(const std::string&)> convert) {
            T newValue = convert(value);
            if (newValue == member) {
                return false;
            }
            member = newValue;
            return true;
        }

    public:
        MCM();
        static MCM& GetSingleton() {
            static MCM instance;
            return instance;
        }

        uint32_t cooldown;
        uint32_t keyBinding;
        float minDistance;
        float maxDistance;
        float maxBlinkDistance;
        bool shouldBlink;
        bool shouldShowVFX;
        bool shouldTeleport;

        // The settings map that maps string keys to functions that handle conversion and assignment
        const OrderedMap<std::string, std::function<bool(const std::string&)>> configSettings = {
            {"Keybind",
             [this](const std::string& value) {
                 return UpdateSetting<uint32_t>(value, this->keyBinding, [](const std::string& val) {
                     RE::ConsoleLog::GetSingleton()->Print("Keybind set to %d", std::stoul(val));
                     return std::stoul(val);
                 });
             }},

            {"MaxDistance",
             [this](const std::string& value) {
                 return UpdateSetting<float>(value, this->maxDistance,
                                             [](const std::string& val) { return std::stof(val); });
             }},

            {"Cooldown",
             [this](const std::string& value) {
                 return UpdateSetting<uint32_t>(value, this->cooldown,
                                                [](const std::string& val) { return std::stoul(val); });
             }},

            {"ShowVFX",
             [this](const std::string& value) {
                 return UpdateSetting<bool>(value, this->shouldShowVFX,
                                            [](const std::string& val) { return (val == "TRUE"); });
             }},

            {"MinDistance",
             [this](const std::string& value) {
                 return UpdateSetting<float>(value, this->minDistance,
                                             [](const std::string& val) { return std::stof(val); });
             }},

            {"ShouldBlink",
             [this](const std::string& value) {
                 return UpdateSetting<bool>(value, this->shouldBlink,
                                            [](const std::string& val) { return (val == "TRUE"); });
             }},

            {"MaxBlinkDistance",
             [this](const std::string& value) {
                 return UpdateSetting<float>(value, this->maxBlinkDistance,
                                             [](const std::string& val) { return std::stof(val); });
             }},

            {"ShouldTeleport", [this](const std::string& value) {
                 return UpdateSetting<bool>(value, this->shouldTeleport,
                                            [](const std::string& val) { return (val == "TRUE"); });
             }}};

        static bool Register(RE::BSScript::IVirtualMachine* a_vm);
        std::string OnSettingChange(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                    RE::StaticFunctionTag*, std::string name, std::string value);
        void LoadSettings();
        void SaveSettings();
    };
}