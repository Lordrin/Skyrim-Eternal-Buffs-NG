#include "UI.h"

namespace {
    void HelpMarker(const char* text) {
        ImGuiMCP::SameLine();
        ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{0.55f, 0.55f, 0.55f, 1.0f});
        ImGuiMCP::TextUnformatted("(?)");
        ImGuiMCP::PopStyleColor();

        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::BeginTooltip();
            ImGuiMCP::PushTextWrapPos(ImGuiMCP::GetFontSize() * 28.0f);
            ImGuiMCP::TextUnformatted(text);
            ImGuiMCP::PopTextWrapPos();
            ImGuiMCP::EndTooltip();
        }
    }

    bool Slider(const char* label, float& value, float min, float max, const char* format) {
        ImGuiMCP::TextUnformatted(label);
        ImGuiMCP::SameLine(130.0f);
        ImGuiMCP::SetNextItemWidth(260.0f);
        return ImGuiMCP::SliderFloat((std::string("##") + label).c_str(), &value, min, max, format);
    }

    bool captureListening = false;
    bool captureArmed = false;

    int PollCapturedInput() {
        struct KeyMap {
            int key;
            int dik;
        };
        static const KeyMap keys[] = {{ImGuiMCP::ImGuiKey_Escape, 1},
                                      {ImGuiMCP::ImGuiKey_1, 2},
                                      {ImGuiMCP::ImGuiKey_2, 3},
                                      {ImGuiMCP::ImGuiKey_3, 4},
                                      {ImGuiMCP::ImGuiKey_4, 5},
                                      {ImGuiMCP::ImGuiKey_5, 6},
                                      {ImGuiMCP::ImGuiKey_6, 7},
                                      {ImGuiMCP::ImGuiKey_7, 8},
                                      {ImGuiMCP::ImGuiKey_8, 9},
                                      {ImGuiMCP::ImGuiKey_9, 10},
                                      {ImGuiMCP::ImGuiKey_0, 11},
                                      {ImGuiMCP::ImGuiKey_Minus, 12},
                                      {ImGuiMCP::ImGuiKey_Equal, 13},
                                      {ImGuiMCP::ImGuiKey_Backspace, 14},
                                      {ImGuiMCP::ImGuiKey_Tab, 15},
                                      {ImGuiMCP::ImGuiKey_Q, 16},
                                      {ImGuiMCP::ImGuiKey_W, 17},
                                      {ImGuiMCP::ImGuiKey_E, 18},
                                      {ImGuiMCP::ImGuiKey_R, 19},
                                      {ImGuiMCP::ImGuiKey_T, 20},
                                      {ImGuiMCP::ImGuiKey_Y, 21},
                                      {ImGuiMCP::ImGuiKey_U, 22},
                                      {ImGuiMCP::ImGuiKey_I, 23},
                                      {ImGuiMCP::ImGuiKey_O, 24},
                                      {ImGuiMCP::ImGuiKey_P, 25},
                                      {ImGuiMCP::ImGuiKey_LeftBracket, 26},
                                      {ImGuiMCP::ImGuiKey_RightBracket, 27},
                                      {ImGuiMCP::ImGuiKey_Enter, 28},
                                      {ImGuiMCP::ImGuiKey_LeftCtrl, 29},
                                      {ImGuiMCP::ImGuiKey_A, 30},
                                      {ImGuiMCP::ImGuiKey_S, 31},
                                      {ImGuiMCP::ImGuiKey_D, 32},
                                      {ImGuiMCP::ImGuiKey_F, 33},
                                      {ImGuiMCP::ImGuiKey_G, 34},
                                      {ImGuiMCP::ImGuiKey_H, 35},
                                      {ImGuiMCP::ImGuiKey_J, 36},
                                      {ImGuiMCP::ImGuiKey_K, 37},
                                      {ImGuiMCP::ImGuiKey_L, 38},
                                      {ImGuiMCP::ImGuiKey_Semicolon, 39},
                                      {ImGuiMCP::ImGuiKey_Apostrophe, 40},
                                      {ImGuiMCP::ImGuiKey_GraveAccent, 41},
                                      {ImGuiMCP::ImGuiKey_LeftShift, 42},
                                      {ImGuiMCP::ImGuiKey_Backslash, 43},
                                      {ImGuiMCP::ImGuiKey_Z, 44},
                                      {ImGuiMCP::ImGuiKey_X, 45},
                                      {ImGuiMCP::ImGuiKey_C, 46},
                                      {ImGuiMCP::ImGuiKey_V, 47},
                                      {ImGuiMCP::ImGuiKey_B, 48},
                                      {ImGuiMCP::ImGuiKey_N, 49},
                                      {ImGuiMCP::ImGuiKey_M, 50},
                                      {ImGuiMCP::ImGuiKey_Comma, 51},
                                      {ImGuiMCP::ImGuiKey_Period, 52},
                                      {ImGuiMCP::ImGuiKey_Slash, 53},
                                      {ImGuiMCP::ImGuiKey_RightShift, 54},
                                      {ImGuiMCP::ImGuiKey_KeypadMultiply, 55},
                                      {ImGuiMCP::ImGuiKey_LeftAlt, 56},
                                      {ImGuiMCP::ImGuiKey_Space, 57},
                                      {ImGuiMCP::ImGuiKey_CapsLock, 58},
                                      {ImGuiMCP::ImGuiKey_F1, 59},
                                      {ImGuiMCP::ImGuiKey_F2, 60},
                                      {ImGuiMCP::ImGuiKey_F3, 61},
                                      {ImGuiMCP::ImGuiKey_F4, 62},
                                      {ImGuiMCP::ImGuiKey_F5, 63},
                                      {ImGuiMCP::ImGuiKey_F6, 64},
                                      {ImGuiMCP::ImGuiKey_F7, 65},
                                      {ImGuiMCP::ImGuiKey_F8, 66},
                                      {ImGuiMCP::ImGuiKey_F9, 67},
                                      {ImGuiMCP::ImGuiKey_F10, 68},
                                      {ImGuiMCP::ImGuiKey_NumLock, 69},
                                      {ImGuiMCP::ImGuiKey_ScrollLock, 70},
                                      {ImGuiMCP::ImGuiKey_Keypad7, 71},
                                      {ImGuiMCP::ImGuiKey_Keypad8, 72},
                                      {ImGuiMCP::ImGuiKey_Keypad9, 73},
                                      {ImGuiMCP::ImGuiKey_KeypadSubtract, 74},
                                      {ImGuiMCP::ImGuiKey_Keypad4, 75},
                                      {ImGuiMCP::ImGuiKey_Keypad5, 76},
                                      {ImGuiMCP::ImGuiKey_Keypad6, 77},
                                      {ImGuiMCP::ImGuiKey_KeypadAdd, 78},
                                      {ImGuiMCP::ImGuiKey_Keypad1, 79},
                                      {ImGuiMCP::ImGuiKey_Keypad2, 80},
                                      {ImGuiMCP::ImGuiKey_Keypad3, 81},
                                      {ImGuiMCP::ImGuiKey_Keypad0, 82},
                                      {ImGuiMCP::ImGuiKey_KeypadDecimal, 83},
                                      {ImGuiMCP::ImGuiKey_F11, 87},
                                      {ImGuiMCP::ImGuiKey_F12, 88},
                                      {ImGuiMCP::ImGuiKey_KeypadEnter, 156},
                                      {ImGuiMCP::ImGuiKey_RightCtrl, 157},
                                      {ImGuiMCP::ImGuiKey_KeypadDivide, 181},
                                      {ImGuiMCP::ImGuiKey_PrintScreen, 183},
                                      {ImGuiMCP::ImGuiKey_RightAlt, 184},
                                      {ImGuiMCP::ImGuiKey_Pause, 197},
                                      {ImGuiMCP::ImGuiKey_Home, 199},
                                      {ImGuiMCP::ImGuiKey_UpArrow, 200},
                                      {ImGuiMCP::ImGuiKey_PageUp, 201},
                                      {ImGuiMCP::ImGuiKey_LeftArrow, 203},
                                      {ImGuiMCP::ImGuiKey_RightArrow, 205},
                                      {ImGuiMCP::ImGuiKey_End, 207},
                                      {ImGuiMCP::ImGuiKey_DownArrow, 208},
                                      {ImGuiMCP::ImGuiKey_PageDown, 209},
                                      {ImGuiMCP::ImGuiKey_Insert, 210},
                                      {ImGuiMCP::ImGuiKey_Delete, 211},
                                      {ImGuiMCP::ImGuiKey_GamepadDpadUp, 266},
                                      {ImGuiMCP::ImGuiKey_GamepadDpadDown, 267},
                                      {ImGuiMCP::ImGuiKey_GamepadDpadLeft, 268},
                                      {ImGuiMCP::ImGuiKey_GamepadDpadRight, 269},
                                      {ImGuiMCP::ImGuiKey_GamepadStart, 270},
                                      {ImGuiMCP::ImGuiKey_GamepadBack, 271},
                                      {ImGuiMCP::ImGuiKey_GamepadL3, 272},
                                      {ImGuiMCP::ImGuiKey_GamepadR3, 273},
                                      {ImGuiMCP::ImGuiKey_GamepadL1, 274},
                                      {ImGuiMCP::ImGuiKey_GamepadR1, 275},
                                      {ImGuiMCP::ImGuiKey_GamepadFaceDown, 276},
                                      {ImGuiMCP::ImGuiKey_GamepadFaceRight, 277},
                                      {ImGuiMCP::ImGuiKey_GamepadFaceLeft, 278},
                                      {ImGuiMCP::ImGuiKey_GamepadFaceUp, 279},
                                      {ImGuiMCP::ImGuiKey_GamepadL2, 280},
                                      {ImGuiMCP::ImGuiKey_GamepadR2, 281}};

        for (const auto& m : keys) {
            if (ImGuiMCP::IsKeyPressed(static_cast<ImGuiMCP::ImGuiKey>(m.key), false)) {
                return m.dik;
            }
        }

        if (ImGuiMCP::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Left, false)) {
            return 256;
        }

        if (ImGuiMCP::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Right, false)) {
            return 257;
        }

        if (ImGuiMCP::IsMouseClicked(ImGuiMCP::ImGuiMouseButton_Middle, false)) {
            return 258;
        }

        if (ImGuiMCP::IsMouseClicked(3, false)) {
            return 259;
        }

        if (ImGuiMCP::IsMouseClicked(4, false)) {
            return 260;
        }

        return -1;
    }
}

