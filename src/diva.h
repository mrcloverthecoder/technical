#pragma once

#include <string>
#include <string_view>
#include <map>
#include <stdint.h>
#include "Helpers.h"

namespace prj
{
	FUNCTION_PTR_H(void*, __fastcall, operatorNew, size_t);
	FUNCTION_PTR_H(void*, __fastcall, operatorDelete, void*);

	// FROM: https://github.com/blueskythlikesclouds/DivaModLoader/blob/master/Source/DivaModLoader/Allocator.h
	template <class T>
	class Allocator
	{
	public:
		using value_type = T;

		Allocator() noexcept {}
		template <class U> Allocator(Allocator<U> const&) noexcept {}

		value_type* allocate(size_t n)
		{
			return reinterpret_cast<value_type*>(operatorNew(n * sizeof(value_type)));
		}

		void deallocate(value_type* p, size_t) noexcept
		{
			operatorDelete(reinterpret_cast<void*>(p));
		}
	};

	template <class T, class U>
	bool operator==(Allocator<T> const&, Allocator<U> const&) noexcept
	{
		return true;
	}

	template <class T, class U>
	bool operator!=(Allocator<T> const& x, Allocator<U> const& y) noexcept
	{
		return !(x == y);
	}

	using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
	using string_view = std::basic_string_view<char, std::char_traits<char>>;
}

namespace diva
{
	struct vec2
	{
		float x;
		float y;
	};

	struct vec3
	{
		float x;
		float y;
		float z;
	};

	struct vec4
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct mat4
	{
		vec4 row0;
		vec4 row1;
		vec4 row2;
		vec4 row3;
	};

	struct Rect
	{
		float x;
		float y;
		float width;
		float height;
	};

	struct SprArgs
	{
		uint32_t kind;
		uint32_t id;
		uint8_t color[4];
		int32_t attr;
		int32_t blend;
		int32_t index;
		int32_t layer;
		int32_t priority;
		int32_t resolution_mode_screen;
		int32_t resolution_mode_sprite;
		vec3 center;
		vec3 trans;
		vec3 scale;
		vec3 rot;
		vec2 skew_angle;
		mat4 mat;
		void* texture;
		int32_t shader;
		int32_t field_AC;
		mat4 transform;
		bool field_F0;
		void* vertex_array;
		size_t num_vertex;
		int32_t field_108;
		void* field_110;
		bool set_viewport;
		vec4 viewport;
		uint32_t flags;
		vec2 sprite_size;
		int32_t field_138;
		vec2 texture_pos;
		vec2 texture_size;
		SprArgs* next;
		void* tex;

		SprArgs();
	};

	enum class FontId {
		CMN_10X16 = 0,
		CMN_NUM12X16,
		CMN_NUM14X18,
		CMN_NUM14X20,
		CMN_NUM20X26,
		CMN_NUM20X22,
		CMN_NUM22X22,
		CMN_NUM22X24,
		CMN_NUM24X30,
		CMN_NUM26X24,
		CMN_NUM28X40,
		CMN_NUM28X40_GOLD,
		CMN_NUM34X32,
		CMN_NUM40X52,
		CMN_NUM56X46,
		CMN_ASC12X18,
		FNT_36_DIVA_36_38_24,
		BOLD36_DIVA_36_38_24,
		FNT_36_DIVA_36_38,
		BOLD36_DIVA_36_38,
		FNT_36CN_DIVA_36_38,
		BOLD36CN_DIVA_36_38,
		FNT_36CN2_DIVA_36_38,
		BOLD36CN2_DIVA_36_38,
		FNT_36KR_DIVA_36_38,
		BOLD36KR_DIVA_36_38,
		FNT_36LATIN9_DIVA_36_38_24,
		BOLD36LATIN9_DIVA_36_38_24,
		FNT_36LATIN9_DIVA_36_38,
		BOLD36LATIN9_DIVA_36_38,
	};

	struct FontInfo
	{
		uint32_t font_id;
		void* raw_font;
		int32_t spr_id;
		int32_t unk14;
		float unk18;
		float unk1C;
		float width;
		float height;
		float unk28;
		float unk2C;
		float unk30;
		float unk34;
		float scaled_width;
		float scaled_height;
		float unk40;
		float unk44;
	};

#pragma pack(push, 1)
	struct PrintWork
	{
		uint8_t color[4];
		uint8_t fill_color[4];
		bool clip;
		int8_t gap9[3];
		vec4 clipData;
		int32_t prio;
		int32_t layer;
		int32_t res_mode;
		uint32_t unk28;
		vec2 text_current;
		vec2 line_origin;
		size_t line_length;
		FontInfo* font;
		uint16_t empty_char;
		int8_t gap4E[2];

		inline void SetColor(uint32_t color, uint32_t fill)
		{
			this->color[0] = color & 0xFF;
			this->color[1] = (color >> 8) & 0xFF;
			this->color[2] = (color >> 16) & 0xFF;
			this->color[3] = (color >> 24) & 0xFF;
			fill_color[0] = fill & 0xFF;
			fill_color[1] = (fill >> 8) & 0xFF;
			fill_color[2] = (fill >> 16) & 0xFF;
			fill_color[3] = (fill >> 24) & 0xFF;
		}
	};

