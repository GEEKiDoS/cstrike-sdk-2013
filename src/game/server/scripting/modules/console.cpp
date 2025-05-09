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

	static JSValue Debug(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		DevLog("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue Log(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		Msg("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue Warn(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const static Color yellow{ 255,255,0,255 };
		ConColorMsg(yellow, "%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue Error(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		::Warning("%s\n", ConsoleArgsToCStr(ctx, argc, argv));
		return JS_UNDEFINED;
	}

	static JSValue AssertError(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc == 0)
			return JS_UNDEFINED;

		if (JS_IsSameValue(ctx, *argv, JS_TRUE))
			return JS_UNDEFINED;

		const char* message = "console.assert";
		if (argc > 1)
		{
			message = ConsoleArgsToCStr(ctx, argc - 1, argv + 1);
		}

		auto error = JS_NewError(ctx);
		auto stack = JS_GetPropertyStr(ctx, error, "stack");
		auto stackStr = JS_ToCString(ctx, stack);

		::Warning("Assertion Failed: %s\n%s\n", message, stackStr);

		JS_FreeCString(ctx, stackStr);
		JS_FreeValue(ctx, stack);
		JS_FreeValue(ctx, error);

		return JS_UNDEFINED;
	}

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto global = JS_GetGlobalObject(ctx);
			auto console = JS_NewObject(ctx);

			JS_SetPropertyStr(ctx, console, "info",
							  JS_NewCFunction(ctx, Log, "info", 1));
			JS_SetPropertyStr(ctx, console, "log",
							  JS_NewCFunction(ctx, Log, "log", 1));
			JS_SetPropertyStr(ctx, console, "warn",
							  JS_NewCFunction(ctx, Warn, "warn", 1));
			JS_SetPropertyStr(ctx, console, "error",
							  JS_NewCFunction(ctx, Error, "error", 1));
			JS_SetPropertyStr(ctx, console, "debug",
							  JS_NewCFunction(ctx, Debug, "debug", 1));
			JS_SetPropertyStr(ctx, console, "assert",
							  JS_NewCFunction(ctx, AssertError, "assert", 1));

			JS_SetPropertyStr(ctx, global, "console", console);

			JS_FreeValue(ctx, global);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Console);
