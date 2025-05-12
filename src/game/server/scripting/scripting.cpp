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

#ifdef DEBUG
extern "C" BOOL __stdcall AllocConsole(void);
#endif

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
		V_snprintf(path, MAX_PATH, "scripts/server/%s", filename);

		if (!g_pFullFileSystem->FileExists(path))
		{
			Warning("script %s not exists\n", filename);
			return nullptr;
		}

		CUtlBuffer buffer;
		if (!g_pFullFileSystem->ReadFile(path, nullptr, buffer))
		{
			Warning("Can not read script %s\n", filename);
			return nullptr;
		}

		uint8_t* result = nullptr;
		*pbuf_len = buffer.TellPut();

		if (ctx)
		{
			result = (uint8_t*)js_malloc(ctx, *pbuf_len);
		}
		else
		{
			result = (uint8_t*)g_pMemAlloc->Alloc(*pbuf_len);
		}

		V_memcpy(result, buffer.Base(), *pbuf_len);

		Msg("Load module %s...\n%s\n", filename, result);
		return result;
	}
}

namespace
{
	void DumpException(JSContext* ctx, JSValueConst ex)
	{
		auto str = JS_ToCString(ctx, ex);
		if (str)
		{
			Warning("%s\n", str);
			JS_FreeCString(ctx, str);
		}
		else
		{
			Warning("[exception]\n");
		}

		JSValue stack;
		if (JS_IsError(ctx, ex))
		{
			stack = JS_GetPropertyStr(ctx, ex, "stack");
		}
		else
		{
			js_std_cmd(/*ErrorBackTrace*/2, ctx, &stack);
		}

		if (!JS_IsUndefined(stack))
		{
			str = JS_ToCString(ctx, stack);
			Warning("%s\n", str);
			JS_FreeCString(ctx, str);
		}
	}

	void JSPromiseRejectionTracker(JSContext* ctx, JSValueConst, JSValueConst reason, bool is_handled, void*)
	{
		if (is_handled)
			return;

		DumpException(ctx, reason);
	}

	static JSValue nextFrame(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		JSValue resolveFunctions[2];
		JSValue promise = JS_NewPromiseCapability(ctx, resolveFunctions);

		g_pScriptingEngine->AddNextFrameResolve({ promise, resolveFunctions[0], resolveFunctions[1] });

		return promise;
	}

#ifdef DEBUG
	void CreateConsole()
	{
		static bool created = false;

		if (!created)
		{
			created = true;

			AllocConsole();

			FILE* f;
			auto _ = freopen_s(&f, "CONOUT$", "w+t", stdout);
			_ = freopen_s(&f, "CONOUT$", "w", stderr);
			_ = freopen_s(&f, "CONIN$", "r", stdin);
		}
	}
#endif
}


CUtlVector<CScriptingSystem::ModuleEntry> CScriptingSystem::modules_;

CScriptingSystem::CScriptingSystem() : CAutoGameSystemPerFrame("JSScriptingEngine")
{
	rt_ = nullptr;
	ctx_ = nullptr;
	running_ = false;
}

void CScriptingSystem::LevelInitPreEntity()
{
#ifdef DEBUG
	CreateConsole();
#endif

	Msg("Initializing Scripting...\n");

	JSMallocFunctions tier0_malloc_functions
	{
		tier0::js_calloc, tier0::js_malloc,
		tier0::js_free, tier0::js_realloc,
	};

	rt_ = JS_NewRuntime2(&tier0_malloc_functions, nullptr);
	Assert(rt_);

#ifdef DEBUG
	JS_SetDumpFlags(rt_, JS_DUMP_LEAKS | JS_DUMP_ATOM_LEAKS);
#endif

	ctx_ = JS_NewContext(rt_);
	Assert(ctx_);

	auto* ctx = ctx_;

	js_std_init_handlers(rt_);
	js_init_module_std(ctx, "std");
	js_init_module_os(ctx, "os");
	js_init_module_bjson(ctx, "bjson");

	for (const auto& entry : modules_)
	{
		Msg("Loading Module %s\n", entry.name);
		entry.mod->Init(ctx);
	}

	JS_SetHostPromiseRejectionTracker(rt_, JSPromiseRejectionTracker, nullptr);
	JS_SetModuleLoaderFunc(rt_, nullptr, js_module_loader, nullptr);

	auto global = JS_GetGlobalObject(ctx);
	ASSIGN_FUNCTION(global, nextFrame, 0);
	JS_FreeValue(ctx_, global);

	running_ = true;
}

