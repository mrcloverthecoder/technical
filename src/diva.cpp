#include <string.h>
#include "diva.h"
#include "Helpers.h"

namespace prj
{
	FUNCTION_PTR(void*, __fastcall, operatorNew, 0x1409777D0, size_t);
	FUNCTION_PTR(void*, __fastcall, operatorDelete, 0x1409B1E90, void*);
}

namespace diva
{
	FUNCTION_PTR(diva::SprArgs*, __fastcall, DrawRectangle, 0x1405B4D40, Rect* rect, int32_t res, int32_t layer, uint32_t color, int32_t prio);

	FUNCTION_PTR(AetArgs*, __fastcall, CreateAetArgsOrg, 0x14028D560, AetArgs*, uint32_t, const char*, int32_t, int32_t);
	FUNCTION_PTR(void, __fastcall, StopAetOrg, 0x1402CA330, int32_t* id);
	FUNCTION_PTR(SprArgs*, __fastcall, DefaultSprArgs, 0x1405B78D0, SprArgs* args);
	FUNCTION_PTR(TextArgs*, __fastcall, DefaultTextArgs, 0x1402C53D0, TextArgs* args);
	FUNCTION_PTR(AetArgs*, __fastcall, DefaultAetArgs, 0x1401A87F0, AetArgs* args);

	SprArgs::SprArgs()
	{
		memset(this, 0, sizeof(diva::SprArgs));
		DefaultSprArgs(this);
	}
	
	TextArgs::TextArgs()
	{
		memset(this, 0, sizeof(diva::TextArgs));
		DefaultTextArgs(this);
	}

	AetArgs::AetArgs()
	{
		memset(this, 0, sizeof(diva::AetArgs));
		DefaultAetArgs(this);
	}

	// NOTE: Temporary. Just until I sort out the std::map types.
	FUNCTION_PTR(void, __fastcall, DestroyAetArgs, 0x1401A9100, AetArgs* args);
	AetArgs::~AetArgs()
	{
		DestroyAetArgs(this);
	}

	void aet::CreateAetArgs(AetArgs* args, uint32_t scene_id, const char* layer_name, int32_t prio)
	{
		CreateAetArgsOrg(args, scene_id, layer_name, prio, 0);
	}

	void aet::Stop(int32_t id)
	{
		if (id != 0)
			StopAetOrg(&id);
	}

	void aet::Stop(int32_t* id)
	{
		if (id != nullptr)
		{
			if (*id != 0)
				StopAetOrg(id);
			*id = 0;
		}
	}

	bool aet::StopOnEnded(int32_t* id)
	{
		if (id != nullptr && *id != 0)
		{
			bool ended = aet::GetEnded(*id);
			if (ended)
				aet::Stop(id);
			return ended;
		}

		return true;
	}
}