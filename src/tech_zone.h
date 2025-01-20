#pragma once

#include <vector>
#include <stdint.h>
#include "diva.h"
#include "mod.h"

enum AetState : int32_t
{
	AetState_Idle     = 0,
	AetState_Start    = 1,
	AetState_Loop     = 2,
	AetState_FailIn   = 3,
	AetState_FailLoop = 4,
	AetState_FailOut  = 5,
	AetState_Success  = 6,
	AetState_End      = 7
};

enum AetStyle : int32_t
{
	AetStyle_F    = 0,
	AetStyle_F2nd = 1,
	AetStyle_X    = 2,
	AetStyle_FT   = 3,
	AetStyle_M39  = 4,
	AetStyle_Max  = 5
};

struct TechStyle
{
	uint32_t aet_set_id = 0;
	uint32_t aet_scene_id = 0;
	uint32_t spr_set_id = 0;
	uint32_t cmn_set_id = 0;
	uint32_t font_spr_id = 0;
};

struct TechZone
{
	int64_t start_time = 0;
	int64_t end_time = 0;
	int64_t first_note_index = -1;
	int64_t last_note_index = -1;
	size_t note_count = 0;
	size_t hit_count = 0;
	bool failed = false;
	bool started = false;

	inline size_t GetRemaining() const { return note_count - hit_count; }
	inline bool IsSuccess() const { return note_count == hit_count; }
};

struct TechZoneManager
{
	bool files_loaded = false;
	int32_t aet_state = AetState_Idle;
	int32_t aet_style = AetStyle_F2nd;
	int32_t bonus_zone = 0;
	int32_t bonus_txt = 0;

	std::vector<TechZone> tech_zones;
	TechZone* tech_zone = nullptr;
	int64_t note_index = 0;

	std::vector<TechStyle> styles;
	TechStyle* style = nullptr;

	inline void ResetAet()
	{
		diva::aet::Stop(&bonus_zone);
		diva::aet::Stop(&bonus_txt);
		aet_state = AetState_Idle;
	}

	inline void Reset()
	{
		ResetAet();
		tech_zones.clear();
		tech_zone = nullptr;
		note_index = 0;
	}

	inline void AddStyle(uint32_t aet_set, uint32_t scene, uint32_t spr_set, uint32_t cmn_set, uint32_t font)
	{
		auto& style = styles.emplace_back();
		style.aet_set_id = aet_set;
		style.aet_scene_id = scene;
		style.spr_set_id = spr_set;
		style.cmn_set_id = cmn_set;
		style.font_spr_id = font;
	}

	void Init();
	void Ctrl(PVGamePvData* pv_data);
	void Dest();
	void Disp();

	bool CheckNoteHit(int32_t hit_state, TechZone** ptz);
	void Parse(PVGamePvData* pv_data);
};