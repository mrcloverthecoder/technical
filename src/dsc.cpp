#include "diva.h"
#include "dsc.h"

static bool CheckModeSelectDifficulty(int32_t bf)
{
	int32_t difficulty_index = diva::GetPvGameplayInfo()->difficulty;
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

void script::GetTechZoneRegions(PVGamePvData* pv_data, std::vector<TechZone>* tech_zones)
{
	int64_t time = 0;
	int64_t tft = 0;
	size_t index = 1;
	int32_t diff = 0;
	int32_t mode = 0;
	TechZone* tech_zone = nullptr;
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
			time = static_cast<int64_t>(pv_data->script_buffer[index + 1]) * TIME_SCALE;
			break;
		case 26: // MODE_SELECT
		case 82: // EDIT_MODE_SELECT
			if (GetModeSelectData(pv_data->script_buffer[index], pv_data, index, &diff, &mode))
			{
				if (mode == 8) // Technical Zone Start
				{
					tech_zone = &tech_zones->emplace_back();
					tech_zone->start_time = time;
				}
				else if (mode == 9) // Technical Zone End
				{
					if (tech_zone != nullptr)
						tech_zone->end_time = time;

					tech_zone = nullptr;
				}
			}

			break;
		}

		// NOTE: Advance read head
		const diva::dsc::OpcodeInfo* info = diva::dsc::GetOpcodeInfo(pv_data->script_buffer[index]);
		index += info->length + 1;
	}
}