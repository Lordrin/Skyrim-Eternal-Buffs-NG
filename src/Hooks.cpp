#include "Hooks.h"

namespace EBuffs::Hooks {
    struct OnApply {
        static void thunk(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect) {
            logger::info("EBuffs: OnApply: MagicTarget::AddEffect called.");
            // Custom logic: get actor safely
            if (a_effect) {
                if (auto actor = a_effect->GetTargetActor(); actor) {
                    logger::info("EBuffs: Actor {} (0x{:X}) got effect {}", actor->GetName(), actor->GetFormID(),
                                 a_effect->spell ? a_effect->spell->avEffectSetting->GetFormID() : 0);
                }
            }
            // Chain to original
            func(a_target, a_effect);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x08};  // slot for EffectAdded
    };

    void Install() {
        logger::info("EBuffs: Installing EffectAdded hook…");
        // Hook the MagicTarget vtable (covers actor and non-actor)
        stl::write_vfunc<RE::MagicTarget, OnApply>();
        logger::info("EBuffs: EffectAdded hook installed.");
    }
}

namespace Book::Hooks {
    enum DefaultAction {
        kAuto = 0,
        kTake,
        kRead,
    };

    struct GetActivateText {
        struct detail {
        public:
            static std::string get_book_text(RE::TESObjectBOOK* a_this, RE::TESObjectREFR* a_activator) {
                // const auto settings = Settings::GetSingleton();
                const auto bookSpell = a_this->GetSpell();
                const auto isHotkeyPressed = Config::GetSingleton().GetToggleKeyHeld();

                // if (bookSpell && !settings->GetAutoUseSpellTomes()) {
                // 	return get_activate_label(a_activator, true);
                // }

                const auto take_label_impl = [&]() { return get_activate_label(a_activator, !isHotkeyPressed); };
                const auto read_label = [&]() { return get_activate_label(a_activator, isHotkeyPressed); };
                const auto take_label = [&]() { return a_this->CanBeTaken() ? take_label_impl() : read_label(); };

                const auto spell_tome_label = [&]() {
                    const auto actor = a_activator->As<RE::Actor>();
                    if (actor && actor->HasSpell(bookSpell) || a_this->IsRead()) {
                        return take_label();
                    }
                    return read_label();
                };

                switch (kRead) {
                    case kTake:
                        return take_label();
                    case kAuto: {
                        if (bookSpell) {
                            return spell_tome_label();
                        }
                        if (a_this->IsRead()) {
                            return take_label();
                        }
                        return read_label();
                    }
                    case kRead:
                        return read_label();
                    default:
                        return get_activate_label(a_activator, false);
                }
            }

        private:
            static std::string get_activate_label(RE::TESObjectREFR* a_activator, bool a_take) {
                if (a_take) {
                    return a_activator->IsCrimeToActivate()
                               ? RE::GameSettingCollection::GetSingleton()->GetSetting("sSteal")->GetString()
                               : RE::GameSettingCollection::GetSingleton()->GetSetting("sTake")->GetString();
                } else {
                    return RE::GameSettingCollection::GetSingleton()->GetSetting("sRead")->GetString();
                }
            }
        };

        static bool thunk(RE::TESObjectBOOK* a_this, RE::TESObjectREFR* a_activator, RE::BSString& a_dst) {
            a_dst =
                std::format("{}\n{}", detail::get_book_text(a_this, a_activator), a_activator->GetDisplayFullName());

            return true;
        }
        static inline REL::Relocation<decltype(thunk)> func;

        static inline constexpr std::size_t idx = 0x4C;
    };

    struct Activate {
        struct detail {
            static bool activate_book(RE::TESObjectBOOK* a_this, RE::TESObjectREFR* a_targetRef,
                                      RE::TESObjectREFR* a_activatorRef, std::int32_t a_targetCount, bool a_read) {
                const bool teachesSpell = a_this->TeachesSpell();

                if (a_read && !teachesSpell) {
                    const auto root = a_targetRef->Get3D();
                    const auto transform = root ? root->world : RE::NiTransform{};

                    RE::BSString str;
                    a_this->GetDescription(str, nullptr);
                    RE::BookMenu::OpenBookMenu(str, &a_targetRef->extraList, a_targetRef, a_this, transform.translate,
                                               transform.rotate, transform.scale, true);
                } else {
                    if (!a_this->CanBeTaken()) {
                        return false;
                    }

                    if (const auto actor = a_activatorRef->As<RE::Actor>(); actor) {
                        actor->PickUpObject(a_targetRef, a_targetCount);
                        if (teachesSpell && a_read && a_this->Read(actor)) {
                            actor->RemoveItem(a_this, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                        }
                    }
                }
                return true;
            }
        };

