// Copyright (c) 2011, Giliam de Carpentier. All rights reserved.
// See http://www.decarpentier.nl/scape-procedural-basics/ for more info
// 
// This file is licensed under the FreeBSD license:
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY GILIAM DE CARPENTIER 'AS IS' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GILIAM DE CARPENTIER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "ProceduralBasics.h"

RasterizerState DisableCulling { CullMode = NONE; };
DepthStencilState DepthEnabling { DepthEnable = TRUE; };
BlendState DisableBlend { BlendEnable[0] = FALSE; };

#define DEFINE_SIMPLE_HLSL_TECHNIQUE(name, psName) \
	technique10 name \
	< \
		string Script = "Pass=p0;"; \
	> \
	{ \
		pass p0 \
		< \
			string Script = "Draw=geometry;"; \
		> \
		{ \
			SetGeometryShader(NULL); \
			SetVertexShader(CompileShader(vs_4_0, vsSimple())); \
			SetPixelShader(CompileShader(ps_4_0, psName())); \
			SetRasterizerState(DisableCulling); \
			SetDepthStencilState(DepthEnabling, 0); \
	    	SetBlendState(DisableBlend, float4(0,0,0,0), 0xFFFFFFFF); \
		} \
	}

DEFINE_SIMPLE_HLSL_TECHNIQUE(tqPerlinNoise, psPerlinNoise);
DEFINE_SIMPLE_HLSL_TECHNIQUE(tqTurbulence, psTurbulence);
DEFINE_SIMPLE_HLSL_TECHNIQUE(tqBillowyTurbulence, psBillowyTurbulence);
DEFINE_SIMPLE_HLSL_TECHNIQUE(tqRidgedTurbulence, psRidgedTurbulence);
DEFINE_SIMPLE_HLSL_TECHNIQUE(tqIqTurbulence, psIqTurbulence);