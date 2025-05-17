#pragma once

bool IsSummonSpell(RE::SpellItem* spellItem);
bool IsLesserPower(RE::SpellItem* spellItem);
bool IsGreaterPower(RE::SpellItem* spellItem);
bool IsShout(RE::SpellItem* spellItem);
bool IsSummon(RE::SpellItem* spellItem);
bool IsScroll(RE::SpellItem* spellItem);
bool IsSpell(RE::SpellItem* spellItem);
bool IsConcentration(RE::SpellItem* spellItem);
// bool IsRecastable(RE::SpellItem* spellItem);
bool IsNonRecastable(RE::SpellItem* spellItem);
bool IsNotCastOnSelf(RE::SpellItem* spellItem);
bool IsTemporaryEffect(RE::ActiveEffect* activeEffect);
std::string GetSpellSourcePluginName(RE::SpellItem* spellItem);


#ifdef _DEBUG
static const char* ToString(RE::MagicSystem::SpellType type)
{
    switch (type)
    {
        case RE::MagicSystem::SpellType::kSpell: return "kSpell";
        case RE::MagicSystem::SpellType::kDisease: return "kDisease";
        case RE::MagicSystem::SpellType::kPower: return "kPower";
        case RE::MagicSystem::SpellType::kLesserPower: return "kLesserPower";
        case RE::MagicSystem::SpellType::kAbility: return "kAbility";
        case RE::MagicSystem::SpellType::kPoison: return "kPoison";
        case RE::MagicSystem::SpellType::kEnchantment: return "kEnchantment";
        // case RE::MagicSystem::SpellType::kPotion: return "kPotion";
        case RE::MagicSystem::SpellType::kAlchemy: return "kAlchemy";
        // case RE::MagicSystem::SpellType::kWortCraft: return "kWortCraft";
        case RE::MagicSystem::SpellType::kIngredient: return "kIngredient";
        case RE::MagicSystem::SpellType::kLeveledSpell: return "kLeveledSpell";
        case RE::MagicSystem::SpellType::kAddiction: return "kAddiction";
        case RE::MagicSystem::SpellType::kVoicePower: return "kVoicePower";
        case RE::MagicSystem::SpellType::kStaffEnchantment: return "kStaffEnchantment";
        case RE::MagicSystem::SpellType::kScroll: return "kScroll";
        default: return "Unknown";
    }
}

static const char* ToString(RE::MagicSystem::CastingType type)
{
    switch (type)
    {
        case RE::MagicSystem::CastingType::kConstantEffect: return "kConstantEffect";
        case RE::MagicSystem::CastingType::kFireAndForget: return "kFireAndForget";
        case RE::MagicSystem::CastingType::kConcentration: return "kConcentration";
        case RE::MagicSystem::CastingType::kScroll: return "kScroll";
        default: return "Unknown";
    }
}

static const char* ToString(RE::MagicSystem::Delivery type)
{
    switch (type)
    {
        case RE::MagicSystem::Delivery::kSelf: return "kSelf";
        case RE::MagicSystem::Delivery::kTouch: return "kTouch";
        case RE::MagicSystem::Delivery::kAimed: return "kAimed";
        case RE::MagicSystem::Delivery::kTargetActor: return "kTargetActor";
        case RE::MagicSystem::Delivery::kTargetLocation: return "kTargetLocation";
        case RE::MagicSystem::Delivery::kTotal: return "kTotal";
        default: return "Unknown";
    }
}

static const char* ToString(RE::EffectSetting::EffectSettingData::Flag flag)
{
    std::string result = "";

    RE::EffectSetting::EffectSettingData::Flag flags[] = {
        RE::EffectSetting::EffectSettingData::Flag::kHostile,
        RE::EffectSetting::EffectSettingData::Flag::kRecover,
        RE::EffectSetting::EffectSettingData::Flag::kDetrimental,
        RE::EffectSetting::EffectSettingData::Flag::kSnapToNavMesh,
        RE::EffectSetting::EffectSettingData::Flag::kNoHitEvent,
        RE::EffectSetting::EffectSettingData::Flag::kDispelWithKeywords,
        RE::EffectSetting::EffectSettingData::Flag::kNoDuration,
        RE::EffectSetting::EffectSettingData::Flag::kNoMagnitude,
        RE::EffectSetting::EffectSettingData::Flag::kNoArea,
        RE::EffectSetting::EffectSettingData::Flag::kFXPersist,
        RE::EffectSetting::EffectSettingData::Flag::kGoryVisuals,
        RE::EffectSetting::EffectSettingData::Flag::kHideInUI,
        RE::EffectSetting::EffectSettingData::Flag::kNoRecast,
        RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsMagnitude,
        RE::EffectSetting::EffectSettingData::Flag::kPowerAffectsDuration,
        RE::EffectSetting::EffectSettingData::Flag::kPainless,
        RE::EffectSetting::EffectSettingData::Flag::kNoHitEffect,
        RE::EffectSetting::EffectSettingData::Flag::kNoDeathDispel
    };

    const char* names[] = {
        "kHostile",
        "kRecover",
        "kDetrimental",
        "kSnapToNavMesh",
        "kNoHitEvent",
        "kDispelWithKeywords",
        "kNoDuration",
        "kNoMagnitude",
        "kNoArea",
        "kFXPersist",
        "kGoryVisuals",
        "kHideInUI",
        "kNoRecast",
        "kPowerAffectsMagnitude",
        "kPowerAffectsDuration",
        "kPainless",
        "kNoHitEffect",
        "kNoDeathDispel"
    };

    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i)
    {
        if ((int)flag & (int)flags[i])
        {
            result += names[i];
            result += "|";
        }
    }

    if (result.empty()) return "kNone";
    if (result.back() == '|') result.pop_back(); // remove trailing '|'

    return result.c_str();
}

static const char* ToString(SKSE::stl::enumeration<RE::SpellItem::SpellFlag, uint32_t> flags)
{
    std::string result = "";

    std::pair<RE::SpellItem::SpellFlag, const char*> flagsArray[] = {
        {RE::SpellItem::SpellFlag::kCostOverride, "kCostOverride"},
        {RE::SpellItem::SpellFlag::kFoodItem, "kFoodItem"},
        {RE::SpellItem::SpellFlag::kExtendDuration, "kExtendDuration"},
        {RE::SpellItem::SpellFlag::kPCStartSpell, "kPCStartSpell"},
        {RE::SpellItem::SpellFlag::kInstantCast, "kInstantCast"},
        {RE::SpellItem::SpellFlag::kIgnoreLOSCheck, "kIgnoreLOSCheck"},
        {RE::SpellItem::SpellFlag::kIgnoreResistance, "kIgnoreResistance"},
        {RE::SpellItem::SpellFlag::kNoAbsorb, "kNoAbsorb"},
        {RE::SpellItem::SpellFlag::kNoDualCastMods, "kNoDualCastMods"}
    };

    for (auto& flag : flagsArray)
    {
        if (flags & flag.first)
        {
            result += flag.second;
            result += "|";
        }
    }

    if (result.empty()) return "kNone";
    if (result.back() == '|') result.pop_back(); // remove trailing '|'

    SKSE::log::debug("current result: {}", result);
    return result.c_str();
}

#endif