namespace SettingsUI {
    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("[Eternal buffs] SKSE Menu Framework not installed. Settings menu skipped.");
            return;
        }

        SKSEMenuFramework::SetSection("Eternal Buffs");
        SKSEMenuFramework::AddSectionItem("Settings", SettingsUI::Render);
        logger::info("[Eternal buffs] Registered SKSE Menu Framework settings.");
    }

    void __stdcall RenderSpellRulesTable() {
        const OrderedMap<std::string, SpellRule>& spellRules = Config::GetSingleton().GetSpellRules();

        if (ImGuiMCP::CollapsingHeader("SPELL RULES")) {
            ImGuiMCP::SetWindowFontScale(0.93f);

            ImGuiMCP::Text("Total rules: %zu", spellRules.Size());
            ImGuiMCP::Spacing();

            static ImGuiMCP::ImGuiTableFlags flags =
                ImGuiMCP::ImGuiTableFlags_Borders | ImGuiMCP::ImGuiTableFlags_RowBg |
                ImGuiMCP::ImGuiTableFlags_Resizable | ImGuiMCP::ImGuiTableFlags_ScrollY |
                ImGuiMCP::ImGuiTableFlags_ScrollX;

            ImGuiMCP::ImVec2 outerSize(0.0f, ImGuiMCP::GetTextLineHeightWithSpacing() * 12.0f);

            if (ImGuiMCP::BeginTable("SpellRulesTable", 5, flags, outerSize)) {
                ImGuiMCP::TableSetupScrollFreeze(0, 1);
                ImGuiMCP::TableSetupColumn("Source", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGuiMCP::TableSetupColumn("Resolved Form", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGuiMCP::TableSetupColumn("Name", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGuiMCP::TableSetupColumn("Permanent", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.05f);
                ImGuiMCP::TableSetupColumn("Duration", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.05f);
                // ImGuiMCP::TableSetupColumn("Magnitude", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.10f);
                ImGuiMCP::TableHeadersRow();

                for (const auto& [ruleName, rule] : spellRules) {
                    ImGuiMCP::TableNextRow();
                    ImGuiMCP::PushID(ruleName.c_str());

                    // --- Source file ---
                    ImGuiMCP::TableSetColumnIndex(0);
                    ImGuiMCP::TextUnformatted(rule.sourceFile.empty() ? "<none>" : rule.sourceFile.c_str());

                    // --- Resolved form ---
                    ImGuiMCP::TableSetColumnIndex(1);
                    if (rule.resolvedForm) {
                        const char* name = rule.resolvedForm->GetName();
                        ImGuiMCP::Text("%s (%08X)", (name && name[0]) ? name : "<unnamed>",
                                       rule.resolvedForm->GetFormID());
                    } else {
                        ImGuiMCP::TextDisabled("(unresolved)");
                    }

                    // --- Name filter ---
                    ImGuiMCP::TableSetColumnIndex(2);
                    ImGuiMCP::TextUnformatted(rule.nameFilter.empty() ? "<none>" : rule.nameFilter.c_str());

                    // // --- Plugin filter ---
                    // ImGuiMCP::TableSetColumnIndex(4);
                    // ImGuiMCP::TextUnformatted(rule.pluginFilter.empty() ? "<none>" : rule.pluginFilter.c_str());

                    // // --- Keywords (joined) ---
                    // ImGuiMCP::TableSetColumnIndex(5);
                    // if (rule.keywordFilter.empty()) {
                    //     ImGuiMCP::TextDisabled("(none)");
                    // } else {
                    //     std::string joined;
                    //     for (size_t i = 0; i < rule.keywordFilter.size(); ++i) {
                    //         joined += rule.keywordFilter[i];
                    //         if (i + 1 < rule.keywordFilter.size()) joined += ", ";
                    //     }
                    //     ImGuiMCP::TextWrapped("%s", joined.c_str());
                    // }

                    // // --- shouldReserveMagicka (tri-state, read-only) ---
                    // ImGuiMCP::TableSetColumnIndex(6);
                    // if (rule.shouldReserveMagicka.has_value()) {
                    //     ImGuiMCP::TextUnformatted(rule.shouldReserveMagicka.value() ? "True" : "False");
                    // } else {
                    //     ImGuiMCP::TextDisabled("(unset)");
                    // }

                    // --  isPermanentEnabled ---
                    ImGuiMCP::TableSetColumnIndex(3);
                    ImGuiMCP::TextUnformatted(rule.isPermanentEnabled ? "Yes" : "No");

                    // --- durationFilter ---
                    ImGuiMCP::TableSetColumnIndex(4);
                    if (rule.durationFilter < 0.0f) {
                        ImGuiMCP::TextDisabled("(default)");
                    } else {
                        ImGuiMCP::Text("%.2f", rule.durationFilter);
                    }

                    // --- minDurationFilter / magnitudeFilter ---
                    // ImGuiMCP::TableSetColumnIndex(5);
                    // {
                    //     std::string magStr =
                    //         (rule.magnitudeFilter < 0.0f)
                    //             ? "(default)"
                    //             : (std::ostringstream{} << std::fixed << std::setprecision(2) <<
                    //             rule.magnitudeFilter)
                    //                   .str();
                    //     ImGuiMCP::Text("min:%u / mag:%s", rule.minDurationFilter, magStr.c_str());
                    // }

                    ImGuiMCP::PopID();
                }

                ImGuiMCP::EndTable();
            }
        }

        ImGuiMCP::SetWindowFontScale(1.0f);
    }

    void __stdcall RenderSavedSpellsTable() {
        const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();

        // static bool wasOpenLastFrame = false;
        // bool isOpenThisFrame = true;  // Render is only called while this tab is visible

        // if (isOpenThisFrame && !wasOpenLastFrame) {
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                logger::debug("Pruning unapplied saved spells.");
                PruneUnappliedSavedSpells(*player);
            }
        // }
        // wasOpenLastFrame = isOpenThisFrame;

        ImGuiMCP::SetWindowFontScale(0.93f);
        ImGuiMCP::SeparatorText("SAVED SPELLS");

        ImGuiMCP::Text("Total saved: %zu", savedSpells.size());
        ImGuiMCP::SameLine(0.0f, 14.0f);

        ImGuiMCP::BeginDisabled(savedSpells.empty());
        if (ImGuiMCP::Button("Dispel All")) {
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                // Copy keys first: mutating the real store,
                std::vector<RE::FormID> allIDs;
                allIDs.reserve(savedSpells.size());
                for (const auto& [spellFormID, effectFormIDs] : savedSpells) {
                    allIDs.push_back(spellFormID);
                }

                for (RE::FormID id : allIDs) {
                    RE::TESForm* spellForm = RE::TESForm::LookupByID(id);
                    RE::SpellItem* spellItem = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
                    if (spellItem) {
                        SpellUtilities::DispelSpellItemFromActor(player, spellItem);
                    }
                    SpellDataPersistence::RemoveSpellFromSave(id);
                }
            }
        }
        ImGuiMCP::EndDisabled();

        ImGuiMCP::Spacing();

        static ImGuiMCP::ImGuiTableFlags flags = ImGuiMCP::ImGuiTableFlags_Borders | ImGuiMCP::ImGuiTableFlags_RowBg |
                                                 ImGuiMCP::ImGuiTableFlags_Resizable |
                                                 ImGuiMCP::ImGuiTableFlags_ScrollY;

        // Cap the table height so it doesn't push the rest of the menu off-screen
        ImGuiMCP::ImVec2 outerSize(0.0f, ImGuiMCP::GetTextLineHeightWithSpacing() * 12.0f);

        // Collect FormIDs to remove; erase happens after the loop, never during it.
        std::vector<RE::FormID> toRemove;

        if (ImGuiMCP::BeginTable("SavedSpellsTable", 4, flags, outerSize)) {
            ImGuiMCP::TableSetupScrollFreeze(0, 1);
            ImGuiMCP::TableSetupColumn("Spell", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.30f);
            ImGuiMCP::TableSetupColumn("FormID", ImGuiMCP::ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGuiMCP::TableSetupColumn("Effects", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGuiMCP::TableSetupColumn("Action", ImGuiMCP::ImGuiTableColumnFlags_WidthFixed, 70.0f);  // new
            ImGuiMCP::TableHeadersRow();

            for (const auto& [spellFormID, effectFormIDs] : savedSpells) {
                ImGuiMCP::TableNextRow();

                // --- Column 1: Spell name ---
                ImGuiMCP::TableSetColumnIndex(0);
                RE::TESForm* spellForm = RE::TESForm::LookupByID(spellFormID);
                const char* spellName = (spellForm && spellForm->GetName() && spellForm->GetName()[0])
                                            ? spellForm->GetName()
                                            : "<unknown spell>";
                ImGuiMCP::TextUnformatted(spellName);

                // --- Column 2: FormID (hex) ---
                ImGuiMCP::TableSetColumnIndex(1);
                ImGuiMCP::Text("%08X", spellFormID);

                // --- Column 3: Effects (resolved names, comma separated) ---
                ImGuiMCP::TableSetColumnIndex(2);
                if (effectFormIDs.empty()) {
                    ImGuiMCP::TextDisabled("(no effects)");
                } else {
                    std::string effectsText;
                    for (size_t i = 0; i < effectFormIDs.size(); ++i) {
                        RE::TESForm* effectForm = RE::TESForm::LookupByID(effectFormIDs[i]);
                        const char* effectName = (effectForm && effectForm->GetName() && effectForm->GetName()[0])
                                                     ? effectForm->GetName()
                                                     : "<unknown>";
                        effectsText += effectName;
                        if (i + 1 < effectFormIDs.size()) {
                            effectsText += ", ";
                        }
                    }
                    ImGuiMCP::TextWrapped("%s", effectsText.c_str());
                }
                // --- Column 4: Dispel button ---
                ImGuiMCP::TableSetColumnIndex(3);

                RE::SpellItem* spellItem = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;

                ImGuiMCP::BeginDisabled(spellItem == nullptr);
                if (ImGuiMCP::Button(("Dispel###Dispel" + std::to_string(spellFormID)).c_str())) {
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        SpellUtilities::DispelSpellItemFromActor(player, spellItem);
                    }
                    toRemove.push_back(spellFormID);
                }
            }

            ImGuiMCP::EndTable();
        }

        // ImGuiMCP::SetWindowFontScale(1.0f);

        for (RE::FormID id : toRemove) {
            SpellDataPersistence::RemoveSpellFromSave(id);
        }
    }

    void __stdcall RenderGeneralRuleGeneric() {
        Config& config = Config::GetSingleton();
        GeneralRule& generalRule = config.GetGeneralRule();
        ConfigLoader& configLoader = ConfigLoader::GetSingleton();
        static const std::unordered_map<std::string, const char*> descriptions = {
            {"Enable", "Master toggle. When off, this mod has no effect."},
            {"Shouts", "Allows shouts to be affected. Recommended to be disabled."},
            {"Spells", "Allows regular spells to be affected. Recommended to be enabled."},
            {"Summons", "Allows summon spells to be affected. Recommended to be enabled."},
            {"LesserPowers", "Allows lesser powers to be affected. Recommended to be disabled."},
            {"GreaterPowers", "Allows greater powers to be affected. Recommended to be disabled."},
            {"Scrolls", "Allows scrolls to be affected. Recommended to be disabled."},
            {"Recastable",
             "If enabled, non-recastable spells are affected. Some spells can not be recasted while their effect is "
             "still running. They must be dispelled manually."},
            // {"ReserveMagicka", "Not implemented. Reserves magicka for this spell type."},
        };

        ImGuiMCP::SetWindowFontScale(0.93f);
        ImGuiMCP::SeparatorText("GENERAL");

        for (const auto& [name, boolPtr] : generalRule.GetFieldsMatchedConfigFile()) {
            if (!boolPtr) continue;

            // ImGuiMCP::Checkbox(name.c_str(), boolPtr);

            if (ImGuiMCP::Checkbox(name.c_str(), boolPtr)) {
                // *boolPtr already holds the new value here.
                configLoader.UpdateConfigValue(config.ConfigFilePath, "General", name, *boolPtr ? "true" : "false");

                // .GetConfigLoader().UpdateConfigValue(config.ConfigFilePath, "general", name,
                //                                            *boolPtr ? "true" : "false");
            }

            if (auto it = descriptions.find(name); it != descriptions.end()) {
                HelpMarker(it->second);
            }
        }
    }

    void __stdcall Render() {
        // static bool wasOpenLastFrame = false;
        // bool isOpenThisFrame = true; // Render is only called while this tab is visible

        // if (isOpenThisFrame && !wasOpenLastFrame) {
        //     if (auto* player = RE::PlayerCharacter::GetSingleton()) {
        //         logger::debug("Pruning unapplied saved spells.");
        //         PruneUnappliedSavedSpells(*player);
        //     }
        // }
        // wasOpenLastFrame = isOpenThisFrame;

        RenderGeneralRuleGeneric();
        RenderSavedSpellsTable();
        RenderSpellRulesTable();

        ImGuiMCP::SetWindowFontScale(1.0f);
    }
}