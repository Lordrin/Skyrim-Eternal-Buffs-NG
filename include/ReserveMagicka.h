#pragma once

bool IsValidSpellToReserve(RE::SpellItem* spellItem);
float CalculateMagickaForReserveSpell(RE::SpellItem* spellItem);
void ApplyReserveSpellToPlayer(RE::SpellItem* spellItem);
RE::Effect& CreateReserveSpellEffect(RE::SpellItem* spellItem);
