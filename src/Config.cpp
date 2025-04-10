#include "Config.h"

namespace BlinkTeleportConfig {
    MCM::MCM()
        : keyBinding(0),
          maxDistance(5000.0f),
          shouldShowVFX(true),
          cooldown(3000),
          minDistance(0.0f),
          shouldBlink(true),
          maxBlinkDistance(2000.0f),
          shouldTeleport(true) {
        LoadSettings();
    }

    std::string MCM::OnSettingChange(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                     RE::StaticFunctionTag*, std::string name, std::string value) {
        // Check if the setting exists in the map
        auto it = configSettings.GetMap().find(name);
        bool isUpdated = false;
        if (it != configSettings.GetMap().end()) {
            try {
                // Call the corresponding conversion and assignment function
                isUpdated = it->second(value);
            } catch (const std::invalid_argument& exception) {
                const std::string msg = std::format("Invalid value for setting '{}'. Value provided: '{}'. Error: {}",
                                                    name, value, exception.what());
                SKSE::log::error("{}", msg);
                return msg;
            } catch (const std::out_of_range& exception) {
                const std::string msg = std::format("Invalid value for setting '{}'. Value provided: '{}'. Error: {}",
                                                    name, value, exception.what());
                SKSE::log::error("{}", msg);
                return msg;
            }
        } else {
            const std::string msg = std::format("Unknown setting: '{}'", name);
            return std::format("{}", msg);
        }
        if (isUpdated) {
            SaveSettings();
        }
        return "";
    }

    void MCM::LoadSettings() {
        const std::vector<std::string>& keys = configSettings.GetOrder();
        for (const auto& key : keys) {
            RE::ConsoleLog::GetSingleton()->Print("Key: %s", key.c_str());
        }
        CSimpleIniA ini;
        ini.SetUnicode();
        SI_Error rc = ini.LoadFile(iniPath_);
        if (rc < 0) {
            SKSE::log::info("Could not load config file");
            return;
        }
        assert(keys.size() == 8);
        keyBinding = (int)ini.GetLongValue("General", keys[0].c_str(), 67);
        maxDistance = (float)ini.GetDoubleValue("General", keys[1].c_str(), 5000.0f);
        cooldown = (uint32_t)ini.GetLongValue("General", keys[2].c_str(), 2500);
        shouldShowVFX = ini.GetBoolValue("General", keys[3].c_str(), true);
        minDistance = (float)ini.GetDoubleValue("General", keys[4].c_str(), 50.0f);
        shouldBlink = ini.GetBoolValue("Blink", keys[5].c_str(), true);
        maxBlinkDistance = (float)ini.GetDoubleValue("Blink", keys[6].c_str(), 2500.0f);
        shouldTeleport = ini.GetBoolValue("General", keys[7].c_str(), true);
    }

    void MCM::SaveSettings() {
        const std::vector<std::string>& keys = configSettings.GetOrder();
        CSimpleIniA ini;
        ini.SetUnicode();
        SI_Error rc = ini.LoadFile(iniPath_);
        if (rc < 0) {
            SKSE::log::info("Could not load config file");
        }
        assert(keys.size() == 8);
        ini.SetLongValue("General", keys[0].c_str(), keyBinding);
        ini.SetDoubleValue("General", keys[1].c_str(), maxDistance);
        ini.SetLongValue("General", keys[2].c_str(), (long)cooldown);
        ini.SetBoolValue("General", keys[3].c_str(), shouldShowVFX);
        ini.SetDoubleValue("General", keys[4].c_str(), minDistance);
        ini.SetBoolValue("Blink", keys[5].c_str(), shouldBlink);
        ini.SetDoubleValue("Blink", keys[6].c_str(), maxBlinkDistance);
        ini.SetBoolValue("General", keys[7].c_str(), shouldTeleport);
        rc = ini.SaveFile(iniPath_);
        if (rc < 0) {
            SKSE::log::error("Failed to save the config file");
        } else {
            SKSE::log::info("Settings saved successfully to {}", iniPath_);
        }
    }

    // Static wrapper function to call the instance's onSettingChange
    static std::string onSettingChangeWrapper(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                              RE::StaticFunctionTag* tag, std::string name, std::string value) {
        return MCM::GetSingleton().OnSettingChange(vm, stackID, tag, name, value);
    }

    static int GetKeyBinding(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                             RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().keyBinding;
    }

    static float GetMaxDistance(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().maxDistance;
    }

    static bool GetShouldShowVFX(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                 RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().shouldShowVFX;
    }

    static int GetBlinkTeleportCooldown(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                        RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().cooldown;
    }

    static bool GetShouldBlink(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                               RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().shouldBlink;
    }

    static float GetBlinkMaxDistance(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                     RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().maxBlinkDistance;
    }

    static bool GetShouldTeleport(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackID,
                                  RE::StaticFunctionTag* tag) {
        return MCM::GetSingleton().shouldTeleport;
    }

    bool MCM::Register(RE::BSScript::IVirtualMachine* a_vm) {
        a_vm->RegisterFunction("OnBlinkTeleportMCMSettingChange", "BlinkTeleportMCM", onSettingChangeWrapper);
        a_vm->RegisterFunction("GetKeyBinding", "BlinkTeleportMCM", GetKeyBinding);
        a_vm->RegisterFunction("GetMaxDistance", "BlinkTeleportMCM", GetMaxDistance);
        a_vm->RegisterFunction("GetShouldShowVFX", "BlinkTeleportMCM", GetShouldShowVFX);
        a_vm->RegisterFunction("GetBlinkTeleportCooldown", "BlinkTeleportMCM", GetBlinkTeleportCooldown);
        a_vm->RegisterFunction("GetShouldBlink", "BlinkTeleportMCM", GetShouldBlink);
        a_vm->RegisterFunction("GetBlinkMaxDistance", "BlinkTeleportMCM", GetBlinkMaxDistance);
        a_vm->RegisterFunction("GetShouldTeleport", "BlinkTeleportMCM", GetShouldTeleport);
        return true;
    }
}