	// TODO: Clean-up this struct. I think the struct might be actually smaller than this.
	struct TextArgs
	{
		float unk00;
		PrintWork print_work;
		uint32_t unk60;
		uint32_t unk64;
		uint32_t unk68;
		uint32_t unk6C;
		uint32_t unk70;
		uint32_t unk74;
		uint32_t unk78;
		uint32_t unk7C;

		TextArgs();
	};
#pragma pack(pop)

	struct AetArgs
	{
		uint32_t scene_id;
		char* layer_name;
		prj::string start_marker;
		prj::string end_marker;
		prj::string loop_marker;
		float start_time;
		float end_time;
		int32_t flags;
		int32_t index;
		int32_t layer;
		int32_t prio;
		int32_t res_mode;
		vec3 pos;
		vec3 rot;
		vec3 scale;
		vec3 anchor;
		float frame_speed;
		vec4 color;
		uint8_t gapD0[16];
		prj::string sound_path;
		uint8_t gapE4[16];
		int32_t sound_queue_index;
		uint8_t gap114[16];
		uint8_t gap124[16];
		uint8_t gap134[16];
		void* frame_rate_control;
		bool sound_voice;
		uint8_t gap151[35];

		AetArgs();
		~AetArgs();
	};

	struct AetLayout
	{
		mat4 matrix;
		vec3 position;
		vec3 anchor;
		float width;
		float height;
		float opacity;
		uint32_t unk64;
		int32_t resolutionMode;
		uint32_t unk6C;
		int32_t unk70;
		uint8_t blendMode;
		uint8_t transferFlags;
		uint8_t trackMatte;
		int32_t unk78;
		int32_t unk7C;
	};

	using AetComposition = std::map<prj::string, AetLayout, std::less<prj::string>, prj::Allocator<std::pair<const prj::string, AetLayout>>>;

	namespace aet
	{
		// NOTE: Queues AetSet file to be loaded
		inline FUNCTION_PTR(void, __fastcall, LoadAetSet, 0x1402C9FA0, uint32_t id, prj::string* out);
		// NOTE: Returns true if AetSet file is still loading, false otherwise
		inline FUNCTION_PTR(bool, __fastcall, CheckAetSetLoading, 0x1402CA020, uint32_t id);
		// NOTE: Free AetSet file from memory
		inline FUNCTION_PTR(void, __fastcall, UnloadAetSet, 0x1402CA040, uint32_t id);
		// NOTE: Plays an Aet layer described by the `args` variable passed in.
		inline FUNCTION_PTR(int32_t, __fastcall, Play, 0x1402CA220, AetArgs* args, int32_t id);
		// NOTE: Returns whether an Aet object has ended playback. Will always return false if the loop flag is set.
		inline FUNCTION_PTR(bool, __fastcall, GetEnded, 0x1402CA510, int32_t id);
		// NOTE: Retrieves layout data for specific Aet object ID.
		inline FUNCTION_PTR(void, __fastcall, GetComposition, 0x1402CA670, AetComposition* comp, int32_t id);

		void CreateAetArgs(AetArgs* args, uint32_t scene_id, const char* layer_name, int32_t prio);

		// NOTE: Stops (removes, not pause!) an Aet layer object created by `diva::aet::Play`
		void Stop(int32_t id);
		// NOTE: Same as the normal one, but sets id to 0 afterwards
		void Stop(int32_t* id);
		// NOTE: Stops if GetEnded() returns true and returns the value of GetEnded().
		bool StopOnEnded(int32_t* id);
	}

	namespace spr
	{
		// NOTE: Queues SprSet file to be loaded
		inline FUNCTION_PTR(void, __fastcall, LoadSprSet, 0x1405B4770, uint32_t id, prj::string_view* out);
		// NOTE: Returns true if SprSet file is still loading, false otherwise
		inline FUNCTION_PTR(bool, __fastcall, CheckSprSetLoading, 0x1405B4810, uint32_t id);
		// NOTE: Free SprSet file from memory
		inline FUNCTION_PTR(void, __fastcall, UnloadSprSet, 0x1405B48B0, uint32_t id);

		// NOTE: Draw sprite to screen
		inline FUNCTION_PTR(SprArgs*, __fastcall, DrawSprite, 0x1405B49C0, SprArgs* args);

		// NOTE: Retrieves a font from the font list
		inline FUNCTION_PTR(FontInfo*, __fastcall, GetFont, 0x1402C4DC0, FontInfo* font, FontId id);
		inline FUNCTION_PTR(void, __fastcall, SetFontSize, 0x1402C5240, FontInfo* font, float w, float h);
		// NOTE: Draw text to the screen (UTF-8, char*)
		inline FUNCTION_PTR(void, __fastcall, DrawTextA, 0x1402C57B0, TextArgs* params, uint32_t flags, const char* text);
	}

	namespace dsc
	{
		struct OpcodeInfo
		{
			int32_t id;
			int32_t length_old;
			int32_t length;
			int32_t unk0C;
			const char* name;
		};

		// NOTE: Returns a pointer to a struct that describes DSC opcode <id>
		inline FUNCTION_PTR(const OpcodeInfo*, __fastcall, GetOpcodeInfo, 0x14024DCA0, int32_t id);
	}
}