#include "cbase.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Engine
{
	namespace Cvar
	{
		JSClassID classId;

		JSValue ToJSValue(JSContext* ctx, ConVar* cvar)
		{
			auto cvar_obj = JS_NewObjectClass(ctx, classId);
			JS_SetOpaque(cvar_obj, cvar);

			return cvar_obj;
		}

		JSValue GetValue(JSContext* ctx, JSValueConst this_val)
		{
			auto* cvar = (ConVar*)JS_GetOpaque(this_val, classId);

			if (!cvar)
			{
				return JS_ThrowInternalError(ctx, "cvar is nullptr");
			}

			return JS_NewString(ctx, cvar->GetString());
		}

		JSValue SetValue(JSContext* ctx, JSValueConst this_val, JSValueConst val)
		{
			auto* cvar = (ConVar*)JS_GetOpaque(this_val, classId);

			if (!cvar)
			{
				return JS_ThrowInternalError(ctx, "cvar is nullptr");
			}

			if (val.tag == JS_TAG_BOOL)
			{
				cvar->SetValue(JS_VALUE_GET_BOOL(val));
			}
			else if (val.tag == JS_TAG_INT)
			{
				cvar->SetValue(JS_VALUE_GET_INT(val));
			}
			else if (val.tag == JS_TAG_FLOAT64)
			{
				cvar->SetValue((float) JS_VALUE_GET_FLOAT64(val));
			}
			else if (val.tag == JS_TAG_STRING)
			{
				auto str = JS_ToCString(ctx, val);
				JS_FreeCString(ctx, str);
				cvar->SetValue(str);
			}
			else
			{
				return JS_ThrowInternalError(ctx, "new value must be string, number or bool");
			}

			return JS_NewString(ctx, cvar->GetString());
		}

		void Finalizer(JSRuntime*, JSValue) {}

		JSClassDef classDef{ "CVar", Finalizer };
		JSCFunctionListEntry methods[] = {
			JS_CGETSET_DEF("value", GetValue, SetValue),
		};

		void Init(JSContext* ctx)
		{
			auto* rt = JS_GetRuntime(ctx);

			JS_NewClassID(rt, &Cvar::classId);
			JS_NewClass(rt, Cvar::classId, &Cvar::classDef);

			JSValue proto = JS_NewObject(ctx);
			JS_SetPropertyFunctionList(ctx, proto, methods, _countof(methods));
			JS_SetClassProto(ctx, classId, proto);
		}
	}

	static JSValue serverCommand(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (!argc)
			return JS_ThrowTypeError(ctx, "serverCommand takes 1 arg");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "serverCommand arg 0 must be string");

		const auto* command = JS_ToCString(ctx, *argv);
		engine->ServerCommand(command);
		JS_FreeCString(ctx, command);

		return JS_UNDEFINED;
	}

	static JSValue findCVar(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (!argc)
			return JS_ThrowTypeError(ctx, "findCVar takes 1 arg");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "findCVar arg 0 must be string");

		const auto* name = JS_ToCString(ctx, *argv);
		auto* cvar = g_pCVar->FindVar(name);
		JS_FreeCString(ctx, name);

		if (!cvar)
			return JS_UNDEFINED;

		return Cvar::ToJSValue(ctx, cvar);
	}

	static JSValue getServerTime(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		return JS_NewFloat64(ctx, engine->GetServerTime());
	}

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto global = JS_GetGlobalObject(ctx);
			auto engine_object = JS_NewObject(ctx);

			ASSIGN_FUNCTION(engine_object, serverCommand, 1);
			ASSIGN_FUNCTION(engine_object, findCVar, 1);
			ASSIGN_FUNCTION(engine_object, getServerTime, 0);

			JS_SetPropertyStr(ctx, global, "engine", engine_object);
			JS_FreeValue(ctx, global);

			Cvar::Init(ctx);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Engine);
