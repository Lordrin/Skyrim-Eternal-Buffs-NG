#pragma once

#include "SpellUtilities.h"

float CalculateMagickaForReserveSpell(RE::SpellItem* spellItem, RE::PlayerCharacter* player);

RE::TESForm& GetReserveSpellForm();
RE::EffectSetting& GetReserveSpellEffectSetting();
RE::SpellItem* CreateReserveSpellCarrier(RE::SpellItem* spellItem);
RE::Effect* CreateReserveSpellEffect(RE::SpellItem* spellItem);
void ApplyReserveSpellToPlayer2(RE::SpellItem* spellItem);
