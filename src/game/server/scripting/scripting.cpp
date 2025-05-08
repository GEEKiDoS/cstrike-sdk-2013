#include "cbase.h"

#include "filesystem.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"

#undef str_size
#include "qjs/quickjs.h"
#include "qjs/quickjs-libc.h"
#include "scripting.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace tier0
{
	void* js_malloc(void* opaque, size_t size)
	{
		auto ptr = g_pMemAlloc->Alloc(size);
		V_memset(ptr, 0, size);

		return ptr;
	}

	void* js_calloc(void* opaque, size_t count, size_t size)
	{
		return js_malloc(nullptr, count * size);
	}

	void js_free(void* opaque, void* ptr)
	{
		return g_pMemAlloc->Free(ptr);
	}

	void* js_realloc(void* opaque, void* ptr, size_t size)
	{
		return g_pMemAlloc->Realloc(ptr, size);
	}
}

extern "C"
{
	uint8_t* js_load_file(JSContext* ctx, size_t* pbuf_len, const char* filename)
	{
		char path[MAX_PATH];
		V_snprintf(path, MAX_PATH, "scripts/server/%s");

		if (!g_pFullFileSystem->FileExists(path))
		{
			Error("script %s not exists\n", filename);
			return nullptr;
		}

		CUtlBuffer buffer;
		if (!g_pFullFileSystem->ReadFile(path, nullptr, buffer))
		{
			Error("Can not read script %s\n", filename);
			return nullptr;
		}

		uint8_t* result = nullptr;

		if (ctx)
		{
			result = (uint8_t*)js_malloc(ctx, buffer.TellPut() + 1);
		}
		else
		{
			result = (uint8_t*)g_pMemAlloc->Alloc(buffer.TellPut() + 1);
		}

		V_memcpy(result, buffer.Base(), buffer.TellPut());
		result[buffer.TellPut()] = 0;

		return result;
	}
}

CScriptingSystem::CScriptingSystem(): CAutoGameSystemPerFrame("JSScriptingEngine")
{
	rt_ = nullptr;
	ctx_ = nullptr;
}

namespace js_console
{
	JSValue Log(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto result = js_std_sprintf(ctx, this_val, argc, argv);
		auto* s = JS_ToCString(ctx, result);

		if (s)
		{
			Msg("%s\n", s);
		}

		JS_FreeValue(ctx, result);
		return JS_UNDEFINED;
	}

	JSValue Warn(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto result = js_std_sprintf(ctx, this_val, argc, argv);
		auto* s = JS_ToCString(ctx, result);

		if (s)
		{
			Warning("%s\n", s);
		}

		JS_FreeValue(ctx, result);
		return JS_UNDEFINED;
	}

	JSValue Error(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto result = js_std_sprintf(ctx, this_val, argc, argv);
		auto* s = JS_ToCString(ctx, result);

		if (s)
		{
			::Error("%s\n", s);
		}

		JS_FreeValue(ctx, result);
		return JS_UNDEFINED;
	}
}

bool CScriptingSystem::Init()
{
	JSMallocFunctions tier0_malloc_functions
	{
		tier0::js_calloc, tier0::js_malloc,
		tier0::js_free, tier0::js_realloc,
	};

	rt_ = JS_NewRuntime2(&tier0_malloc_functions, nullptr);
	Assert(rt_);

	ctx_ = JS_NewContext(rt_);
	Assert(ctx_);

	js_std_init_handlers(rt_);
	js_init_module_std(ctx_, "std");
	js_init_module_os(ctx_, "os");
	js_init_module_bjson(ctx_, "bjson");

	auto global = JS_GetGlobalObject(ctx_);
	auto console = JS_NewObject(ctx_);

	JS_SetPropertyStr(ctx_, console, "log",
					  JS_NewCFunction(ctx_, js_console::Log, "log", 1));
	JS_SetPropertyStr(ctx_, console, "warn",
					  JS_NewCFunction(ctx_, js_console::Warn, "warn", 1));
	JS_SetPropertyStr(ctx_, console, "error",
					  JS_NewCFunction(ctx_, js_console::Error, "error", 1));

	JS_SetPropertyStr(ctx_, global, "console", console);

	JS_FreeValue(ctx_, global);

	return true;
}

void CScriptingSystem::Shutdown()
{
	JS_FreeContext(ctx_);
	JS_FreeRuntime(rt_);

	ctx_ = nullptr;
	rt_ = nullptr;
}

void CScriptingSystem::FrameUpdatePostEntityThink()
{
	js_std_loop(ctx_);
}

void CScriptingSystem::Eval(const char* code)
{
	auto value = JS_Eval(ctx_, code, V_strlen(code), "<CONSOLE>", JS_EVAL_TYPE_GLOBAL);
	
	if (!JS_IsUndefined(value))
	{
		js_console::Log(ctx_, JS_UNDEFINED, 1, &value);
	}

	JS_FreeValue(ctx_, value);
}

static CScriptingSystem s_ScriptingEngine;
CScriptingSystem* g_pScriptingEngine = &s_ScriptingEngine;

namespace
{
	void JS_Eval_f(const CCommand& args)
	{
		g_pScriptingEngine->Eval(args.ArgS());
	}

	static ConCommand js_eval("js_eval", JS_Eval_f, "Eval Javascript Code");
}