        static bool thunk(RE::TESObjectBOOK* a_this, RE::TESObjectREFR* a_targetRef, RE::TESObjectREFR* a_activatorRef,
                          std::uint8_t, RE::TESBoundObject*, std::int32_t a_targetCount) {
            if (a_activatorRef) {
                if (a_activatorRef->IsPlayerRef()) {
                    // const auto settings = Settings::GetSingleton();
                    const auto bookSpell = a_this->GetSpell();

                    // if (bookSpell && !settings->GetAutoUseSpellTomes()) {
                    //     if (!detail::activate_book(a_this, a_targetRef, a_activatorRef, a_targetCount, false)) {
                    //         return false;
                    //     }
                    // } else {
                    const auto isHotkeyPressed = Config::GetSingleton().GetToggleKeyHeld();

                    const auto take_impl = [&]() {
                        return detail::activate_book(a_this, a_targetRef, a_activatorRef, a_targetCount,
                                                     isHotkeyPressed);
                    };
                    const auto read = [&]() {
                        return detail::activate_book(a_this, a_targetRef, a_activatorRef, a_targetCount,
                                                     !isHotkeyPressed);
                    };
                    const auto take = [&]() { return a_this->CanBeTaken() ? take_impl() : read(); };

                    switch (kRead) {
                        case kAuto: {
                            if (bookSpell) {
                                return a_this->IsRead() ? take() : read();
                            }
                            if (a_this->IsRead()) {
                                return take();
                            }
                            return read();
                        }
                        case kTake:
                            return take();
                        case kRead:
                            return read();
                    }
                    // }
                } else {
                    if (!detail::activate_book(a_this, a_targetRef, a_activatorRef, a_targetCount, false)) {
                        return false;
                    }
                }
            }

            return true;
        }
        static inline REL::Relocation<decltype(thunk)> func;

        static inline constexpr std::size_t idx = 0x37;
    };

    void Install() {
        // Settings::GetSingleton()->LoadSettings();

        stl::write_vfunc<RE::TESObjectBOOK, Activate>();
        stl::write_vfunc<RE::TESObjectBOOK, GetActivateText>();
    }
}

namespace ActiveEffect::Hooks {
    // virtual void           OnAdd(MagicTarget* a_target);                              // 01
    // 	virtual void           OnRemove();                                                // 02 - { return; }
    // virtual void           Update(float a_delta);                                     // 04 - { return; }
    // 	virtual void           EvaluateConditions(float a_delta, bool a_forceUpdate);     // 05
    //     virtual void  Start();                                                   // 14 - { return; }
    // 	virtual void  Finish();             // 15 - { return; }

    struct onAdd {
        static void thunk(RE::ActiveEffect* a_this, RE::MagicTarget* a_target) {
            logger::info("EBuffs: OnAdd: ActiveEffect::AddEffect called.");
            func(a_this, a_target);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x01};
    };

    struct onRemove {
        static void thunk(RE::ActiveEffect* a_this) {
            logger::info("EBuffs: OnRemove: ActiveEffect::RemoveEffect called.");
            func(a_this);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x02};
    };

    struct Update {
        static void thunk(RE::ActiveEffect* a_this, float a_delta) {
            // if (a_this && a_this->GetBaseObject()) {
            //     logger::info("EBuffs: Update: ActiveEffect::Update called. Effect: {}",
            //                  a_this->GetBaseObject()->GetName());
            // }
            logger::info("EBuffs: Update: ActiveEffect::Update called.");
            func(a_this, a_delta);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x04};
    };

    struct EvaluateConditions {
        static void thunk(RE::ActiveEffect* a_this, float a_delta, bool a_forceUpdate) {
            logger::info("EBuffs: EvaluateConditions: ActiveEffect::EvaluateConditions called.");
            func(a_this, a_delta, a_forceUpdate);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x05};
    };

    struct Start {
        static void thunk(RE::ActiveEffect* a_this) {
            func(a_this);
            if (a_this) {
                try {
                    if (a_this->GetBaseObject()) {
                        logger::info("EBuffs: Start: ActiveEffect::Start called. Effect: {}",
                                     a_this->GetBaseObject()->GetName());
                    }
                }

                catch (const std::exception& e) {
                    logger::error("EBuffs: Start: ActiveEffect::Start failed: {}", e.what());
                }
            }

            logger::info("EBuffs: Start: ActiveEffect::Start called.");
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x14};
    };

