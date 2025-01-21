#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <stdio.h>
#include "tech_zone.h"
#include "thirdparty/toml.hpp"

static PVGamePvData* pv_data = nullptr;
static TechZoneManager zone_mgr;

static bool ShouldEnableTechZones()
{
	// NOTE: I'm currently disabling the Technical Zones in practice mode until I figure out
	//       how to handle the practice mode rewinding feature.

	int32_t type = diva::GetPvGameplayInfo()->type;
	return (type != 3 && type != 6) && !diva::IsPracticeMode();
}

//
//
//

HOOK(bool, __fastcall, TaskPvGameInit, 0x1405DA040, uint64_t a1)
{
	if (ShouldEnableTechZones())
		zone_mgr.Init();

	return originalTaskPvGameInit(a1);
}

HOOK(bool, __fastcall, TaskPvGameCtrl, 0x1405DA060, uint64_t a1)
{
	if (ShouldEnableTechZones() && pv_data != nullptr)
		zone_mgr.Ctrl(pv_data);

	return originalTaskPvGameCtrl(a1);
}

HOOK(bool, __fastcall, TaskPvGameDest, 0x1405DA0A0, uint64_t a1)
{
	if (ShouldEnableTechZones())
		zone_mgr.Dest();

	return originalTaskPvGameDest(a1);
}

HOOK(bool, __fastcall, TaskPvGameDisp, 0x1405DA090, uint64_t a1)
{
	if (ShouldEnableTechZones())
		zone_mgr.Disp();

	return originalTaskPvGameDisp(a1);
}

// NOTE: Not sure what class this belongs to, that's why I named it TaskPvSomething
//       It's a pretty deep call from TaskPvInit.
HOOK(bool, __fastcall, TaskPvSomethingInit, 0x15067B560, int64_t a1, int64_t a2, int64_t a3, int64_t a4)
{
	if (ShouldEnableTechZones())
	{
		int8_t* pv = *reinterpret_cast<int8_t**>(a1);
		PVGamePvData* pv_data = reinterpret_cast<PVGamePvData*>(pv + 200);

		// NOTE: Get states before and after the call, to check if they've changed and not
		//       accidentally call the loading function twice
		int32_t state = *reinterpret_cast<int32_t*>(a1 + 8);
		bool ret = originalTaskPvSomethingInit(a1, a2, a3, a4);
		int32_t next_state = *reinterpret_cast<int32_t*>(a1 + 8);

		if (state != next_state)
		{
			// NOTE: The DSC file is read in state 4, however the targets are only populated in state 21.
			//
			if (state == 21)
				zone_mgr.Parse(pv_data);
		}

		return ret;
	}

	return originalTaskPvSomethingInit(a1, a2, a3, a4);
}

HOOK(uint64_t, __fastcall, PVGamePvDataCtrl, 0x14024EB50, PVGamePvData* pv_data, float a2, int64_t a3, char a4)
{
	::pv_data = pv_data;
	return originalPVGamePvDataCtrl(pv_data, a2, a3, a4);
}

static int32_t GetHitStateBasic(int32_t org)
{
	if (org >= HitState_CoolDouble && org <= HitState_SadDouble)
		return org - 9;
	else if (org >= HitState_CoolTriple && org <= HitState_SadTriple)
		return org - 13;
	else if (org >= HitState_CoolQuad && org <= HitState_SadQuad)
		return org - 17;
	return org;
}

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60, void* a1, void* a2, void* a3, void* a4, int32_t a5, void* a6, int32_t* multi_count, void* a8, void* a9, void* a10, bool* slide, bool* slide_chain, bool* slide_chain_start, bool* slide_chain_max, bool* slide_chain_continues, void* a16)
{
	int32_t hit_state = originalGetHitState(a1, a2, a3, a4, a5, a6, multi_count, a8, a9, a10, slide, slide_chain, slide_chain_start, slide_chain_max, slide_chain_continues, a16);

	if (ShouldEnableTechZones() && hit_state != HitState_None)
	{
		if (!*slide_chain || (*slide_chain && *slide_chain_start))
			zone_mgr.CheckNoteHit(GetHitStateBasic(hit_state), nullptr);
	}

	return hit_state;
}

//
//
//

static void LoadSettings()
{
	toml::table tbl = toml::parse_file("config.toml");
	if (toml::node* n = tbl.get("settings"); n != nullptr && n->is_table())
	{
		toml::table* settings = n->as_table();

		if (n = settings->get("style"); n != nullptr && n->is_integer())
		{
			int32_t style = n->value_or<int32_t>(0);
			if (style >= 0 && style < AetStyle_Max)
				zone_mgr.aet_style = style;
		}
	}
}

static void Initialize()
{
	// F
	zone_mgr.AddStyle(
		14029010,
		14029011,
		14028010,
		14028050,
		1973610232
	);

	// F 2nd
	zone_mgr.AddStyle(
		14029000,
		14029001,
		14028000,
		14028050,
		1973610232
	);

	LoadSettings();
}

extern "C"
{
	void __declspec(dllexport) Init()
	{
		Initialize();
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameCtrl);
		INSTALL_HOOK(TaskPvGameDest);
		INSTALL_HOOK(TaskPvGameDisp);
		INSTALL_HOOK(TaskPvSomethingInit);
		INSTALL_HOOK(PVGamePvDataCtrl);
		INSTALL_HOOK(GetHitState);
	}
};