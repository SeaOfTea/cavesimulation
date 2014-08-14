#define MAX_METABALLS   400   // should match the .cpp file
#define IOR             2.5

#define PI 3.1415

Buffer<float4> SampleDataBuffer;

struct SampleData
{
    float4 Pos : SV_Position;
    float4 Field : TEXCOORD0;   // Gradient in .xyz, value in .w
	float3 tex1 : TEXCOORD1;
};

struct SurfaceVertex
{
    float4 Pos : SV_Position;
    float3 N : NORMAL;
	float3 tex : TEXCOORD1;
};

cbuffer constants
{
    float R0Constant = ((1.0 - (1.0/IOR)) * (1.0 - (1.0/IOR))) / ((1.0 + (1.0/IOR)) * (1.0 + (1.0/IOR)));
    float R0Inv = 1.0 - ((1.0 - (1.0/IOR)) * (1.0 - (1.0/IOR)))/((1.0 + (1.0/IOR)) * (1.0 + (1.0/IOR)));
};


cbuffer cb0 : register(b0)
{
    row_major float4x4 ProjInv;
    row_major float3x3 ViewIT;
    row_major float4x4 WorldViewProj;
    row_major float4x4 World;

    uint NumMetaballs;
    float4 Metaballs[MAX_METABALLS];    // .xyz -> metaball center, .w -> metaball squared radius

    float3 ViewportOrg;
    float3 ViewportSizeInv;

    float3 LightPos1;        // view-space light position 1
    float3 LightPos2;        // view-space light position 2
    float3 LightPos3;        // view-space light position 3

	float3 SpotLightPos1;
	float3 SpotLightDirection1;

	float3 EyeVec;
    

};

Texture2D rockTexture;
Texture2D rockBump;
Texture2D dirtTexture;
Texture2D noiseTexture;

SamplerState RockSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};



float4 GetWorldSpacePos( float4 WindowPos )
{
    float4 ClipPos;
    ClipPos.x = (2 * ((WindowPos.x - ViewportOrg.x) * ViewportSizeInv.x) - 1);
    ClipPos.y = (-2 * ((WindowPos.y - ViewportOrg.y) * ViewportSizeInv.y) + 1);
    ClipPos.z = ((WindowPos.z - ViewportOrg.z) * ViewportSizeInv.z);
    ClipPos.w = 1;

    float4 Pos;
    Pos = mul(ClipPos, ProjInv);    // backtransform clipspace position to get viewspace position
    Pos.xyz /= Pos.w;               // re-normalize
    

    return Pos;
}


// Metaball function
// Returns metaball function value in .w and its gradient in .xyz
float4 Metaball(float3 Pos, float3 Center, float RadiusSq)
{
    float4 o;

    float3 d = Pos - Center;
    float DistSq = dot(d, d);
    float InvDistSq = 1 / DistSq;

    o.xyz = -2 * RadiusSq * InvDistSq * InvDistSq * d;
    o.w = RadiusSq * InvDistSq;

    return o;
}


SamplerState TriLinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};


// Vertex shader calculates field contributions at each grid vertex
SampleData SampleFieldVS(float3 Pos : POSITION)
{
    SampleData o;
    
    float3 WorldPos = mul(float4(Pos, 1), World).xyz;

    // Sum up contributions from all metaballs

    o.Field = 0;
    o.Pos = mul(float4(Pos.xyz, 1), WorldViewProj);
	o.tex1 = mul(float4(Pos.xyz, 0), World).xyz;

    for (uint i = 0; i<NumMetaballs; i++)
    {
		//o.Field += WorldPos.y;
        o.Field += Metaball(WorldPos, Metaballs[i].xyz, (Metaballs[i].w));
		
		
    }

	
    // Transform position and normals

	


    //o.Field.xyz = -normalize(mul(o.Field.xyz, ViewIT));  // we want normals in view space


    // Generate in-out flags

    return o;
}

