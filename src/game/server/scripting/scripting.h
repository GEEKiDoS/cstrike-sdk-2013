#pragma once
#include "igamesystem.h"
#include "tier1/utlvector.h"

struct JSRuntime;
struct JSContext;
struct JSValue;

class IScriptingModule
{
public:
	virtual void Init(JSContext* ctx) const = 0;
};

struct Promise
{
	JSValue promise;
	JSValue resolve;
	JSValue reject;
};

class CScriptingSystem : public CAutoGameSystemPerFrame
{
public:
	CScriptingSystem();

	void LevelInitPreEntity() override;
	void LevelInitPostEntity() override;
	void LevelShutdownPostEntity() override;
	void FrameUpdatePreEntityThink() override;
	void FrameUpdatePostEntityThink() override;
	void Shutdown() override;

	void Eval(const char* code);
	void Exec(const char* file);

	void AddNextFrameResolve(Promise promise);

	template <typename T>
	class Installer final
	{
		static_assert(std::is_base_of<IScriptingModule, T>::value, "Module has invalid base class");

	public:
		Installer(const char* name)
		{
			Register({ name, new T() });
		}
	};

private:
	struct ModuleEntry
	{
		const char* name;
		IScriptingModule* mod;
	};

	static void Register(ModuleEntry scriptmod);

	bool running_;

	JSRuntime* rt_;
	JSContext* ctx_;

	CUtlVector<Promise> pendingNextFramePromise_;

	static CUtlVector<ModuleEntry> modules_;
};

extern CScriptingSystem* g_pScriptingEngine;

#define REGISTER_SCRIPTING_MODULE(name)                          \
namespace														\
{																\
	static CScriptingSystem::Installer<name##::Module> __module {#name}; \
}


#define ASSIGN_FUNCTION_1(parent, name, func, argSize) \
{ \
auto __func = JS_NewCFunction(ctx, func, #name, argSize); \
JS_SetPropertyStr(ctx, parent, #name, __func); \
}

#define ASSIGN_FUNCTION(parent, name, argSize) ASSIGN_FUNCTION_1(parent, name, name, argSize)
