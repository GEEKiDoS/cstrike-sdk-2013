#include "cbase.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Engine
{
	static JSValue ServerCommand(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (!argc)
			return JS_ThrowTypeError(ctx, "serverCommand takes 1 arg");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "serverCommand arg 0 must be string");

		const auto* command = JS_ToCString(ctx, *argv);
		engine->ServerCommand(command);

		return JS_UNDEFINED;
	}

	static JSValue ConVarToJSValue(JSContext* ctx, ConVar* cvar)
	{
		/*auto cvar_obj = JS_NewObjectClass(ctx, );
		auto cvar_prototype = JS_NewObject(ctx);

		JS_SetOpaque()

		JS_SetPropertyStr(ctx, cvar_obj, "__ptr", JS_MKPTR())*/
		
		auto v = g_pCVar->FindVar("test");
	}

	JSValue CVarGetValue(JSContext* ctx, JSValueConst this_val)
	{
		return JS_UNDEFINED;
	}

	JSValue CVarSetValue(JSContext* ctx, JSValueConst this_val, JSValueConst val)
	{
		return JS_UNDEFINED;
	}

	void CVarFinalizer(JSRuntime*, JSValue) {}
	JSClassDef cvar_class_def{ "CVar", CVarFinalizer };
	JSCFunctionListEntry cvar_methods[] = {
		JS_CGETSET_DEF("value", CVarGetValue, CVarSetValue),
	};

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto global = JS_GetGlobalObject(ctx);
			auto engine_object = JS_NewObject(ctx);

			JS_SetPropertyStr(ctx, engine_object, "serverCommand",
							  JS_NewCFunction(ctx, ServerCommand, "serverCommand", 1));

			JS_SetPropertyStr(ctx, global, "engine", engine_object);
			JS_FreeValue(ctx, global);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Engine);