//float NoisePS(SurfaceVertex IN, float density) : SV_Target
//{
//	float3 WorldPos = mul(float4(IN.Pos.xyz, 1), World).xyz;
//
//	float ret = NoiseTexture.Sample(NoiseSampler, WorldPos).x;
//
//	return ret;
//}


SampleData PassThroughVS(SampleData IN)
{
	SampleData OUT;

	OUT = IN;
    return OUT;
}

// Estimate where isosurface intersects grid edge with endpoints v0, v1
SurfaceVertex CalcIntersection(SampleData v0, SampleData v1)
{
    SurfaceVertex o;

	// We're taking special care to generate bit-exact results regardless of traversal (v0,v1) or (v1, v0)

	float t = (2.0 - (v0.Field.w + v1.Field.w)) / (v1.Field.w - v0.Field.w);
    
	o.Pos = 0.5 * (t * (v1.Pos - v0.Pos) + (v1.Pos + v0.Pos));
	o.N = 0.5 * (t * (v1.Field.xyz - v0.Field.xyz) + (v1.Field.xyz + v0.Field.xyz));	
	o.tex = 0.5 * (t * (v1.tex1 - v0.tex1) + (v1.tex1 + v0.tex1));

    return o;
}

// This struct stores vertex indices of up to 4 edges from the input tetrahedron. The GS code below 
// uses these indices to index into the input vertex set for interpolation along those edges. 
// It basically encodes topology for the output triangle strip (of up to 2 triangles).
struct TetrahedronIndices 
{ 
	uint4 e0; 
	uint4 e1; 
};

// putting edge tables in a tbuffer seems to give better perf
tbuffer EdgeTables
{
    const TetrahedronIndices EdgeTableGS[16] =
    {
        { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
        { 3, 0, 3, 1 }, { 3, 2, 0, 0 },
        { 2, 1, 2, 0 }, { 2, 3, 0, 0 },
        { 2, 0, 3, 0 }, { 2, 1, 3, 1 },
        { 1, 2, 1, 3 }, { 1, 0, 0, 0 },
        { 1, 0, 1, 2 }, { 3, 0, 3, 2 },
        { 1, 0, 2, 0 }, { 1, 3, 2, 3 },
        { 3, 0, 1, 0 }, { 2, 0, 0, 0 },
        { 0, 2, 0, 1 }, { 0, 3, 0, 0 },
        { 0, 1, 3, 1 }, { 0, 2, 3, 2 },
        { 0, 1, 0, 3 }, { 2, 1, 2, 3 },
        { 3, 1, 2, 1 }, { 0, 1, 0, 0 },
        { 0, 2, 1, 2 }, { 0, 3, 1, 3 },
        { 1, 2, 3, 2 }, { 0, 2, 0, 0 },
        { 0, 3, 2, 3 }, { 1, 3, 0, 0 },
        { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
    };

    const uint2 EdgeTableVS[16][4] =
    {
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
        { { 3, 0 }, { 3, 1 }, { 3, 2 }, { 2, 2 } },
        { { 2, 1 }, { 2, 0 }, { 2, 3 }, { 3, 3 } },
        { { 2, 0 }, { 3, 0 }, { 2, 1 }, { 3, 1 } },
        { { 1, 2 }, { 1, 3 }, { 1, 0 }, { 0, 0 } },
        { { 1, 0 }, { 1, 2 }, { 3, 0 }, { 3, 2 } },
        { { 1, 0 }, { 2, 0 }, { 1, 3 }, { 2, 3 } },
        { { 3, 0 }, { 1, 0 }, { 2, 0 }, { 0, 0 } },
        { { 0, 2 }, { 0, 1 }, { 0, 3 }, { 3, 3 } },
        { { 0, 1 }, { 3, 1 }, { 0, 2 }, { 3, 2 } },
        { { 0, 1 }, { 0, 3 }, { 2, 1 }, { 2, 3 } },
        { { 3, 1 }, { 2, 1 }, { 0, 1 }, { 1, 1 } },
        { { 0, 2 }, { 1, 2 }, { 0, 3 }, { 1, 3 } },
        { { 1, 2 }, { 3, 2 }, { 0, 2 }, { 2, 2 } },
        { { 0, 3 }, { 2, 3 }, { 1, 3 }, { 3, 3 } },
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } }
    };
}

