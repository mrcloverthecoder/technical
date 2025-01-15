#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include "mod.h"

enum HitState : int32_t
{
	HitState_Cool = 0,
	HitState_Fine = 1,
	HitState_Safe = 2,
	HitState_Sad  = 3,
	HitState_RedWrong   = 4,
	HitState_GreyWrong  = 5,
	HitState_GreenWrong = 6,
	HitState_BlueWrong  = 7,
	HitState_Worst      = 8,
	HitState_CoolMulti  = 9,
	HitState_FineMulti  = 10,
	HitState_SafeMulti  = 11,
	HitState_None       = 21
};

HOOK(uint64_t, __fastcall, ExecuteModeSelect, 0x1503B04A0, PVGamePvData* pv_data, int32_t opc)
{
	if (opc == 26)
	{
		int32_t difficulty = pv_data->script_buffer[pv_data->script_pos + 1];
		int32_t mode = pv_data->script_buffer[pv_data->script_pos + 2];

		printf("[Technical] pv_data:%p opc:%d (%d %d)\n", pv_data, opc, difficulty, mode);

		if (mode == 8) // Technical Zone Start
		{
			size_t index = 0;
			for (auto& tech_zone : work.tech_zones)
			{
				printf("[Technical]\tmax:%llu index:%llu tzindex:%llu p:%p\n", work.tech_zones.size(), index, work.tech_zone_index, &tech_zone);
				printf("[Technical]\t %llu / %llu / %llu\n", tech_zone.first_target_index, tech_zone.last_target_index, work.target_index);
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

	return originalExecuteModeSelect(pv_data, opc);
};

HOOK(uint64_t, __fastcall, PVGamePvDataInit, 0x14024E3B0, PVGamePvData* pv_data, uint64_t a2, char a3)
{
	uint64_t ret = originalPVGamePvDataInit(pv_data, a2, a3);

	printf("[Technical] Reset!\n");
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
			if (pv_data->script_buffer[index + 2] == 8) // Technical Zone Start
			{
				tech_zone = &work.tech_zones.emplace_back();
				tech_zone->time_begin = time;
				tech_zone->first_target_index = 0xFFFFFFFF;
				tech_zone->last_target_index = 0xFFFFFFFF;
			}
			else if (pv_data->script_buffer[index + 2] == 9) // Technical Zone End
			{
				if (tech_zone != nullptr)
					tech_zone->time_end = time;

				tech_zone = nullptr;
			}

			break;
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
				// TODO: Take in account the timing window? Not sure if it's necessary.
				int64_t hit_time = grp.targets[0].hit_time;
				if (hit_time >= zone.time_begin && hit_time < zone.time_end)
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
	// HACK: Reset Pv data if script read head position is before the first TIME command. This is hacky,
	//       but does the job for resetting on retry.
	if (pv_data->script_pos < 2)
		work.ResetPv();

	work.pv_data = pv_data;
	return originalPVGamePvDataCtrl(pv_data, a2, a3, a4);
}

HOOK(int32_t, __fastcall, GetHitState, 0x14026BF60, void* a1, void* a2, void* a3, void* a4, int32_t a5, void* a6, int32_t* multi_count, void* a8, void* a9, void* a10, bool* slide, bool* slide_chain, bool* slide_chain_start, bool* slide_chain_max, bool* slide_chain_continues, void* a16)
{
	int32_t hit_state = originalGetHitState(a1, a2, a3, a4, a5, a6, multi_count, a8, a9, a10, slide, slide_chain, slide_chain_start, slide_chain_max, slide_chain_continues, a16);
	
	if (hit_state != HitState_None)
	{
		if (work.tech_zone != nullptr)
		{
			TechnicalZone* zone = work.tech_zone;
			if (work.target_index >= zone->first_target_index && work.target_index <= zone->last_target_index)
			{
				if (hit_state >= HitState_Sad && hit_state <= HitState_Worst)
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

FUNCTION_PTR(void*, __fastcall, MMDrawTextA, 0x14027B160, float x, float y, int32_t res, int32_t layer, const char* text, bool x_advance, int32_t color, void* a8);

HOOK(bool, __fastcall, TaskPvGameInit, 0x1405DA040, uint64_t a1)
{
	printf("[Technical] TaskPvGame::Init();\n");
	prj::string_view strv;
	prj::string str;
	diva::spr::LoadSprSet(14028000, &strv);
	diva::aet::LoadAetSet(14029000, &str);

	return originalTaskPvGameInit(a1);
}

HOOK(bool, __fastcall, TaskPvGameCtrl, 0x1405DA060, uint64_t a1)
{
	diva::spr::CheckSprSetLoading(14028000);
	diva::aet::CheckAetSetLoading(14029000);
	return originalTaskPvGameCtrl(a1);
}

HOOK(bool, __fastcall, TaskPvGameDest, 0x1405DA0A0, uint64_t a1)
{
	printf("[Technical] TaskPvGame::Dest();\n");
	// NOTE: Call reset to make sure there are no Aet objects lying around before unloading the aet set.
	work.Reset();
	diva::spr::UnloadSprSet(14028000);
	diva::aet::UnloadAetSet(14029000);
	return originalTaskPvGameDest(a1);
}

const uint32_t AetGamTchznMainID = 140290001;

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
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
			args.start_marker = "";
			args.end_marker = "loop_start";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			// NOTE: Create text aet object
			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamTchznMainID, "bonus_start_txt", 4);
			args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Loop;
		}
		break;
	case AetState_Loop:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
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
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
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
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
			args.start_marker = "failed_loop_start";
			args.end_marker = "failed_loop_start";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);
		}
		break;
	case AetState_FailOut:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
			args.start_marker = "failed_loop_end";
			args.end_marker = "failed_end";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamTchznMainID, "bonus_end_txt", 4);
			txt_args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Idle;
		}
		break;
	case AetState_Success:
		if (diva::aet::StopOnEnded(&work.bonus_zone_aet))
		{
			diva::aet::CreateAetArgs(&args, AetGamTchznMainID, "bonus_zone", 4);
			args.start_marker = "clear_start";
			args.end_marker = "clear_end";
			args.flags = 0x20000;
			work.bonus_zone_aet = diva::aet::Play(&args, 0);

			diva::aet::Stop(&work.bonus_txt_aet);
			diva::aet::CreateAetArgs(&txt_args, AetGamTchznMainID, "bonus_complete_txt", 4);
			args.flags = 0x20000;
			work.bonus_txt_aet = diva::aet::Play(&txt_args, 0);

			work.aet_state = AetState_Idle;
		}
		break;
	}
}

