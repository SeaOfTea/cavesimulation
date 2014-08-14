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

float Script : STANDARDSGLOBAL 
<
    string UIWidget = "none";
    string ScriptClass = "object";
    string ScriptOrder = "standard";
    string ScriptOutput = "color";
>;

float4x4 worldViewProjection : WorldViewProjection 
<
	string UIWidget="None"; 
>;

float gLacunarity 
<
    string UIWidget = "slider";
    float UIMin = 1.0;
    float UIMax = 3.0;
    float UIStep = 0.01;
    string UIName =  "Lacunarity";
> = 1.92;

float gGain 
<
    string UIWidget = "slider";
    float UIMin = 0.0;
    float UIMax = 1.0;
    float UIStep = 0.01;
    string UIName =  "Gain";
> = 0.618;

float gSeed 
<
    string UIWidget = "slider";
    float UIMin = 0.0;
    float UIMax = 255.0 / 256;
    float UIStep = 1.0 / 256;
    string UIName =  "Seed";
> = 0.0;

float gUVScale
<
    string UIWidget = "slider";
    float UIMin = 0.0;
    float UIMax = 100.0;
    float UIStep = 0.1;
    string UIName =  "UV Scale";
> = 4.0;

texture2D texPerlinPerm2D
<
    string UIWidget = "none";
    string ResourceName = "PerlinPerm2D.png";
    string ResourceType = "2D";
	int MipLevels = 0;
>;

texture2D texPerlinGrad2D
<
    string UIWidget = "none";
    string ResourceName = "PerlinGrad2D.png";
    string ResourceType = "2D";
	int MipLevels = 0;
>;

SamplerState samplerPerlinPerm2D
{
    Texture = <texPerlinPerm2D>;
    Filter = MIN_MAG_MIP_Point;
	//MaxLOD = 0;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samplerPerlinGrad2D
{
    Texture = <texPerlinGrad2D>;
    Filter = MIN_MAG_MIP_Point;
	//MaxLOD = 0;
    AddressU = Wrap;
    AddressV = Wrap;
};
