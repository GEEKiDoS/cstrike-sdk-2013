#include "cbase.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Entity
{

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto global = JS_GetGlobalObject(ctx);
			auto entities = JS_NewObject(ctx);

			JS_SetPropertyStr(ctx, global, "entities", entities);
			JS_FreeValue(ctx, global);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Entity);
