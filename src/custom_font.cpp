#include "custom_font.h"

void DrawNumber(int32_t value, int32_t max_digits, const CustomFontArgs* args)
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