#pragma once

#include "mod.h"
#include "tech_zone.h"

constexpr int64_t DSC_TIME_SCALE = 100;
constexpr int64_t TIME_SCALE = 10000;

namespace script
{
	void GetTechZoneRegions(PVGamePvData* pv_data, std::vector<TechZone>* tech_zones);
}