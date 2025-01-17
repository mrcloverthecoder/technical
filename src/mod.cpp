#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include "mod.h"

// NOTE: Each index corresponds to an `AetMode` enum value
//
static const uint32_t AetSetIDs[2]  = { 14029010, 14029000 };
static const uint32_t AetGamIDs[2]  = { 14029011, 14029001 };
static const uint32_t SprSetIDs[2]  = { 14028010, 14028000 };
static const uint32_t SprTchznCmnID = 14028050;
static const uint32_t FontSpriteIDs[2] = { 1973610232, 1973610232 };

const int64_t LeniencyWindow = 10000;

struct PvGameplayInfo
{
	int32_t type;
	int32_t difficulty;
};

FUNCTION_PTR(PvGameplayInfo*, __fastcall, GetPvGameplayInfo, 0x14027DD90);
FUNCTION_PTR(bool, __fastcall, IsPracticeMode, 0x1401E7B90);

static bool ShouldEnableTechZones()
{
	// NOTE: I'm currently disabling the Technical Zones in practice mode until I figure out
	//       how to handle the practice mode rewinding feature.

	int32_t type = GetPvGameplayInfo()->type;
	return (type != 3 && type != 6) && !IsPracticeMode();
}

static bool CheckModeSelectDifficulty(int32_t bf)
{
	int32_t difficulty_index = GetPvGameplayInfo()->difficulty;
	if (difficulty_index >= 0 && difficulty_index < 5)
		return (bf & (1 << difficulty_index)) != 0;
	return false;
}

static bool GetModeSelectData(int32_t op, PVGamePvData* pv_data, int32_t offset, int32_t* diff, int32_t* mode)
{
	if (op == 82) // EDIT_MODE_SELECT
	{
		*diff = 31;
		*mode = pv_data->script_buffer[offset + 1];
		return true;
	}

	// MODE_SELECT
	*diff = pv_data->script_buffer[offset + 1];
	*mode = pv_data->script_buffer[offset + 2];
	return CheckModeSelectDifficulty(*diff);
}

enum HitState : int32_t
{
	HitState_Cool = 0,
	HitState_Fine = 1,
	HitState_Safe = 2,
	HitState_Sad  = 3,
	HitState_RedWrong    = 4,
	HitState_GreyWrong   = 5,
	HitState_GreenWrong  = 6,
	HitState_BlueWrong   = 7,
	HitState_Worst       = 8,
	HitState_CoolDouble  = 9,
	HitState_FineDouble  = 10,
	HitState_SafeDouble  = 11,
	HitState_SadDouble   = 12,
	HitState_CoolTriple  = 13,
	HitState_FineTriple  = 14,
	HitState_SafeTriple  = 15,
	HitState_SadTriple   = 16,
	HitState_CoolQuad    = 17,
	HitState_FineQuad    = 18,
	HitState_SafeQuad    = 19,
	HitState_SadQuad     = 20,
	HitState_None        = 21
};

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

HOOK(uint64_t, __fastcall, ExecuteModeSelect, 0x1503B04A0, PVGamePvData* pv_data, int32_t opc)
{
	if (ShouldEnableTechZones())
	{
		int32_t difficulty = 0;
		int32_t mode = 0;

		if (GetModeSelectData(opc, pv_data, pv_data->script_pos, &difficulty, &mode))
		{
			if (mode == 8) // Technical Zone Start
			{
				size_t index = 0;
				for (auto& tech_zone : work.tech_zones)
				{
					if (index == work.tech_zone_index)
					{
						// NOTE: Reset technical zone state
						work.tech_zone = &tech_zone;
						work.tech_zone->targets_hit = 0;
						work.tech_zone->failed = false;

						work.aet_state = AetState_Start;
						work.tech_zone_index++;
						break;
					}

					index++;
				}
			}
			else if (mode == 9) // Technical Zone End
			{
				if (work.tech_zone != nullptr)
				{
					if (work.tech_zone->IsSuccess())
						work.aet_state = AetState_Success;
					else
						work.aet_state = AetState_FailOut;
				}
			}
		}
	}

	return originalExecuteModeSelect(pv_data, opc);
};

