#pragma once

// This file is required.
#define ENABLE_SKYRIM_AE
#define SKYRIM_AE
#define OFFSET(se, ae) ae

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "RE/B/BSContainer.h"
#include "RE/P/PerkEntryVisitor.h"

#include "Config.h"

using namespace std::literals;
namespace logger = SKSE::log;

namespace stl
{
	using namespace SKSE::stl;

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}


// #include "Version.h"

// #ifdef SKYRIM_AE
// #else
// #define OFFSET(se, ae) se
// #endif