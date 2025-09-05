#include "BaseVSShader.h"

#include <d3d9.h>
#include <shaderapi\ishaderapi.h>
#include <reshade\reshade.hpp>
using namespace reshade::api;

class CShaderAPIBase : public IShaderAPI
{
	public:
		// constructor, destructor
		CShaderAPIBase();
		virtual ~CShaderAPIBase();

		// Called when the device is initializing or shutting down
		virtual bool OnDeviceInit() = 0;
		virtual void OnDeviceShutdown() = 0;

		// Pix events
		virtual void BeginPIXEvent(unsigned long color, const char* szName) = 0;
		virtual void EndPIXEvent() = 0;
		virtual void AdvancePIXFrame() = 0;

		// Release, reacquire objects
		virtual void ReleaseShaderObjects() = 0;
		virtual void RestoreShaderObjects() = 0;

		// Resets the render state to its well defined initial value
		virtual void ResetRenderState(bool bFullReset = true) = 0;

		// Returns a d3d texture associated with a texture handle
		virtual void* GetD3DTexture(ShaderAPITextureHandle_t hTexture) = 0;

		// Queues a non-full reset of render state next BeginFrame.
		virtual void QueueResetRenderState() = 0;

		// Methods of IShaderDynamicAPI
	public:
		virtual void GetCurrentColorCorrection(ShaderColorCorrectionInfo_t* pInfo);

	protected:
};

BEGIN_VS_SHADER_FLAGS(ReShadePP, "ReShade Postprocess", SHADER_NOT_EDITABLE)
	effect_runtime* main_runtime = nullptr;
	command_list* main_cmd_list = nullptr;
	resource_view main_backbuffer;

	static void on_init_command_list(command_list* cmd_list)
	{
		main_cmd_list = cmd_list;
	}

	static void on_destroy_command_list(command_list* cmd_list)
	{
		main_cmd_list = nullptr;
	}

	static void on_init_effect_runtime(effect_runtime* runtime)
	{
		auto* device = runtime->get_device();
		auto backbuffer = runtime->get_back_buffer(0);
		auto format = device->get_resource_desc(backbuffer);

		device->create_resource_view(backbuffer,
									 resource_usage::render_target,
									 resource_view_desc(format_to_default_typed(format.texture.format)),
									 &main_backbuffer
		);

		runtime->set_effects_state(false);

		main_runtime = runtime;
	}

	static void on_destroy_effect_runtime(effect_runtime* runtime)
	{
		auto* device = runtime->get_device();
		device->destroy_resource_view(main_backbuffer);

		main_runtime = nullptr;
	}

	void init()
	{
		reshade::register_event<reshade::addon_event::init_command_list>(on_init_command_list);
		reshade::register_event<reshade::addon_event::destroy_command_list>(on_destroy_command_list);
		reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
		reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_destroy_effect_runtime);
	}

	BEGIN_SHADER_PARAMS
		SHADER_PARAM(depthbuffer, SHADER_PARAM_TYPE_TEXTURE, "_rt_DepthBuffer", "DepthBuffer Texture")
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if (params[depthbuffer]->IsDefined())
		{
			LoadTexture(depthbuffer);
		}
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	void BindTextureToReshade(const char* name, int param, IShaderDynamicAPI* pShaderAPI)
	{
		auto cShaderApi = reinterpret_cast<CShaderAPIBase*>(pShaderAPI);
		auto textureHandle = GetShaderAPITextureBindHandle(param, 0);
		auto d3dTexture = cShaderApi->GetD3DTexture(textureHandle);

		if (!d3dTexture)
		{
			return;
		}

		resource_view rtv{ uint64_t(d3dTexture) };
		main_runtime->update_texture_bindings(name, rtv, rtv);
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);
			Draw();
		}

		DYNAMIC_STATE
		{
			BindTextureToReshade("DEPTH", depthbuffer, pShaderAPI);

			main_runtime->set_effects_state(true);
			main_runtime->render_effects(main_cmd_list, main_backbuffer, main_backbuffer);
			main_runtime->set_effects_state(false);
		}
	}
END_SHADER

uint32_t DllMain(HINSTANCE instance, uint32_t reason, void* reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		AssertMsg(reshade::register_addon(instance), "Failed to register reshade addon");
		ReShadePP::init();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		reshade::unregister_addon(instance);
	}

	return true;
}