[MaxVertexCount(4)]
void TessellateTetrahedraGS(lineadj SampleData In[4], inout TriangleStream<SurfaceVertex> Stream)
{
    // construct index for this tetrahedron
    uint index = (uint(In[0].Field.w > 1) << 3) | (uint(In[1].Field.w > 1) << 2) | (uint(In[2].Field.w > 1) << 1) | uint(In[3].Field.w > 1);


    // don't bother if all vertices out or all vertices in
    if (index > 0 && index < 15)
    {
        uint4 e0 = EdgeTableGS[index].e0;
        uint4 e1 = EdgeTableGS[index].e1;
        
        // Emit a triangle
        Stream.Append( CalcIntersection(In[e0.x], In[e0.y]) );
        Stream.Append( CalcIntersection(In[e0.z], In[e0.w]) );
        Stream.Append( CalcIntersection(In[e1.x], In[e1.y]) );

        // Emit additional triangle, if necessary
        if (e1.z != 0) {
            Stream.Append( CalcIntersection(In[e1.z], In[e1.w]) );
        }
        
    }
}

TextureCube EnvMap;

float FresnelApprox(float3 I, float3 N)
{
    return R0Constant + R0Inv * pow(1.0 - dot(I, N), 5.0);
}

float4 ShadeSurfacePS( SurfaceVertex IN ) : SV_Target
{
    float4 Pos = GetWorldSpacePos( IN.Pos );

	float3 N = normalize(IN.N);
	//N= -1/N;
    float3 L1 = normalize(LightPos1 - Pos.xyz);
    float3 L2 = normalize(LightPos2 - Pos.xyz);
    float3 L3 = normalize(LightPos3 - Pos.xyz);
    float3 I = normalize(Pos.xyz);

    float3 R = reflect(I, N);

    float4 Reflected = EnvMap.Sample( TriLinearSampler, mul(ViewIT, R ) );

    float NdotL1 = max(0, dot(N, L1));
    float NdotL2 = max(0, dot(N, L2));
    float NdotL3 = max(0, dot(N, L3));

    float3 Color = NdotL1 * float3(1, 1, 1) + pow(max(dot(R, L1), 0), 32)
                    + NdotL2 * float3(0.65, 0.6, 0.45) + pow(max(dot(R, L2), 0), 32)
                    + NdotL3 * float3(0.7, 0.7, 0.8) + pow(max(dot(R, L3), 0), 32);

    return lerp(EnvMap.Sample( TriLinearSampler, mul(ViewIT, R) ), float4(Color, 1), FresnelApprox(I, N) * 0.05 );

}

float4 SimplePS( SurfaceVertex IN, uniform float4 color ) : SV_Target
{
    return color;
}

