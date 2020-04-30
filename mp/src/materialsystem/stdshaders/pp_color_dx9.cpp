//========= Copyright � 1996-2007, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "basevsshader.h"

#include "SDK_screenspaceeffect_vs30.inc"
#include "pp_color_hdr_ps30.inc"

BEGIN_VS_SHADER_FLAGS(pp_color_hdr, "hdr post-processing effects", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Full framebuffer texture")
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
}

SHADER_FALLBACK
{
	// This shader should not be *used* unless we're >= DX9  (bloomadd.vmt/screenspace_general_dx8 should be used for DX8)
	return 0;
}

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableBlending(false);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, false);

		int		format = VERTEX_POSITION;
		int		numTexCoords = 1;
		int *	pTexCoordDimensions = NULL;
		int		userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat(format, numTexCoords, pTexCoordDimensions, userDataSize);

		DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs30);
		SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs30);

		DECLARE_STATIC_PIXEL_SHADER(pp_color_hdr_ps30);
		SET_STATIC_PIXEL_SHADER(pp_color_hdr_ps30);
		
	}
	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);

		DECLARE_DYNAMIC_PIXEL_SHADER(pp_color_hdr_ps30);
		SET_DYNAMIC_PIXEL_SHADER(pp_color_hdr_ps30);

		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs30);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs30);
	}
	Draw();
}
END_SHADER
