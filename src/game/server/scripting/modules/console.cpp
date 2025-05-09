#include "cbase.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Console
{
	static const char* ConsoleArgsToCStr(JSContext* ctx, int argc, JSValueConst* argv)
	{
		if (argc == 0)
			return "";

		JSValueConst fmt = *argv;

		const char* s = nullptr;

		if (JS_IsString(fmt))
		{
			auto result = js_std_sprintf(ctx, JS_UNDEFINED, argc, argv);
			s = JS_ToCString(ctx, result);
			JS_FreeValue(ctx, result);
		}
		else
		{
			s = JS_ToCString(ctx, fmt);

			if (!s && JS_IsObject(fmt))
			{
				JS_FreeValue(ctx, JS_GetException(ctx));
				JSValue t = JS_ToObjectString(ctx, fmt);
				s = JS_ToCString(ctx, t);
				JS_FreeValue(ctx, t);
			}
		}

		if (s)
			return s;

		return s ? s : "<exception>";
	}

	static JSValue Log(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		Msg("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue Warn(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		Warning("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue Error(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		::Error("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto global = JS_GetGlobalObject(ctx);
			auto console = JS_NewObject(ctx);

			JS_SetPropertyStr(ctx, console, "log",
							  JS_NewCFunction(ctx, Log, "log", 1));
			JS_SetPropertyStr(ctx, console, "warn",
							  JS_NewCFunction(ctx, Warn, "warn", 1));
			JS_SetPropertyStr(ctx, console, "error",
							  JS_NewCFunction(ctx, Error, "error", 1));

			JS_SetPropertyStr(ctx, global, "console", console);

			JS_FreeValue(ctx, global);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Console);