struct CustomFontArgs
{
	uint32_t sprite_id = 0;
	diva::vec2 glyph_size;
	diva::vec2 size;
	diva::vec3 pos;
	int32_t index;
	int32_t layer;
	int32_t prio;
	uint32_t color;
};

static int32_t CountDecimalPlaces(int32_t value)
{
	int32_t num = 0;
	while (value != 0)
	{
		num++;
		value /= 10;
	}

	return num + 1;
}

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
		spr_args.resolution_mode_screen = 14;
		spr_args.resolution_mode_sprite = 14;
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
	if (work.pv_data != nullptr && work.tech_zone != nullptr)
	{
		CtrlBonusZoneUI(work.pv_data, work.tech_zone);

		if (work.bonus_zone_aet != 0)
		{
			diva::AetComposition comp;
			diva::aet::GetComposition(&comp, work.bonus_zone_aet);

			if (auto it = comp.find("p_notes_num_rt"); it != comp.end())
			{
				CustomFontArgs args;
				args.sprite_id  = 2951910002;
				args.glyph_size = { 30.0f, 32.0f };
				args.size       = { 45.0f, 48.0f };
				args.pos        = it->second.position;
				args.index      = 0;
				args.layer      = 0;
				args.prio       = 8;
				args.color      = work.tech_zone->failed ? 0xFF7F7F7F : 0xFFFFFFFF;

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