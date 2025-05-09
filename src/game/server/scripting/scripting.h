#pragma once
#include "igamesystem.h"
#include "tier1/utlvector.h"

struct JSRuntime;
struct JSContext;

class IScriptingModule
{
public:
	virtual void Init(JSContext* ctx) const = 0;
};

class CScriptingSystem : public CAutoGameSystemPerFrame
{
public:
	CScriptingSystem();

	void LevelInitPreEntity() override;
	void LevelShutdownPostEntity() override;
	void FrameUpdatePreEntityThink() override;
	void Shutdown() override;
	void Eval(const char* code);

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

	JSRuntime* rt_;
	JSContext* ctx_;

	static CUtlVector<ModuleEntry> modules_;
};

extern CScriptingSystem* g_pScriptingEngine;

#define REGISTER_SCRIPTING_MODULE(name)                          \
namespace														\
{																\
	static CScriptingSystem::Installer<name##::Module> __module {#name}; \
}
