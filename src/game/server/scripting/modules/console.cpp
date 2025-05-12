#include "cbase.h"

#include "tier1/utlstring.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Console
{
	static CUtlString ConsoleArgsToCStr(JSContext* ctx, int argc, JSValueConst* argv)
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

		auto result = CUtlString{ s ? s : "<exception>" };
		JS_FreeCString(ctx, s);

		return result;
	}

	static JSValue debug(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		DevLog("%s\n", ConsoleArgsToCStr(ctx, argc, argv).Get());
		return JS_UNDEFINED;
	}

	static JSValue log(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		Msg("%s\n", ConsoleArgsToCStr(ctx, argc, argv).Get());
		return JS_UNDEFINED;
	}

	static JSValue warn(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const static Color yellow{ 255,255,0,255 };
		ConColorMsg(yellow, "%s\n", ConsoleArgsToCStr(ctx, argc, argv).Get());
		return JS_UNDEFINED;
	}

	static JSValue error(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		Warning("%s\n", ConsoleArgsToCStr(ctx, argc, argv).Get());
		return JS_UNDEFINED;
	}

	static JSValue assertError(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc == 0)
			return JS_UNDEFINED;

		if (JS_IsSameValue(ctx, *argv, JS_TRUE))
			return JS_UNDEFINED;

		CUtlString message = "console.assert";
		if (argc > 1)
		{
			message = ConsoleArgsToCStr(ctx, argc - 1, argv + 1);
		}

		auto error = JS_NewError(ctx);
		auto stack = JS_GetPropertyStr(ctx, error, "stack");
		auto stackStr = JS_ToCString(ctx, stack);

		::Warning("Assertion Failed: %s\n%s\n", message.Get(), strstr(stackStr, "\n") + 1);

		// double ref somehow
		JS_FreeCString(ctx, stackStr);
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

			ASSIGN_FUNCTION(console, log, 1);
			ASSIGN_FUNCTION_1(console, info, log, 1);
			ASSIGN_FUNCTION(console, warn, 1);
			ASSIGN_FUNCTION(console, error, 1);
			ASSIGN_FUNCTION(console, debug, 1);
			ASSIGN_FUNCTION_1(console, assert, assertError, 2);

			JS_SetPropertyStr(ctx, global, "console", console);

			JS_FreeValue(ctx, global);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Console);
