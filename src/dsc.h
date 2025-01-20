#pragma once

#include "mod.h"
#include "tech_zone.h"

constexpr int64_t DSC_TIME_SCALE = 100;
constexpr int64_t TIME_SCALE = 10000;

namespace diva
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
}

void GetPvScriptTargets(PVGamePvData* pv_data, std::vector<diva::TargetGroup>* targets, std::vector<TechZone>* tech_zones);