float4 DiffusePS( SurfaceVertex IN ) : SV_Target
{
		
	float4 AmbientColor = float4(0.2, 0.2, 0.2, 1);
	float AmbientIntensity = 0.5;

	float Shininess = 100;
	float4 SpecularColor = float4(1, 1, 1, 1);    
	float SpecularIntensity = 0.5;
		
	float4 Kd = 0.5;
	float4 diffuseLight = 0.5;

	float3 LightN = normalize(mul(IN.N, ViewIT)); 

	float4 Pos = GetWorldSpacePos( IN.Pos );
	float3 texN = normalize(IN.N);


	//float4 derp = rockTexture.Sample(RockSampler, IN.tex.xy*2.5f);
	

	float3 blend_weights = abs(texN); // might need normalizing

	//Tighten up blend zone:
	blend_weights = (blend_weights - 0.2) * 7;
	blend_weights = max(blend_weights, 0);

	//Force weights to sum to 1.0
	blend_weights /= (blend_weights.x +blend_weights.y + blend_weights.z) .xxx;
	
	float4 blended_color;
	float3 blended_bump_vec;
	{
		float2 coord1 = IN.tex.yz * 4.5;
		float2 coord2 = IN.tex.zx * 4.5;
		float2 coord3 = IN.tex.xy * 4.5;

		float4 col1 = rockTexture.Sample(RockSampler, coord1);
		float4 col2 = dirtTexture.Sample(RockSampler, coord2)*0.5;
		float4 col3 = rockTexture.Sample(RockSampler, coord3);

		float2 bumpFetch1 = rockBump.Sample(RockSampler, coord1).xy - 0.5;
		float2 bumpFetch2 = rockBump.Sample(RockSampler, coord2).xy - 0.5;
		float2 bumpFetch3 = rockBump.Sample(RockSampler, coord3).xy - 0.5;
		float3 bump1 = float3(0, bumpFetch1.x, bumpFetch1.y);
		float3 bump2 = float3(bumpFetch2.y, 0, bumpFetch2.x);
		float3 bump3 = float3(bumpFetch3.x, bumpFetch3.y, 0);

		blended_color = col1.xyzw * blend_weights.xxxx +
						col2.xyzw * blend_weights.yyyy + 
						col3.xyzw * blend_weights.zzzz;

		blended_bump_vec = bump1.xyz * blend_weights.xxx + 
			bump2.xyz * blend_weights.yyy +
			bump3.xyz * blend_weights.zzz;
	}
	LightN = normalize(LightN-blended_bump_vec);


	float3 L1 = normalize(LightPos1 - Pos.xyz);
	float3 L2 = normalize(LightPos2 - Pos.xyz);
	float3 L3 = normalize(LightPos3 - Pos.xyz);

	float3 spotlight = normalize( SpotLightPos1 - Pos.xyz);


	float NdotL1 = max(0, dot(LightN, L1));
    float NdotL2 = max(0, dot(LightN, L2));
    float NdotL3 = max(0, dot(LightN, L3));
	float Ndotspot = max(0, dot(LightN, spotlight));
	
	float3 I = normalize(Pos.xyz);
	float3 V = normalize(-Pos.xyz);

	float4 vDiff = diffuseLight * Kd * NdotL1;
	float4 vDiff2 = diffuseLight * Kd * NdotL2;
	float4 vDiff3 = diffuseLight * Kd * NdotL3;
	float4 vDiff4 = diffuseLight * Kd * Ndotspot;

	float3 Color = vDiff + vDiff2 + vDiff3;

	float3 R = reflect(-(L1),LightN);

	float4 specular = SpecularColor * SpecularIntensity * pow(max(dot(R,V),0),Shininess);

	//return lerp(derp ,float4(Color, 1), 0.5);
	//return lerp(blended_color, float4(Color, 0.5f), 0.05f );

	return saturate(blended_color * (vDiff+vDiff2+vDiff3+vDiff4 + AmbientColor * AmbientIntensity + specular));

	//return blended_color * 0.5f;

	
}

////--------------------------------------------------------------------------------------
//// Colored grid
////--------------------------------------------------------------------------------------
//float4 RenderGridColor( VS_OUTPUT In ) : SV_TARGET
//{
//    float3 GridIn = (In.WorldPos) * 3.8;
//    float GridHard = 500;
//    float GridOut = 1 - Grid( GridIn, float3( 0.03, 0.03, 0.03 ), GridHard );
//    
//    float OutGrid = GridColor( GridIn );
//    
//    float4 OutColor = lerp( float4( 1, 0, 0, 1 ), float4( 0, 0, 1, 1 ), OutGrid );
//    return OutColor * GridOut;
//}



