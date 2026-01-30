#pragma once

#include "SpellUtilities.h"

float CalculateMagickaForReserveSpell(RE::SpellItem* spellItem);

RE::TESForm& GetReserveSpellForm();
RE::EffectSetting& GetReserveSpellEffectSetting();
RE::Effect* CreateReserveSpellEffect(RE::SpellItem* spellItem);
void ApplyReserveSpellToPlayer(RE::SpellItem* spellItem);
