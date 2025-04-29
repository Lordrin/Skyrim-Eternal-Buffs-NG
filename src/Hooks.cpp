#include "Hooks.h"

namespace EBuffs::Hooks {
    struct OnApply {
        struct detail {
            static void on_add(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect) {
                logger::info("EBuffs: OnApply: MagicTarget::AddEffect called.");

                if (!a_effect) {
                    logger::info("EBuffs: OnApply: Effect is null.");
                    return;
                }
                if (!a_target) {
                    logger::info("EBuffs: OnApply: Target is null.");
                    return;
                }

                if (a_target->MagicTargetIsActor()) {
                    // Attempt the dynamic cast
                    RE::Actor* actor = dynamic_cast<RE::Actor*>(a_target);

                    // ALWAYS check if the cast was successful
                    if (actor) {
                        // Successfully converted! Now you can use the 'actor' pointer.
                        logger::info("EBuffs: OnApply: Successfully cast MagicTarget to Actor. FormID: {:X}, Name: {}",
                                     actor->GetFormID(), actor->GetName());
                        // Do something with the actor...
                        // actor->DoSomething();
                    } else {
                        // This case is unlikely if MagicTargetIsActor() was true,
                        // but good practice for dynamic_cast. Could indicate RTTI issues or hierarchy problems.
                        logger::info(
                            "EBuffs: OnApply: MagicTargetIsActor() was true, but dynamic_cast failed.");  // GetTargetReference()
                                                                                                          // often
                                                                                                          // returns the
                                                                                                          // REFR
                    }
                } else {
                    logger::info("EBuffs: OnApply: Target is not an Actor.");
                }
            }
            // static void on_add(RE::MagicTarget* a_target) {
            //     if (!a_target) {
            //         logger::info("EBuffs: OnApply: Target is null.");
            //         return;
            //     }

            //     if (a_target->MagicTargetIsActor()) {
            //         // Attempt the dynamic cast
            //         RE::Actor* actor = dynamic_cast<RE::Actor*>(a_target);

            //         // ALWAYS check if the cast was successful
            //         if (actor) {
            //             // Successfully converted! Now you can use the 'actor' pointer.
            //             logger::info("EBuffs: OnApply: Successfully cast MagicTarget to Actor. FormID: {:X}, Name:
            //             {}",
            //                          actor->GetFormID(), actor->GetName());
            //             // Do something with the actor...
            //             // actor->DoSomething();
            //         } else {
            //             // This case is unlikely if MagicTargetIsActor() was true,
            //             // but good practice for dynamic_cast. Could indicate RTTI issues or hierarchy problems.
            //             logger::info(
            //                 "EBuffs: OnApply: MagicTargetIsActor() was true, but dynamic_cast failed.");  //
            //                 GetTargetReference()
            //                                                                                               // often
            //                                                                                               // returns
            //                                                                                               the
            //                                                                                               // REFR
            //         }
            //     } else {
            //         logger::info("EBuffs: OnApply: Target is not an Actor.");
            //     }
            // }
        };
        static void thunk(RE::MagicTarget* a_target, RE::ActiveEffect* a_effect) { detail::on_add(a_target, a_effect); }

        static inline REL::Relocation<decltype(thunk)> func;
        // static inline REL::Relocation<decltype(thunk)> orig;
        static constexpr std::size_t idx{0x08};
        // static inline constexpr std::size_t size = 0x08;
    };

    struct MagicCasterSpellCast_Hook {
        // virtual void                       SpellCast(bool a_doCast, std::uint32_t a_arg2, MagicItem* a_spell);    // 09
        struct detail {
            static void on_cast(RE::MagicCaster* a_caster, RE::MagicItem* a_magicItem) {
                logger::info("EBuffs: MagicCasterSpellCast hook triggered.");
            }
        };

        static void thunk(RE::MagicCaster* a_this, bool a_doCast, std::uint32_t a_arg2, RE::MagicItem* a_magicItem) { detail::on_cast(a_this, a_magicItem); }

        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t idx{0x09};
    };

    void Install() {
        logger::info("EBuffs: Installing hooks...");
        auto x = RE::MagicTarget::VTABLE;
        // auto y = RE::ActorMagicCaster::VTABLE;
        stl::write_vfunc<RE::MagicTarget, OnApply>();
        // stl::write_vfunc<RE::MagicTarget, MagicCasterSpellCast_Hook>();
        logger::info("EBuffs: Hooks installed.");  // Check if this line appears
    }
}

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