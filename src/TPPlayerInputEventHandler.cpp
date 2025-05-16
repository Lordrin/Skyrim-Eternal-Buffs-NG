#include "TPPlayerInputEventHandler.h"

RE::BSEventNotifyControl TPPlayerInputEventHandler::ProcessEvent(RE::InputEvent* const* a_event,
                                                                 RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/) {
    if (!a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto hotKey = Config::GetSingleton().GetKeyBinding();
    if (hotKey == 0 || hotKey == 1) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto UI = RE::UI::GetSingleton();
    if (!UI || UI->IsMenuOpen(RE::Console::MENU_NAME) || UI->GameIsPaused()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // from https://github.com/powerof3/ReadOrTakeBooks
    for (auto inputEvent = *a_event; inputEvent; inputEvent = inputEvent->next) {
        if (inputEvent->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            const RE::ButtonEvent* button = static_cast<const RE::ButtonEvent*>(inputEvent);
            const RE::INPUT_DEVICE device = inputEvent->GetDevice();
            uint32_t key = button->GetIDCode();

            switch (device) {
                case RE::INPUT_DEVICE::kMouse:
                    key += SKSE::InputMap::kMacro_MouseButtonOffset;
                    break;
                case RE::INPUT_DEVICE::kGamepad:
                    key = SKSE::InputMap::GamepadMaskToKeycode(key);
                    break;
                default:
                    break;
            }

            const bool toggleKeyHeld = Config::GetSingleton().GetToggleKeyHeld();

            if (key == hotKey) {
                if (Config::GetSingleton().GetToggleKeyHeld() != button->IsHeld()) {
                    logger::trace("ToggleKeyHeld changed from {} to {}", toggleKeyHeld, button->IsHeld());
                    Config::GetSingleton().GetToggleKeyHeld() = button->IsHeld();
                }
            }
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

void TPPlayerInputEventHandler::Register() {
    auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(&GetSingleton());
        SKSE::log::info("InputEventHandler registered with BSInputDeviceManager");
    } else {
        SKSE::log::error("Failed to get BSInputDeviceManager singleton");
    }
}
