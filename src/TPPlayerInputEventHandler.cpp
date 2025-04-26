#include "TPPlayerInputEventHandler.h"

RE::BSEventNotifyControl TPPlayerInputEventHandler::ProcessEvent(RE::InputEvent* const* a_event,
                                                                 RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/) {
    if (!a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto hotKey = Config::keyBinding;
    if (hotKey == 0 || hotKey == 1) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto UI = RE::UI::GetSingleton();
    if (!UI || UI->IsMenuOpen(RE::Console::MENU_NAME) || UI->GameIsPaused()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // const std::chrono::milliseconds debounceDuration{config_.cooldown};
    // auto currentTime = std::chrono::steady_clock::now();
    // Check if enough time has passed since the last event was processed
    // if (currentTime - lastProcessedTime < debounceDuration) {
    //     return RE::BSEventNotifyControl::kContinue;
    // }

    for (auto inputEvent = *a_event; inputEvent; inputEvent = inputEvent->next) {
        if (inputEvent->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            const RE::ButtonEvent* button = static_cast<const RE::ButtonEvent*>(inputEvent);
            const auto device = inputEvent->GetDevice();
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

            auto& toggleKeyHeld = Config::toggleKeyHeld;

            if (key == hotKey) {
                if (toggleKeyHeld != button->IsHeld()) {
                    toggleKeyHeld = button->IsHeld();

                    // if (const auto crossHairPickData = RE::CrosshairPickData::GetSingleton()) {
                    //     const auto target = crossHairPickData->target.get();
                    //     auto base = target ? target->GetBaseObject() : nullptr;

                    //     if (base && base->IsBook()) {
                    //         player->UpdateCrosshairs();
                    //     }
                    // }
                }
            }

            // if (key == config_.keyBinding) {
            //     TeleportPlayer::GetSingleton().Teleport();

            //     // Update the last processed time after handling the event
            //     lastProcessedTime = std::chrono::steady_clock::now();
            // }
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

void TPPlayerInputEventHandler::Register() {
    // static TPPlayerInputEventHandler inputEventHandler;
    auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(&GetSingleton());
        SKSE::log::info("InputEventHandler registered with BSInputDeviceManager");
    } else {
        SKSE::log::error("Failed to get BSInputDeviceManager singleton");
    }
}