HOOK(uint64_t, __fastcall, PVGamePvDataInit, 0x14024E3B0, PVGamePvData* pv_data, uint64_t a2, char a3)
{
	uint64_t ret = originalPVGamePvDataInit(pv_data, a2, a3);

	if (!ShouldEnableTechZones())
		return ret;

	work.Reset();
	int64_t time = 0;
	int32_t tft = 0;
	int32_t index = 1;
	TechnicalZone* tech_zone = nullptr;
	WorkData::TargetGroup* group = nullptr;
	bool end = false;
	while (!end)
	{
		if (index >= 45000)
			break;

		switch (pv_data->script_buffer[index])
		{
		case 0: // END
			end = true;
			break;
		case 1: // TIME
			time = pv_data->script_buffer[index + 1];
			group = nullptr;
			break;
		case 6: // TARGET
			if (group == nullptr)
			{
				work.targets.push_back({});
				group = &work.targets.back();
			}

			if (group->target_count < 4)
			{
				group->targets[group->target_count].spawn_time = time;
				group->targets[group->target_count].hit_time = time + tft;
				group->targets[group->target_count].type = pv_data->script_buffer[index + 1];
				group->target_count++;
			}

			break;
		case 26: // MODE_SELECT
		case 82: // EDIT_MODE_SELECT
		{
			int32_t diff = 0;
			int32_t mode = 0;

			if (GetModeSelectData(pv_data->script_buffer[index], pv_data, index, &diff, &mode))
			{
				if (mode == 8) // Technical Zone Start
				{
					tech_zone = &work.tech_zones.emplace_back();
					tech_zone->time_begin = time;
					tech_zone->first_target_index = 0xFFFFFFFF;
					tech_zone->last_target_index = 0xFFFFFFFF;
				}
				else if (mode == 9) // Technical Zone End
				{
					if (tech_zone != nullptr)
						tech_zone->time_end = time;

					tech_zone = nullptr;
				}
			}

			break;
		}
		case 28: // BAR_TIME_SET
		{
			int32_t bpm = pv_data->script_buffer[index + 1];
			int32_t sig = pv_data->script_buffer[index + 2] + 1;
			tft = static_cast<int32_t>(60.0f / bpm * sig * 100000.0f);
			break;
		}
		case 58: // TARGET_FLYING_TIME
			tft = pv_data->script_buffer[index + 1] * 100;
			break;
		}

		// NOTE: Advance read head
		const diva::dsc::OpcodeInfo* info = diva::dsc::GetOpcodeInfo(pv_data->script_buffer[index]);
		index += info->length + 1;
	}

	// NOTE: Populate technical zones' targets
	for (TechnicalZone& zone : work.tech_zones)
	{
		size_t target_index = 0;
		int64_t last_target_time = 0;
		int32_t last_target_type = -1;
		for (WorkData::TargetGroup& grp : work.targets)
		{
			// NOTE: Check if the current note is not a chainslide "mini" note.
			//       The timing threshold for a target to be considered a mini chainslide
			//       note is 334ms, so this code *may* glitch when using 1/12 slide division
			//       on BPMs lower than 60, but I don't think someone will be charting a song
			//       at such a low BPM, anyway.
			int32_t target_type = grp.targets[0].type;
			bool slide = (target_type == 15 || target_type == 16);
			int64_t time_offset = grp.targets[0].spawn_time - last_target_time;
			bool is_mini_slide = slide && last_target_type == target_type && time_offset <= 33400;

			if (!is_mini_slide)
			{
				int64_t min_time = grp.targets[0].hit_time - LeniencyWindow;
				int64_t max_time = grp.targets[0].hit_time + LeniencyWindow;
				if (min_time >= zone.time_begin && max_time < zone.time_end)
				{
					if (zone.first_target_index == 0xFFFFFFFF)
						zone.first_target_index = target_index;

					zone.last_target_index = target_index;
					zone.targets_max++;
				}

				target_index++;
			}
			
			last_target_time = grp.targets[0].spawn_time;
			last_target_type = grp.targets[0].type;
		}
	}

	return ret;
}

