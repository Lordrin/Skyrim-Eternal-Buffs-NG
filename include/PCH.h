#pragma once

// This file is required.

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "Config.h"

#include <chrono>

using SpellEffectsMap = std::map<RE::FormID, std::vector<RE::FormID>>;
using namespace std::literals;
namespace logger = SKSE::log;
