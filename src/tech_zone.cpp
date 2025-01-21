#include "tech_zone.h"
#include "dsc.h"
#include "custom_font.h"

// NOTE: DIVA's timing window is 130ms long; Here we use the fine window, which is 100ms long.
// 
//       This denotes how many space (time) there should be in each direction
//       of a note inside a technical zone for it to be considered a valid note.
constexpr int64_t TIMING_WINDOW = 100 * DSC_TIME_SCALE * TIME_SCALE;
// NOTE: The timing threshold for a target to be considered a mini chainslide note
//       here is 334ms, so this code *may* glitch when using 1/12 slide division
//       on BPMs lower than 60, but I don't think someone will be charting a song
//       at such a low BPM, anyway.
constexpr int64_t MINI_SLIDE_THRESHOLD = 134 * DSC_TIME_SCALE * TIME_SCALE;

void TechZoneManager::Init()
{
	if (aet_style >= 0 && aet_style < AetStyle_Max)
		style = &styles[aet_style];

	prj::string str;
	prj::string_view strv;
	diva::aet::LoadAetSet(style->aet_set_id, &str);
	diva::spr::LoadSprSet(style->spr_set_id, &strv);
	diva::spr::LoadSprSet(style->cmn_set_id, &strv);

	Reset();
}

void TechZoneManager::Ctrl(PVGamePvData* pv_data)
{
	if (pv_data == nullptr)
		return;

	// NOTE: Check state of files
	if (!files_loaded)
	{
		files_loaded = !diva::aet::CheckAetSetLoading(style->aet_set_id) &&
			!diva::spr::CheckSprSetLoading(style->spr_set_id) &&
			!diva::spr::CheckSprSetLoading(style->cmn_set_id);
		return;
	}

	// HACK: Check if script position is at the beggining of it and
	//       reset the technical zones states.
	if (pv_data->script_pos < 2)
	{
		ResetAet();
		tech_zone = nullptr;
		note_index = 0;

		for (auto& tz : tech_zones)
		{
			tz.hit_count = 0;
			tz.failed = false;
			tz.started = false;
		}
	}

	// NOTE: Store current/next technical zone for quicker use
	for (auto& tz : tech_zones)
	{
		if (pv_data->cur_time >= tz.start_time)
			tech_zone = &tz;
	}

	if (tech_zone == nullptr)
		return;

	if (pv_data->cur_time >= tech_zone->start_time)
	{
		// NOTE: Begin the tech zone's animation cycle
		if (!tech_zone->started && aet_state == AetState_Idle)
		{
			aet_state = AetState_Start;
			tech_zone->started = true;
		}

		// NOTE: Check if user has died in the middle of the tech zone and do the
		//       fail out animation to prevent it from getting stuck at the results screen
		//       in the AetState_FailLoop animation cycle.
		if (pv_data->cur_time <= tech_zone->end_time)
		{
			int32_t life = *(reinterpret_cast<int32_t*>(diva::GetPvGameData()) + 46221);
			// TODO: Fix technical zone immediately failing if your life is 0 when it
			//       begins in no fail mode.
			// 
			// bool no_fail = *(reinterpret_cast<int8_t*>(diva::GetPvGameData()) + 0x2D31D);

			if (life == 0 /*&& !no_fail*/ && aet_state == AetState_FailLoop)
				aet_state = AetState_FailOut;
		}

		// NOTE: Check if the tech zone has ended and finish their animation cycle
		if (pv_data->cur_time >= tech_zone->end_time && tech_zone->started)
		{
			if (tech_zone->failed && aet_state == AetState_FailLoop)
				aet_state = AetState_FailOut;
			else if (tech_zone->IsSuccess() && aet_state == AetState_Loop)
				aet_state = AetState_Success;
		}
	}
}

void TechZoneManager::Dest()
{
	diva::aet::UnloadAetSet(style->aet_set_id);
	diva::spr::UnloadSprSet(style->spr_set_id);
	diva::spr::UnloadSprSet(style->cmn_set_id);
	files_loaded = false;
	Reset();
}