    struct Finish {
        static void thunk(RE::ActiveEffect* a_this) {
            // if (a_this && a_this->GetBaseObject()) {
            //     logger::info("EBuffs: Finish: ActiveEffect::Finish called. Effect: {}",
            //                  a_this->GetBaseObject()->GetName());
            // }
            logger::info("EBuffs: Finish: ActiveEffect::Finish called.");
            func(a_this);
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x15};
    };

    void Install() {
        REL::Relocation<uintptr_t> Vtable{RE::VTABLE_ActiveEffectReferenceEffectController[0]};

        onAdd::func = Vtable.write_vfunc(onAdd::idx, &onAdd::thunk);
        onRemove::func = Vtable.write_vfunc(onRemove::idx, &onRemove::thunk);
        Update::func = Vtable.write_vfunc(Update::idx, &Update::thunk);
        EvaluateConditions::func = Vtable.write_vfunc(EvaluateConditions::idx, &EvaluateConditions::thunk);
        Start::func = Vtable.write_vfunc(Start::idx, &Start::thunk);
        Finish::func = Vtable.write_vfunc(Finish::idx, &Finish::thunk);

        // stl::write_vfunc<RE::ActiveEffect, onAdd>();
        // stl::write_vfunc<RE::ActiveEffect, onRemove>();
        // stl::write_vfunc<RE::ActiveEffect, Update>();
        // stl::write_vfunc<RE::ActiveEffect, EvaluateConditions>();
        // stl::write_vfunc<RE::ActiveEffect, Start>();
        // stl::write_vfunc<RE::ActiveEffect, Finish>();
    }
}

namespace ActorMagicCaster::Hooks {
    struct CasterHook {
        static void SpellCast(RE::ActorMagicCaster* caster, bool a_doCast, std::uint32_t a_arg2,
                              RE::MagicItem* a_spell) {
            // logger::info("EBuffs: SpellCast: ActorMagicCaster::SpellCast called.");
            // if (a_spell) {
            //     logger::info("EBuffs: SpellCast: ActorMagicCaster::SpellCast called. Spell: {}, ",
            //     a_spell->GetName());
            // }

            // if (caster->actor) {
            //     logger::info("EBuffs: SpellCast: ActorMagicCaster::SpellCast called. Spell: {}, caster: {}.",
            //                  a_spell->GetName(), caster->actor->GetName());
            // }
            func(caster, a_doCast, a_arg2, a_spell);
        }
        static inline REL::Relocation<decltype(SpellCast)> func;
        static bool CheckCast(RE::ActorMagicCaster* caster, RE::MagicItem* spell, bool dualCast, float* alchStrength,
                              RE::MagicSystem::CannotCastReason* a_reason, bool useBaseValueForCost) {
            // logger::info("EBuffs: CheckCast: ActorMagicCaster::CheckCast called.");
            // if (spell) {
            //     logger::info("EBuffs: SpellCast: ActorMagicCaster::SpellCast called. Spell: {}, ", spell->GetName());
            // }
            // if (caster->actor) {
            //     logger::info("EBuffs: CheckCast: ActorMagicCaster::CheckCast called. Spell: {}, caster: {}.",
            //                  spell->GetName(), caster->actor->GetName());
            // }
            // return true;
            return funcA(caster, spell, dualCast, alchStrength, a_reason, useBaseValueForCost);
        }
        static inline REL::Relocation<decltype(CheckCast)> funcA;
    };
    void Install() {
        REL::Relocation<uintptr_t> ActorCasterVtbl{RE::VTABLE_ActorMagicCaster[0]};
        CasterHook::func = ActorCasterVtbl.write_vfunc(0x9, &CasterHook::SpellCast);
        CasterHook::funcA = ActorCasterVtbl.write_vfunc(0xA, &CasterHook::CheckCast);
    }
}

namespace ActiveEffectReferenceEffectController::Hooks {
    struct ActiveEffectReferenceEffectControllerHook {
        static RE::TESObjectREFR* GetTargetReference(RE::ActiveEffectReferenceEffectController* a_this) {
            if (a_this && a_this->target && a_this->effect) {
                auto player = RE::PlayerCharacter::GetSingleton();
                if (a_this->target.get().get() == player) {
                    logger::info(
                        "EBuffs: GetTargetReference: ActiveEffectReferenceEffectController::GetTargetReference called. "
                        "Actor: {}, Effect: {}, Spell: {}",
                        a_this->target.get().get()->GetName(), a_this->effect->GetBaseObject()->GetName(),
                        a_this->effect->spell ? a_this->effect->spell->GetName() : "Not found");
                }
            }
            logger::info(
                "EBuffs: GetTargetReference: ActiveEffectReferenceEffectController::GetTargetReference called.");

            // if (*a_ref) {
            //     logger::info(
            //         "EBuffs: GetTargetReference: ActiveEffectReferenceEffectController::GetTargetReference called. "
            //         "Target: {}",
            //         (*a_ref)->GetName());
            // }

            // if (a_this && a_this->effect) {
            //     logger::info(
            //         "EBuffs: GetTargetReference: ActiveEffectReferenceEffectController::GetTargetReference called. "
            //         "Effect: {}",
            //         a_this->effect->GetBaseObject()->GetName());
            // }

            // if (a_this && a_this->target) {
            //     logger::info(
            //         "EBuffs: GetTargetReference: ActiveEffectReferenceEffectController::GetTargetReference called. "
            //         "Actor: {}",
            //         a_this->target.get().get()->GetName());
            // }

            return func(a_this);
        }
        static inline REL::Relocation<decltype(GetTargetReference)> func;
        static constexpr std::size_t idx{0x0B};
    };