HOOK(uint64_t, __fastcall, PVGamePvDataCtrl, 0x14024EB50, PVGamePvData* pv_data, float a2, int64_t a3, char a4)
{
	if (ShouldEnableTechZones())
	{
		// HACK: Reset Pv data if script read head position is before the first TIME command. This is hacky,
		//       but does the job for resetting on retry.
		if (pv_data->script_pos < 2)
			work.ResetPv();

		work.pv_data = pv_data;
	}

	return originalPVGamePvDataCtrl(pv_data, a2, a3, a4);
}

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60, void* a1, void* a2, void* a3, void* a4, int32_t a5, void* a6, int32_t* multi_count, void* a8, void* a9, void* a10, bool* slide, bool* slide_chain, bool* slide_chain_start, bool* slide_chain_max, bool* slide_chain_continues, void* a16)
{
	int32_t hit_state = originalGetHitState(a1, a2, a3, a4, a5, a6, multi_count, a8, a9, a10, slide, slide_chain, slide_chain_start, slide_chain_max, slide_chain_continues, a16);

	if (!ShouldEnableTechZones())
		return hit_state;
	
	if (hit_state != HitState_None)
	{
		if (work.tech_zone != nullptr)
		{
			TechnicalZone* zone = work.tech_zone;
			if (work.target_index >= zone->first_target_index && work.target_index <= zone->last_target_index)
			{
				if (GetHitStateBasic(hit_state) >= HitState_Safe && GetHitStateBasic(hit_state) <= HitState_Worst)
					work.tech_zone->failed = true;

				if (!work.tech_zone->failed)
				{
					if (!*slide_chain || (*slide_chain && *slide_chain_start))
						work.tech_zone->targets_hit++;
				}
			}
		}

		if (!*slide_chain || (*slide_chain && *slide_chain_start))
			work.target_index++;
	}

	return hit_state;
};

HOOK(bool, __fastcall, TaskPvGameInit, 0x1405DA040, uint64_t a1)
{
	if (ShouldEnableTechZones())
	{
		prj::string_view strv;
		prj::string str;
		diva::spr::LoadSprSet(SprSetIDs[work.aet_mode], &strv);
		diva::spr::LoadSprSet(SprTchznCmnID, &strv);
		diva::aet::LoadAetSet(AetSetIDs[work.aet_mode], &str);
	}

	return originalTaskPvGameInit(a1);
}

HOOK(bool, __fastcall, TaskPvGameCtrl, 0x1405DA060, uint64_t a1)
{
	if (!work.files_loaded && ShouldEnableTechZones())
	{
		work.files_loaded = !diva::spr::CheckSprSetLoading(SprSetIDs[work.aet_mode]) &&
			!diva::spr::CheckSprSetLoading(SprTchznCmnID) &&
			!diva::aet::CheckAetSetLoading(AetSetIDs[work.aet_mode]);
	}

	return originalTaskPvGameCtrl(a1);
}

HOOK(bool, __fastcall, TaskPvGameDest, 0x1405DA0A0, uint64_t a1)
{
	if (ShouldEnableTechZones())
	{
		// NOTE: Call reset to make sure there are no Aet objects lying around before unloading the aet set.
		work.Reset();
		diva::spr::UnloadSprSet(SprSetIDs[work.aet_mode]);
		diva::spr::UnloadSprSet(SprTchznCmnID);
		diva::aet::UnloadAetSet(AetSetIDs[work.aet_mode]);
		work.files_loaded = false;
	}

	return originalTaskPvGameDest(a1);
}

static void CtrlBonusZoneUI(PVGamePvData* pv_data, TechnicalZone* tech_zone)
{
	diva::AetArgs args;
	diva::AetArgs txt_args;

	switch (work.aet_state)
	{
	case AetState_Idle:
		diva::aet::StopOnEnded(&work.bonus_zone_aet);
		diva::aet::StopOnEnded(&work.bonus_txt_aet);
		break;
	case AetState_Start:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "";
			args.end_marker = "loop_start";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			// NOTE: Create text aet object
			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamIDs[work.aet_mode], "bonus_start_txt", 4);
			args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Loop;
		}
		break;
	case AetState_Loop:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "loop_start";
			args.end_marker = "loop_end";
			// NOTE: Even though this is a loop, I don't use the LOOP flag here because I need `GetEnded` to
			//       work so that I can properly transition between animations.
			args.flags = 0x20000;

			work.bonus_zone_aet = diva::aet::Play(&args, 0);
		}

		if (tech_zone->failed)
			work.aet_state = AetState_FailIn;

		break;
	case AetState_FailIn:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "failed_start";
			args.end_marker = "failed_loop_start";
			args.flags = 0x20000;

			work.bonus_zone_aet = diva::aet::Play(&args, 0);
			work.aet_state = AetState_FailLoop;
		}
		break;
	case AetState_FailLoop:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "failed_loop_start";
			args.end_marker = "failed_loop_start";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);
		}
		break;
	case AetState_FailOut:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "failed_loop_end";
			args.end_marker = "failed_end";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamIDs[work.aet_mode], "bonus_end_txt", 4);
			txt_args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Idle;
		}
		break;
	case AetState_Success:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamIDs[work.aet_mode], "bonus_zone", 4);
			args.start_marker = "clear_start";
			args.end_marker = "clear_end";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamIDs[work.aet_mode], "bonus_complete_txt", 4);
			args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Idle;
		}
		break;
	}
}

