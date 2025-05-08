#pragma once
#include "igamesystem.h"

struct JSRuntime;
struct JSContext;

class CScriptingSystem : public CAutoGameSystemPerFrame
{
public:
	CScriptingSystem();

	bool Init() override;
	void Shutdown() override;
	void FrameUpdatePostEntityThink() override;

	void Eval(const char* code);

private:
	JSRuntime* rt_;
	JSContext* ctx_;
};