    void Install() {
        REL::Relocation<uintptr_t> ActorCasterVtbl{RE::VTABLE_ActiveEffectReferenceEffectController[0]};
        ActiveEffectReferenceEffectControllerHook::func =
            ActorCasterVtbl.write_vfunc(0xB, &ActiveEffectReferenceEffectControllerHook::GetTargetReference);
    }
};
// namespace EBuffs::Hooks {
//     struct OnApply {
//         struct detail {
//             static void on_add(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect) {
//                 logger::info("EBuffs: OnApply: MagicTarget::AddEffect called.");

//                 if (!a_effect) {
//                     logger::info("EBuffs: OnApply: Effect is null.");
//                     return;
//                 }
//                 if (!a_target) {
//                     logger::info("EBuffs: OnApply: Target is null.");
//                     return;
//                 }

//                 if (auto actor = a_effect->GetTargetActor(); actor) {
//                     // Now ‘actor’ is your RE::Actor*

//                     // if (auto actor = a_target->As<RE::Actor>(); actor) {
//                     //     // Safe, no RTTI required
//                     //     logger::info("FormID: {:X}", actor->GetFormID());
//                     // }

//                     if (a_target->MagicTargetIsActor()) {
//                         // Attempt the dynamic cast
//                         // RE::Actor* actor = dynamic_cast<RE::Actor*>(a_target);

//                         // ALWAYS check if the cast was successful
//                         if (actor) {
//                             // Successfully converted! Now you can use the 'actor' pointer.
//                             logger::info(
//                                 "EBuffs: OnApply: Successfully cast MagicTarget to Actor. FormID: {:X}, Name: {}",
//                                 actor->GetFormID(), actor->GetName());
//                             // Do something with the actor...
//                             // actor->DoSomething();
//                         } else {
//                             // This case is unlikely if MagicTargetIsActor() was true,
//                             // but good practice for dynamic_cast. Could indicate RTTI issues or hierarchy problems.
//                             logger::info(
//                                 "EBuffs: OnApply: MagicTargetIsActor() was true, but dynamic_cast failed.");  //
//                                 GetTargetReference()
//                             // often
//                             // returns the
//                             // REFR
//                         }
//                     } else {
//                         logger::info("EBuffs: OnApply: Target is not an Actor.");
//                     }
//                 }
//             }
//             // static void on_add(RE::MagicTarget* a_target) {
//             //     if (!a_target) {
//             //         logger::info("EBuffs: OnApply: Target is null.");
//             //         return;
//             //     }

//             //     if (a_target->MagicTargetIsActor()) {
//             //         // Attempt the dynamic cast
//             //         RE::Actor* actor = dynamic_cast<RE::Actor*>(a_target);

//             //         // ALWAYS check if the cast was successful
//             //         if (actor) {
//             //             // Successfully converted! Now you can use the 'actor' pointer.
//             //             logger::info("EBuffs: OnApply: Successfully cast MagicTarget to Actor. FormID: {:X},
//             //             Name:{}",
//             //                          actor->GetFormID(), actor->GetName());
//             //             // Do something with the actor...
//             //             // actor->DoSomething();
//             //         } else {
//             //             // This case is unlikely if MagicTargetIsActor() was true,
//             //             // but good practice for dynamic_cast. Could indicate RTTI issues or hierarchy problems.
//             //             logger::info(
//             //                 "EBuffs: OnApply: MagicTargetIsActor() was true, but dynamic_cast failed.");  //
//             //                 GetTargetReference()
//             //                                                                                               // often
//             //                                                                                               //
//             returns
//             //                                                                                               the
//             //                                                                                               // REFR
//             //         }
//             //     } else {
//             //         logger::info("EBuffs: OnApply: Target is not an Actor.");
//             //     }
//             // }
//         };
//         static void thunk(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect) {
//             detail::on_add(a_target, a_effect);
//             func(a_target, a_effect);  // invoke original behavior
//         }

//         static inline REL::Relocation<decltype(thunk)> func;
//         // static inline REL::Relocation<decltype(thunk)> orig;
//         static constexpr std::size_t idx{0x08};
//         // static inline constexpr std::size_t size = 0x08;
//     };

//     struct MagicCasterSpellCast_Hook {
//         // virtual void                       SpellCast(bool a_doCast, std::uint32_t a_arg2, MagicItem* a_spell); //
//         // 09
//         struct detail {
//             static void on_cast(RE::MagicTarget* a_caster, RE::MagicItem* a_magicItem) {
//                 logger::info("EBuffs: MagicCasterSpellCast hook triggered.");
//             }
//         };

//         static void thunk(RE::MagicTarget* a_this, RE::MagicItem* a_magicItem) { detail::on_cast(a_this,
//         a_magicItem); }

//         static inline REL::Relocation<decltype(thunk)> func;
//         static constexpr std::size_t idx{0x09};
//     };

//     void Install() {
//         logger::info("EBuffs: Installing hooks...");
//         auto x = RE::MagicTarget::VTABLE;
//         // auto y = RE::ActorMagicCaster::VTABLE;
//         stl::write_vfunc<RE::Actor, OnApply>();
//         // stl::write_vfunc<RE::MagicTarget, MagicCasterSpellCast_Hook>();
//         logger::info("EBuffs: Hooks installed.");  // Check if this line appears
//     }
// }

// namespace EBuffs::Hooks
// {
//     // Structure to manage the hook for MagicTarget::AddEffect
//     struct MagicTarget_AddEffect_Hook
//     {
//         // This is the function signature for the *hook*.
//         // It must match the original member function's signature,
//         // but with the 'this' pointer added as the *first* argument.
//         // Original: void AddEffect(ActiveEffect* a_effect);
//         // Hooked:   void Hooked(MagicTarget* a_this, ActiveEffect* a_effect);
//         static void Hooked(RE::MagicTarget* a_thisMagicTarget, RE::ActiveEffect* a_effect)
//         {
//             // --- Your Custom Code Start ---

//             // Basic null checks (good practice, though often unnecessary here if game guarantees validity)
//             if (!a_thisMagicTarget) {
//                  logger::warn("EBuffs: MagicTarget::AddEffect hook triggered with null target (this). Skipping custom
//                  logic.");
//                  // Still call original in case the game somehow expects this call flow
//                  _OriginalFunc(a_thisMagicTarget, a_effect);
//                  return;
//             }
//             if (!a_effect) {
//                  logger::warn("EBuffs: MagicTarget::AddEffect hook triggered with null effect on target. Skipping
//                  custom logic.");
//                  // Still call original
//                  _OriginalFunc(a_thisMagicTarget, a_effect);
//                  return;
//             }

//             // Get Base Effect info (EffectSetting / MGEF)
//             RE::EffectSetting* baseEffect = a_effect->GetBaseObject();
//             std::string effectName = baseEffect ? baseEffect->GetName() : "UNKNOWN EFFECT";
//             RE::FormID effectFormID = baseEffect ? baseEffect->GetFormID() : 0;

//             logger::info("EBuffs: AddEffect Hook: Applying Effect '{}' ({:X}) to Target Ref Handle",
//                          effectName,
//                          effectFormID);

//             // Check if the target is an Actor
//             // Note: MagicTarget has multiple derived classes (Actor, TESObjectREFR for non-actors)
//             if (a_thisMagicTarget->MagicTargetIsActor()) // Use the built-in check
//             {
//                 // dynamic_cast is the safest way to convert, though static_cast might work
//                 // if you are absolutely certain after the MagicTargetIsActor check.
//                 RE::Actor* targetActor = dynamic_cast<RE::Actor*>(a_thisMagicTarget);

//                 if (targetActor) {
//                      logger::info("    -> Target is Actor: '{}' (FormID: {:X}, Base FormID: {:X})",
//                                   targetActor->GetName(),
//                                   targetActor->GetFormID(),        // The instance ID
//                                   targetActor->GetBaseObject() ? targetActor->GetBaseObject()->GetFormID() : 0); //
//                                   The base NPC_ ID

//                     // --- THIS IS WHERE YOU ADD YOUR ACTOR-SPECIFIC LOGIC ---
//                     // Example: Check if the effect is Stoneflesh
//                     // Make sure to handle potential null pointer from GetBaseObject()
//                     const RE::FormID stonefleshFormID = 0x12F05; // Example FormID for Stoneflesh MGEF (Vanilla
//                     SE/AE) - VERIFY! if (baseEffect && baseEffect->GetFormID() == stonefleshFormID) {
//                         logger::info("    -> Stoneflesh applied to {}", targetActor->GetName());
//                         // Do something specific for Stoneflesh...
//                     }
//                     // --------------------------------------------------------

//                 } else {
//                     // This should technically not happen if MagicTargetIsActor() returned true,
//                     // but good to log just in case (could indicate RTTI issues).
//                     logger::error("    -> MagicTargetIsActor() was true, but dynamic_cast to Actor failed for target
//                     handle!");
//                 }
//             } else {
//                  // It's a MagicTarget but not an Actor (e.g., could be an inanimate object targetted by a spell)
//                  RE::TESObjectREFR* targetRef = nullptr;
//                  auto refHandle = a_thisMagicTarget->GetTargetStatsObject(); // Try to get the underlying
//                  TESObjectREFR if (refHandle) {
//                      targetRef = refHandle;
//                     //  targetRef = refHandle.get().get();
//                  }
//                  if (targetRef) {
//                     logger::info("    -> Target is not an Actor. Target Ref: '{}' (FormID: {:X}, Base FormID: {:X})",
//                                  targetRef->GetName(),
//                                  targetRef->GetFormID(),
//                                  targetRef->GetBaseObject() ? targetRef->GetBaseObject()->GetFormID() : 0);
//                  } else {
//                      logger::info("    -> Target is not an Actor and couldn't resolve to TESObjectREFR (Target
//                      Handle).");
//                  }
//             }

//             // --- Your Custom Code End ---

//             // **VERY IMPORTANT:** Call the original function!
//             // If you don't do this, the effect will never actually be added, breaking game mechanics.
//             _OriginalFunc(a_thisMagicTarget, a_effect);

//             logger::debug("EBuffs: Finished AddEffect Hook for Target Handle, Effect '{}'", effectName);
//         }

//         // Static member to store the pointer to the original function
//         // The type must match the hook function's signature
//         static inline REL::Relocation<decltype(&Hooked)> _OriginalFunc;

//         // Installs the hook
//         static bool Install()
//         {
//             // <<< === STEP 1: Replace with the correct ID for your Skyrim Version (SE or AE) === >>>
//              constexpr REL::ID MagicTarget_AddEffect_ID_AE(51960); // Example for AE 1.6.x
//              constexpr REL::ID MagicTarget_AddEffect_ID_SE(51057); // Example for SE 1.5.97

//             // Select the ID based on the runtime version
//             REL::ID functionID = REL::Module::IsAE() ? MagicTarget_AddEffect_ID_AE : MagicTarget_AddEffect_ID_SE;
//              // <<< ============================================================================== >>>

//              logger::info("EBuffs: Attempting to hook MagicTarget::AddEffect (ID: {})...", functionID.id());

//              // Get the trampoline interface
//              // Using the global trampoline is generally recommended
//             SKSE::Trampoline& trampoline = SKSE::GetTrampoline();

//              // Use write_call<5> to replace a 5-byte call instruction pointing to AddEffect
//              // This is a common hooking method. It patches calls *to* AddEffect.
//              // If AddEffect is called directly via vtable in some places, this specific hook point might miss it,
//              // but it catches many common cases like spell application.
//              // Alternatively, you could hook the function's entry point directly using write_branch<5>,
//              // but write_call is often sufficient and sometimes safer depending on the function.
//              // Let's stick with write_call as it's conceptually simpler for this case.
//              // We need the address from the REL::ID object.
//             // _OriginalFunc = trampoline.write_call<5>(functionID.address(), Hooked);

//             //  if (_OriginalFunc) {
//             //      logger::info("EBuffs: Successfully hooked MagicTarget::AddEffect.");
//             //      return true;
//             //  } else {
//             //      logger::error("EBuffs: Failed to hook MagicTarget::AddEffect!");
//             //      return false;
//             //  }

//              // Call write_call and capture the *result* (the address of the original/trampoline)
//              // The return type is typically std::uintptr_t or similar pointer-like type.
//              auto originalFuncAddress = trampoline.write_call<5>(functionID.address(), Hooked);

//              // Check if the write_call succeeded by checking the returned address
//              // A null or zero address indicates failure.
//              if (originalFuncAddress) {
//                  // Success! Store the returned address in our Relocation object for later calls.
//                  // REL::Relocation can be assigned from a raw address.
//                  _OriginalFunc = originalFuncAddress;

//                  logger::info("EBuffs: Successfully hooked MagicTarget::AddEffect at address 0x{:X}",
//                  functionID.address()); return true;
//              } else {
//                  // Failure! The trampoline hook could not be written.
//                  logger::error("EBuffs: Failed to hook MagicTarget::AddEffect (trampoline.write_call failed for ID
//                  {})!", functionID.id()); return false;
//              }
//         }
//     };

//     // Your main Install function for all hooks in this namespace
//     void Install() {
//         logger::info("EBuffs: Installing hooks...");

//         if (MagicTarget_AddEffect_Hook::Install()) {
//             // Hook installed successfully
//         } else {
//             // Handle hook installation failure if needed
//         }

//         // Install other hooks here if you have more...

//         logger::info("EBuffs: Hooks installation process finished.");
//     }

// } // namespace EBuffs::Hooks

// namespace EBuffs::Hooks
// {
//     // Structure to manage the hook for MagicTarget::AddEffect
//     struct MagicTarget_AddEffect_Hook
//     {
//         // This is the function signature for the *hook*.
//         // It must match the original member function's signature,
//         // but with the 'this' pointer added as the *first* argument.
//         // Original: void AddEffect(ActiveEffect* a_effect);
//         // Hooked:   void Hooked(MagicTarget* a_this, ActiveEffect* a_effect);
//         static void Hooked(RE::MagicTarget* a_thisMagicTarget, RE::ActiveEffect* a_effect)
//         {
//             // --- Your Custom Code Start ---

//             // Basic null checks (good practice, though often unnecessary here if game guarantees validity)
//             if (!a_thisMagicTarget) {
//                  logger::warn("EBuffs: MagicTarget::AddEffect hook triggered with null target (this). Skipping custom
//                  logic.");
//                  // Still call original in case the game somehow expects this call flow
//                  _OriginalFunc(a_thisMagicTarget, a_effect);
//                  return;
//             }
//             if (!a_effect) {
//                  logger::warn("EBuffs: MagicTarget::AddEffect hook triggered with null effect on target. Skipping
//                  custom logic.");
//                  // Still call original
//                  _OriginalFunc(a_thisMagicTarget, a_effect);
//                  return;
//             }

//             // Get Base Effect info (EffectSetting / MGEF)
//             RE::EffectSetting* baseEffect = a_effect->GetBaseObject();
//             std::string effectName = baseEffect ? baseEffect->GetName() : "UNKNOWN EFFECT";
//             RE::FormID effectFormID = baseEffect ? baseEffect->GetFormID() : 0;

//             logger::info("EBuffs: AddEffect Hook: Applying Effect '{}' ({:X}) to Target Ref Handle",
//                          effectName,
//                          effectFormID);

//             // Check if the target is an Actor
//             // Note: MagicTarget has multiple derived classes (Actor, TESObjectREFR for non-actors)
//             if (a_thisMagicTarget->MagicTargetIsActor()) // Use the built-in check
//             {
//                 // dynamic_cast is the safest way to convert, though static_cast might work
//                 // if you are absolutely certain after the MagicTargetIsActor check.
//                 RE::Actor* targetActor = dynamic_cast<RE::Actor*>(a_thisMagicTarget);

//                 if (targetActor) {
//                      logger::info("    -> Target is Actor: '{}' (FormID: {:X}, Base FormID: {:X})",
//                                   targetActor->GetName(),
//                                   targetActor->GetFormID(),        // The instance ID
//                                   targetActor->GetBaseObject() ? targetActor->GetBaseObject()->GetFormID() : 0); //
//                                   The base NPC_ ID

//                     // --- THIS IS WHERE YOU ADD YOUR ACTOR-SPECIFIC LOGIC ---
//                     // Example: Check if the effect is Stoneflesh
//                     // Make sure to handle potential null pointer from GetBaseObject()
//                     const RE::FormID stonefleshFormID = 0x12F05; // Example FormID for Stoneflesh MGEF (Vanilla
//                     SE/AE) - VERIFY! if (baseEffect && baseEffect->GetFormID() == stonefleshFormID) {
//                         logger::info("    -> Stoneflesh applied to {}", targetActor->GetName());
//                         // Do something specific for Stoneflesh...
//                     }
//                     // --------------------------------------------------------

//                 } else {
//                     // This should technically not happen if MagicTargetIsActor() returned true,
//                     // but good to log just in case (could indicate RTTI issues).
//                     logger::error("    -> MagicTargetIsActor() was true, but dynamic_cast to Actor failed for target
//                     handle!");
//                 }
//             } else {
//                  // It's a MagicTarget but not an Actor (e.g., could be an inanimate object targetted by a spell)
//                  RE::TESObjectREFR* targetRef = nullptr;
//                  auto refHandle = a_thisMagicTarget->GetTargetStatsObject(); // Try to get the underlying
//                  TESObjectREFR if (refHandle) {
//                      targetRef = refHandle;
//                     //  targetRef = refHandle.get().get();
//                  }
//                  if (targetRef) {
//                     logger::info("    -> Target is not an Actor. Target Ref: '{}' (FormID: {:X}, Base FormID: {:X})",
//                                  targetRef->GetName(),
//                                  targetRef->GetFormID(),
//                                  targetRef->GetBaseObject() ? targetRef->GetBaseObject()->GetFormID() : 0);
//                  } else {
//                      logger::info("    -> Target is not an Actor and couldn't resolve to TESObjectREFR (Target
//                      Handle).");
//                  }
//             }

//             // --- Your Custom Code End ---

//             // **VERY IMPORTANT:** Call the original function!
//             // If you don't do this, the effect will never actually be added, breaking game mechanics.
//             _OriginalFunc(a_thisMagicTarget, a_effect);

//             logger::debug("EBuffs: Finished AddEffect Hook for Target Handle, Effect '{}'", effectName);
//         }

//         // Static member to store the pointer to the original function
//         // The type must match the hook function's signature
//         static inline REL::Relocation<decltype(&Hooked)> _OriginalFunc;

//         // Installs the hook
//         static bool Install()
//         {
//             // <<< === STEP 1: Replace with the correct ID for your Skyrim Version (SE or AE) === >>>
//              constexpr REL::ID MagicTarget_AddEffect_ID_AE(51960); // Example for AE 1.6.x
//              constexpr REL::ID MagicTarget_AddEffect_ID_SE(51057); // Example for SE 1.5.97

//             // Select the ID based on the runtime version
//             REL::ID functionID = REL::Module::IsAE() ? MagicTarget_AddEffect_ID_AE : MagicTarget_AddEffect_ID_SE;
//              // <<< ============================================================================== >>>

//              logger::info("EBuffs: Attempting to hook MagicTarget::AddEffect (ID: {})...", functionID.id());

//              // Get the trampoline interface
//              // Using the global trampoline is generally recommended
//             SKSE::Trampoline& trampoline = SKSE::GetTrampoline();

//              // Use write_call<5> to replace a 5-byte call instruction pointing to AddEffect
//              // This is a common hooking method. It patches calls *to* AddEffect.
//              // If AddEffect is called directly via vtable in some places, this specific hook point might miss it,
//              // but it catches many common cases like spell application.
//              // Alternatively, you could hook the function's entry point directly using write_branch<5>,
//              // but write_call is often sufficient and sometimes safer depending on the function.
//              // Let's stick with write_call as it's conceptually simpler for this case.
//              // We need the address from the REL::ID object.
//             // _OriginalFunc = trampoline.write_call<5>(functionID.address(), Hooked);

//             //  if (_OriginalFunc) {
//             //      logger::info("EBuffs: Successfully hooked MagicTarget::AddEffect.");
//             //      return true;
//             //  } else {
//             //      logger::error("EBuffs: Failed to hook MagicTarget::AddEffect!");
//             //      return false;
//             //  }

//              // Call write_call and capture the *result* (the address of the original/trampoline)
//              // The return type is typically std::uintptr_t or similar pointer-like type.
//              SKSE::AllocTrampoline(1 << 10);
//              auto originalFuncAddress = trampoline.write_call<5>(functionID.address(), Hooked);

//              // Check if the write_call succeeded by checking the returned address
//              // A null or zero address indicates failure.
//              if (originalFuncAddress) {
//                  // Success! Store the returned address in our Relocation object for later calls.
//                  // REL::Relocation can be assigned from a raw address.
//                  _OriginalFunc = originalFuncAddress;

//                  logger::info("EBuffs: Successfully hooked MagicTarget::AddEffect at address 0x{:X}",
//                  functionID.address()); return true;
//              } else {
//                  // Failure! The trampoline hook could not be written.
//                  logger::error("EBuffs: Failed to hook MagicTarget::AddEffect (trampoline.write_call failed for
//                  ID{})!", functionID.id()); return false;
//              }
//         }
//     };

//     // Your main Install function for all hooks in this namespace
//     void Install() {
//         logger::info("EBuffs: Installing hooks...");

//         if (MagicTarget_AddEffect_Hook::Install()) {
//             // Hook installed successfully
//         } else {
//             // Handle hook installation failure if needed
//         }

//         // Install other hooks here if you have more...

//         logger::info("EBuffs: Hooks installation process finished.");
//     }

// } // namespace EBuffs::Hooks