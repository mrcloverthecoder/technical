#pragma once

#include <list>
#include <vector>
#include "diva.h"

// NOTE: Some information were taken from ReDIVA. Thank you, Koren!
//		 https://github.com/korenkonder/ReDIVA/blob/master/src/ReDIVA/pv_game/pv_game_pv_data.hpp
//
struct PVGamePvData
{
	uint8_t gap0[8];
	uint8_t byte8[4];
	int32_t script_buffer[45000];
	int32_t script_pos;
	int64_t qword2BF30;
	int64_t qword2BF38;
	int64_t cur_time;
	float cur_time_float;
	float prev_time_float;
	uint8_t gap2BF50[32];
	int int2BF70;
	uint8_t gap2BF74[36];
	float float2BF98;
	float float2BF9C;
	int32_t dword2BFA0;
	int32_t dword2BFA4;
	uint64_t script_time;
	uint8_t gap2BFA8[16];
	uint8_t byte2BFC0;
	uint8_t byte2BFC1;
	__declspec(align(8)) uint8_t byte2BFC8;
	int32_t dword2BFCC;
	int32_t dword2BFD0;
	uint8_t byte2BFD4;
	__declspec(align(2)) uint8_t byte2BFD6;
	uint8_t gap2BFD7[961];
	uint8_t byte2C398;
	int32_t dword2C39C;
	uint8_t gap2C3A0[8];
	uint8_t byte2C3A8;
	int32_t dword2C3AC;
	uint8_t gap2C3B0[272];
	uint8_t byte2C4C0;
	int32_t dword2C4C4;
	float float2C4C8;
	float float2C4CC;
	float float2C4D0;
	float float2C4D4;
	float float2C4D8;
	float float2C4DC;
	uint8_t gap2C4E0;
	unsigned __int8 unsigned___int82C4E1;
	int32_t targets_remaining;
	uint8_t gap2C4E8[24];
	size_t target_index;
	float float2C508;
	char char2C50C;
	uint8_t gap2C50D[63];
	float float2C54C;
	uint8_t gap2C550[4];
	unsigned int unsigned_int2C554;
	uint8_t gap2C558[712];
	uint8_t byte2C820;
	int32_t dword2C824;
	int32_t dword2C828;
	float float2C82C;
};

struct TechnicalZone
{
	int64_t time_begin;
	int64_t time_end;
	size_t first_target_index = 0xFFFFFFFF;
	size_t last_target_index = 0xFFFFFFFF;
	size_t targets_max = 0;

	size_t targets_hit = 0;
	bool failed = false;

	inline size_t GetRemaining() const { return targets_max - targets_hit; }
	inline bool IsSuccess() const { return (targets_max == targets_hit) && !failed; }
};

enum AetState : int32_t
{
	AetState_Idle = 0,
	AetState_Start = 2,
	AetState_Loop = 3,
	AetState_FailIn = 4,
	AetState_FailLoop = 5,
	AetState_FailOut = 6,
	AetState_Out = 7,
	AetState_Success = 8,
	AetState_End = 9
};

enum AetMode : int32_t
{
	AetMode_F    = 0,
	AetMode_F2nd = 1,
	AetMode_Max,      // NOTE: Move this down as other UIs are implemented.

	AetMode_X    = 2,
	AetMode_FT   = 3,
	AetMode_MM   = 4,
};

struct WorkData
{
	struct Target
	{
		int64_t spawn_time;
		int64_t hit_time;
		int32_t type;
	};

	struct TargetGroup
	{
		size_t target_count;
		Target targets[4];
	};

	std::vector<TargetGroup> targets;
	std::list<TechnicalZone> tech_zones;
	TechnicalZone* tech_zone = nullptr; // NOTE: Current active technical zone
	size_t tech_zone_index = 0;
	size_t target_index = 0; // NOTE: Index of the target being currently polled by GetHitState
	PVGamePvData* pv_data = nullptr;

	bool files_loaded = false;
	int32_t aet_state = AetState_Idle;
	int32_t bonus_zone_aet = 0;
	int32_t bonus_txt_aet = 0;

	int32_t default_style = AetMode_F;
	int32_t aet_mode = AetMode_F;

	inline void ResetAet()
	{
		diva::aet::Stop(&bonus_zone_aet);
		diva::aet::Stop(&bonus_txt_aet);
		aet_state = AetState_Idle;
	}

	inline void ResetPv()
	{
		ResetAet();
		tech_zone = nullptr;
		tech_zone_index = 0;
		target_index = 0;
		pv_data = nullptr;
	}

	inline void Reset()
	{
		ResetPv();
		targets.clear();
		tech_zones.clear();
		files_loaded = false;
	}
};

inline WorkData work;