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
}

namespace SettingsUI {
    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("[Eternal buffs] SKSE Menu Framework not installed. Settings menu skipped.");
            return;
        }

        SKSEMenuFramework::SetSection("Eternal Buffs");
        SKSEMenuFramework::AddSectionItem("Settings", SettingsUI::Render);
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
             "still running. They must be dispelled manually. Some cloak spells have non-recastable effects."},
            // {"ReserveMagicka", "Not implemented. Reserves magicka for this spell type."},
        };

        ImGuiMCP::SetWindowFontScale(0.93f);
        ImGuiMCP::SeparatorText("GENERAL");

        for (const auto& [name, boolPtr] : generalRule.GetFieldsMatchedConfigFile()) {
            if (!boolPtr) continue;

            if (ImGuiMCP::Checkbox(name.c_str(), boolPtr)) {
                configLoader.UpdateConfigValue(config.ConfigFilePath, "General", name, *boolPtr ? "true" : "false");
            }

            if (auto it = descriptions.find(name); it != descriptions.end()) {
                HelpMarker(it->second);
            }
        }
    }

    void __stdcall Render() {
        RenderGeneralRuleGeneric();
        RenderSavedSpellsTable();
        RenderSpellRulesTable();

        ImGuiMCP::SetWindowFontScale(1.0f);
    }
}