#pragma once

#include "diva.h"

struct CustomFontArgs
{
	uint32_t sprite_id = 0;
	diva::vec2 glyph_size = { 0.0f, 0.0f };
	diva::vec2 size = { 0.0f, 0.0f };
	diva::vec3 pos = { 0.0f, 0.0f, 0.0f };
	int32_t index = 0;
	int32_t layer = 0;
	int32_t prio = 7;
	int32_t res = 14;
	uint32_t color = 0xFFFFFFFF;
};

void DrawNumber(int32_t value, int32_t max_digits, const CustomFontArgs* args);