struct CustomFontArgs
{
	uint32_t sprite_id    = 0;
	diva::vec2 glyph_size = { 0.0f, 0.0f};
	diva::vec2 size = { 0.0f, 0.0f };
	diva::vec3 pos  = { 0.0f, 0.0f, 0.0f };
	int32_t index   = 0;
	int32_t layer   = 0;
	int32_t prio    = 7;
	int32_t res     = 14;
	uint32_t color  = 0xFFFFFFFF;
};

static void DrawNotesNumber(int32_t value, int32_t max_digits, const CustomFontArgs* args)
{
	for (int i = 0; i < max_digits; i++)
	{
		int32_t digit = value - value / 10 * 10;
		value /= 10;

		// NOTE: Discard leading zeroes
		if (digit == 0 && value == 0 && i > 0)
			break;

		diva::SprArgs spr_args;
		spr_args.id = args->sprite_id;
		spr_args.resolution_mode_screen = args->res;
		spr_args.resolution_mode_sprite = args->res;
		spr_args.index = args->index;
		spr_args.layer = args->layer;
		spr_args.priority = args->prio;
		spr_args.attr = 0x100000; // SPR_ATTR_CTR_RT
		memcpy(spr_args.color, &args->color, 4);

		spr_args.trans.x = args->pos.x - args->size.x * i;
		spr_args.trans.y = args->pos.y;
		spr_args.trans.z = 0.0f;

		// NOTE: Calculate glyph texture
		spr_args.flags = 0x03;
		spr_args.texture_pos = { args->glyph_size.x * digit, 0.0f };
		spr_args.texture_size = { args->glyph_size.x, args->glyph_size.y };
		spr_args.sprite_size = { args->size.x, args->size.y };

		diva::spr::DrawSprite(&spr_args);
	}
}

HOOK(bool, __fastcall, TaskPvGameDisp, 0x1405DA090, uint64_t a1)
{
	if (work.pv_data != nullptr && work.tech_zone != nullptr && ShouldEnableTechZones())
	{
		CtrlBonusZoneUI(work.pv_data, work.tech_zone);

		if (work.bonus_zone_aet != 0)
		{
			diva::AetComposition comp;
			diva::aet::GetComposition(&comp, work.bonus_zone_aet);

			if (auto it = comp.find("p_notes_num_rt"); it != comp.end())
			{
				CustomFontArgs args;
				args.sprite_id  = FontSpriteIDs[work.aet_mode];
				args.glyph_size = { 30.0f, 32.0f };
				args.size       = { 45.0f, 48.0f };
				args.pos        = it->second.position;
				args.index      = 0;
				args.layer      = 0;
				args.prio       = 8;
				args.res        = 14;
				args.color      = work.tech_zone->failed ? 0xFF7F7F7F : 0xFFFFFFFF;

				// NOTE: The F-style AET is HDTV720, not HDTV1080.
				if (work.aet_mode == AetMode_F)
				{
					args.pos.x *= 1.5f;
					args.pos.y *= 1.5f;
				}

				int32_t number = work.tech_zone->GetRemaining();
				if (number > 999)
					number = 999;

				DrawNotesNumber(number, 3, &args);
			}
		}
	}

	return originalTaskPvGameDisp(a1);
}

extern "C"
{
	void __declspec(dllexport) Init()
	{
		INSTALL_HOOK(ExecuteModeSelect);
		INSTALL_HOOK(PVGamePvDataInit);
		INSTALL_HOOK(PVGamePvDataCtrl);
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameCtrl);
		INSTALL_HOOK(TaskPvGameDest);
		INSTALL_HOOK(TaskPvGameDisp);
		INSTALL_HOOK(GetHitState);
	}
};