void TechZoneManager::Disp()
{
	diva::AetArgs args;
	diva::AetArgs txt_args;

	switch (aet_state)
	{
	case AetState_Idle:
		break;
	case AetState_Start:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "", "loop_start");
			bonus_zone = diva::aet::Play(&args, 0);

			// NOTE: Create text aet object
			diva::aet::Stop(&bonus_txt);
			diva::aet::CreateAetArgs(&txt_args, style->aet_scene_id, "bonus_start_txt", 0x20000, 0, 4, "", "");
			bonus_txt = diva::aet::Play(&txt_args, 0);

			aet_state = AetState_Loop;
		}
		break;
	case AetState_Loop:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "loop_start", "loop_end");
			bonus_zone = diva::aet::Play(&args, 0);
		}

		if (tech_zone->failed)
			aet_state = AetState_FailIn;

		break;
	case AetState_FailIn:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "failed_start", "failed_loop_start");
			bonus_zone = diva::aet::Play(&args, 0);
			aet_state = AetState_FailLoop;
		}
		break;
	case AetState_FailLoop:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			// HACK: Looping seems to be a bit buggy here, so a quick and dirty way to fix it is
			//       to just make it play a single frame.
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "failed_loop_start", "failed_loop_start");
			bonus_zone = diva::aet::Play(&args, 0);
		}
		break;
	case AetState_FailOut:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "failed_loop_end", "failed_end");
			bonus_zone = diva::aet::Play(&args, 0);

			diva::aet::Stop(&bonus_txt);
			diva::aet::CreateAetArgs(&txt_args, style->aet_scene_id, "bonus_end_txt", 0x20000, 0, 4, "", "");
			bonus_txt = diva::aet::Play(&txt_args, 0);

			aet_state = AetState_End;
		}
		break;
	case AetState_Success:
		if (diva::aet::StopOnEnded(&bonus_zone))
		{
			diva::aet::CreateAetArgs(&args, style->aet_scene_id, "bonus_zone", 0x20000, 0, 4, "clear_start", "clear_end");
			bonus_zone = diva::aet::Play(&args, 0);

			diva::aet::Stop(&bonus_txt);
			diva::aet::CreateAetArgs(&txt_args, style->aet_scene_id, "bonus_complete_txt", 0x20000, 0, 4, "", "");
			bonus_txt = diva::aet::Play(&txt_args, 0);

			aet_state = AetState_End;
		}
		break;
	case AetState_End:
		if (diva::aet::StopOnEnded(&bonus_zone) && diva::aet::StopOnEnded(&bonus_txt))
			aet_state = AetState_Idle;
		break;
	}

	// NOTE: Draw 'remaining notes' number sprites
	if (bonus_zone != 0)
	{
		diva::AetComposition comp;
		diva::aet::GetComposition(&comp, bonus_zone);

		if (auto it = comp.find("p_notes_num_rt"); it != comp.end())
		{
			CustomFontArgs args;
			args.sprite_id = style->font_spr_id;
			args.glyph_size = { 30.0f, 32.0f };
			args.size = { 45.0f, 48.0f };
			args.pos = it->second.position;
			args.index = 0;
			args.layer = 0;
			args.prio = 8;
			args.res = 14;
			args.color = tech_zone->failed ? 0xFF7F7F7F : 0xFFFFFFFF;

			// NOTE: The F-style AET is HDTV720
			if (aet_style == AetStyle_F)
			{
				args.pos.x *= 1.5f;
				args.pos.y *= 1.5f;
			}
			// NOTE: The X-style AET is 1280x728
			else if (aet_style == AetStyle_X)
			{
				args.pos.x *= 1.5f;
				args.pos.y *= 1.48351648f;
			}

			int32_t number = tech_zone->GetRemaining();
			if (number > 999)
				number = 999;

			DrawNumber(number, 3, &args);
		}
	}
}

bool TechZoneManager::CheckNoteHit(int32_t hit_state, TechZone** ptz)
{
	for (auto& tz : tech_zones)
	{
		if (note_index >= tz.first_note_index && note_index <= tz.last_note_index)
		{
			if (hit_state != HitState_Cool && hit_state != HitState_Fine)
				tz.failed = true;

			if (!tz.failed)
				tz.hit_count++;

			if (ptz != nullptr)
				*ptz = &tz;

			note_index++;
			return true;
		}
	}

	note_index++;
	return false;
}

void TechZoneManager::Parse(PVGamePvData* pv_data)
{
	// NOTE: Parse DSC
	script::GetTechZoneRegions(pv_data, &tech_zones);

	// NOTE: Resolve technical zone's notes
	for (TechZone& zone : tech_zones)
	{
		size_t target_index = 0;
		for (PvDscTargetGroup& grp : pv_data->targets)
		{
			if (grp.target_count < 1)
				continue;

			PvDscTarget& tgt = grp.targets[0];
			if (!tgt.slide_chain || (tgt.slide_chain && tgt.slide_chain_start))
			{
				int64_t min_time = grp.hit_time - TIMING_WINDOW;
				int64_t max_time = grp.hit_time + TIMING_WINDOW;
				if (min_time >= zone.start_time && max_time <= zone.end_time)
				{
					if (zone.first_note_index == -1)
						zone.first_note_index = target_index;

					zone.last_note_index = target_index;
					zone.note_count++;
				}

				target_index++;
			}
		}
	}
}