// Ideally we would like to input SampleData[4] directly, but
// HLSL compiler doesn't cope too well with "array of structures" type of input to VS
SurfaceVertex TessellateTetrahedraInstancedVS( uint tetraIndices[6] : TEXCOORD,
                                               uint vertexIndex : SV_VertexID )
{

    uint index = 0;
    SurfaceVertex Out = (SurfaceVertex)0;

    [loop]
    for (uint i = 0; i<4; i++)
        index |= uint(SampleDataBuffer.Load( tetraIndices[i] * 2 + 1 ).w > 1) << (3 - i);

    if (index > 0 && index < 15)
    {

        uint2 e = EdgeTableVS[index][vertexIndex];

        SampleData v1, v2;
    
        v1.Pos = SampleDataBuffer.Load( tetraIndices[e.x] * 2 );
        v1.Field = SampleDataBuffer.Load( tetraIndices[e.x] * 2 + 1 );
		v1.tex1 = SampleDataBuffer.Load( tetraIndices[e.x] * 2 + 2 );

        v2.Pos = SampleDataBuffer.Load( tetraIndices[e.y] * 2 );
        v2.Field = SampleDataBuffer.Load( tetraIndices[e.y] * 2 + 1 );
		v2.tex1 = SampleDataBuffer.Load( tetraIndices[e.y] * 2 + 2 );
		
        Out = CalcIntersection( v1, v2 );
    }
	Out.tex = mul(float4(Out.Pos.xyz, 0), World).xyz;



    return Out;
}

DepthStencilState EnableDepthDSS
{
    DepthEnable = true;
    DepthWriteMask = 1;
};

RasterizerState WireFrameRS
{
    MultiSampleEnable = True;
    CullMode = None;
    FillMode = WireFrame;
};

RasterizerState SolidRS
{
    MultiSampleEnable = True;
    CullMode = Front;
    FillMode = Solid;
};


technique10 MarchingTetrahedraWireFrame
{
    pass P0
    {
        SetRasterizerState( WireFrameRS );
        SetDepthStencilState( EnableDepthDSS, 0 );

        SetVertexShader( CompileShader( vs_4_0, SampleFieldVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, TessellateTetrahedraGS() ) );
        SetPixelShader( CompileShader( ps_4_0, SimplePS( float4( 0.7, 0.7, 0.7, 1 ) ) ) );
    }
}

// Tessellate isosurface in a single pass
technique10 MarchingTetrahedraSinglePassGS
{
    pass P0
    {
        SetRasterizerState( SolidRS );
        SetDepthStencilState( EnableDepthDSS, 0 );

        SetVertexShader( CompileShader( vs_4_0, SampleFieldVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, TessellateTetrahedraGS() ) );
		SetPixelShader( CompileShader( ps_4_0, DiffusePS() ) );
    }
}

// Tessellate isosurface in two passes, streaming out VS results in-between
GeometryShader StreamOutGS = ConstructGSWithSO( CompileShader( vs_4_0, PassThroughVS() ), "SV_Position.xyzw; TEXCOORD0.xyzw; TEXCOORD1.xyz;" );

technique10 MarchingTetrahedraMultiPassGS
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, SampleFieldVS() ) );
        SetGeometryShader( StreamOutGS );
        SetPixelShader( NULL );
    }
    pass P1
    {
		SetRasterizerState( SolidRS );
        SetDepthStencilState( EnableDepthDSS, 0 );

        SetVertexShader( CompileShader ( vs_4_0, PassThroughVS() ) );
        SetGeometryShader( CompileShader( gs_4_0, TessellateTetrahedraGS() ) );
        SetPixelShader( CompileShader( ps_4_0, DiffusePS() )  );
    }

}

 //Alternative technique that doesn't use geometry shaders Very slow
technique10 MarchingTetrahedraInstancedVS
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, SampleFieldVS() ) );
        SetGeometryShader( StreamOutGS );
        SetPixelShader( NULL );
    }

    pass P1
    {
        SetRasterizerState( SolidRS );
        SetDepthStencilState( EnableDepthDSS, 0 );

        SetVertexShader( CompileShader( vs_4_0, TessellateTetrahedraInstancedVS() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_4_0, DiffusePS() ) );
    }
}