void CScriptingSystem::LevelInitPostEntity()
{
	Exec("index.js");
}

void CScriptingSystem::LevelShutdownPostEntity()
{
	running_ = false;

	Msg("Shutting down...");

	// stop all promises
	if (ctx_ && rt_)
	{
		for (const auto& promise : pendingNextFramePromise_)
		{
			auto undefined = JS_UNDEFINED;
			JS_Call(ctx_, promise.reject, promise.promise, 1, &undefined);

			JS_FreeValue(ctx_, promise.resolve);
			JS_FreeValue(ctx_, promise.reject);
			JS_FreeValue(ctx_, undefined);
		}

		pendingNextFramePromise_.Purge();

		JSContext* ctx;
		while(JS_IsJobPending(rt_))
			JS_ExecutePendingJob(rt_, &ctx);
	}

	if (ctx_)
	{
		JS_FreeContext(ctx_);
		ctx_ = nullptr;
	}

	if (rt_)
	{
		JS_FreeRuntime(rt_);
		rt_ = nullptr;
	}
}

void CScriptingSystem::FrameUpdatePreEntityThink()
{
	if (!running_)
		return;

	for (const auto& promise : pendingNextFramePromise_)
	{
		JS_Call(ctx_, promise.resolve, promise.promise, 0, nullptr);

		JS_FreeValue(ctx_, promise.resolve);
		JS_FreeValue(ctx_, promise.reject);
	}

	pendingNextFramePromise_.Purge();

	JSContext* ctx;
	while (JS_IsJobPending(rt_))
		JS_ExecutePendingJob(rt_, &ctx);
}

void CScriptingSystem::FrameUpdatePostEntityThink()
{
	if (!running_)
		return;

}

void CScriptingSystem::Shutdown()
{
	for (const auto& entry : modules_)
	{
		delete entry.mod;
	}

	modules_.Purge();
}

void CScriptingSystem::Register(ModuleEntry module)
{
	modules_.AddToTail(module);
}

void CScriptingSystem::Eval(const char* code)
{
	if (!ctx_)
	{
		Msg("Server is not running.\n");
	}

	auto value = JS_Eval(ctx_, code, V_strlen(code), "<CONSOLE>", JS_EVAL_TYPE_GLOBAL);

	if (!JS_IsUndefined(value))
	{
		auto* s = JS_ToCString(ctx_, value);
		if (!s && JS_IsObject(value))
		{
			JS_FreeValue(ctx_, JS_GetException(ctx_));
			JSValue t = JS_ToObjectString(ctx_, value);
			s = JS_ToCString(ctx_, t);
			JS_FreeValue(ctx_, t);
		}

		if (s)
		{
			Msg("%s\n", s);
			JS_FreeCString(ctx_, s);
		}
		else
		{
			JSValue e = JS_GetException(ctx_);
			DumpException(ctx_, e);
			JS_FreeValue(ctx_, e);
		}
	}

	JS_FreeValue(ctx_, value);
}

void CScriptingSystem::Exec(const char* file)
{
	JS_FreeValue(ctx_, JS_LoadModule(ctx_, file, file));
}

void CScriptingSystem::AddNextFrameResolve(Promise promise)
{
	pendingNextFramePromise_.AddToTail(promise);
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

	void JS_Exec_f(const CCommand& args)
	{
		g_pScriptingEngine->Exec(args.ArgS());
	}

	static ConCommand js_exec("js_exec", JS_Exec_f, "Execute Javascript Module");
}
