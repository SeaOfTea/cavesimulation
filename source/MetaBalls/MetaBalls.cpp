#include <cstdlib>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTShapes.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include <time.h>

#include "NVUTMedia.h"
#include "NVUTSkybox.h"

// Tile size of 2x2x2 blocks works best
#define TILE_SIZE_X         2
#define TILE_SIZE_Y         2
#define TILE_SIZE_Z         2

#define MIN_RES             8       // minimum resolution for the grid is 8x8x4 blocks
#define MAX_RES             32     // maximum resolution for the grid is 32x32x16 blocks

#define MAX_METABALLS       400    // should match .fx file

#define frand() (((float)rand()/RAND_MAX))* 1.8f

// MetaBall
struct CMetaBall
{
    float Size;     // size of this metaball
    float R;        // radius for rotation
    float Speed;    // speed of rotation
    float Phase;    // initial phase
};

D3DXVECTOR4 Metaballs[MAX_METABALLS];

CGrowableArray<CMetaBall> g_Metaballs;  // a list of currently animating metaballs
bool g_ShowHelp = false;
bool g_Animate = false;

UINT g_nGridResolution;

// Default grid resolution
UINT g_GridBlocksX = 26;
UINT g_GridBlocksY = 26;
UINT g_GridBlocksZ = 26;

D3DXVECTOR3 WorldLightPos1( 0, 0, 0 );
D3DXVECTOR3 WorldLightPos2( -100, 0, -100 );   // for backlighting
D3DXVECTOR3 WorldLightPos3( 100, 0, -100 );   // for backlighting

D3DXVECTOR3 WorldSpotLightPos1(0,0,0);
D3DXVECTOR3 WorldSpotLightDirection1(0,0,0);


CFirstPersonCamera            g_Camera;                    // a model viewing camera
CDXUTDialogResourceManager    g_DialogResourceManager;     // manager for shared resources of dialogs
CD3DSettingsDlg               g_SettingsDlg;               // Device settings dialog
CDXUTDialog                   g_HUD;                       // dialog for standard controls
CDXUTDialog                   g_Interface;                  // dialog for sample specific controls

// Setup camera
D3DXVECTOR3 vEye(0, 0, 0.7f);
D3DXVECTOR3 vAt(0, 0, 0);

bool bCameraSet = false;



NVUTSkybox                    g_Skybox;

// Direct3D 10 resources
ID3DX10Font * g_pFont = NULL;
ID3DX10Sprite * g_pSprite = NULL;
ID3D10Effect * g_pEffect = NULL;
ID3D10StateBlock * g_pCurrentState = NULL;  // stateblock for tracking state

ID3D10Buffer * g_pGridIndices = NULL;   // Index buffer for the sampling grid

ID3D10Buffer * g_pGridVertices = NULL;  // Vertex buffer for the sampling grid
ID3D10InputLayout * g_pGridInputLayout = NULL;  // Input layout for the sampling grid

ID3D10Buffer * g_pSampleDataBuffer = NULL;   // Buffer for streamout
ID3D10InputLayout * g_pSampleDataInputLayout = NULL;  // Input layout for the stream out buffer
ID3D10ShaderResourceView * g_pSampleDataBufferSRV = NULL;   // Shader resource view for the sample data buffer
ID3D10InputLayout * g_pIndicesInputLayout = NULL;   // Input layout for instanced index buffer (VS instancing approach)

// Environment map
ID3D10Texture2D * g_pEnvMap = NULL;                    
ID3D10ShaderResourceView * g_pEnvMapSRV = NULL;

ID3D10ShaderResourceView *g_gradTexture = NULL;
ID3D10ShaderResourceView *g_permTexture = NULL;
ID3D10ShaderResourceView *g_pPermTextureSRV = NULL;
ID3D10ShaderResourceView *g_rockTexture = NULL;
ID3D10ShaderResourceView *g_dirtTexture = NULL;
ID3D10ShaderResourceView *g_rockBump = NULL;
ID3D10ShaderResourceView*g_noiseTexture = NULL;
ID3D10Texture2D *g_pJitterTex = NULL;


ID3D10EffectShaderResourceVariable *g_gradEffectVar = NULL;
ID3D10EffectShaderResourceVariable *g_permEffectVar = NULL;
ID3D10EffectShaderResourceVariable* g_ptxRandomByte = NULL;
ID3D10EffectShaderResourceVariable* g_rockEffectVar = NULL;
ID3D10EffectShaderResourceVariable* g_dirtEffectVar = NULL;
ID3D10EffectShaderResourceVariable* g_rockBumpVar = NULL;
ID3D10EffectShaderResourceVariable *g_ptxJitterTex = NULL;
ID3D10EffectShaderResourceVariable *g_noiseEffectVar = NULL;
ID3D10EffectShaderResourceVariable *g_ptxPermTex = NULL;
ID3D10ShaderResourceView* g_pJitterTextureSRV = NULL;



// Effect variables
ID3D10EffectMatrixVariable * g_pmProjInv;
ID3D10EffectMatrixVariable * g_pmViewIT;
ID3D10EffectMatrixVariable * g_pmWorldViewProj;
ID3D10EffectMatrixVariable * g_pmWorld;
ID3D10EffectScalarVariable * g_piNumMetaballs;
ID3D10EffectVectorVariable * g_pvMetaballs;
ID3D10EffectVectorVariable * g_pvLightPos1;
ID3D10EffectVectorVariable * g_pvLightPos2;
ID3D10EffectVectorVariable * g_pvLightPos3;
ID3D10EffectVectorVariable * g_pvSpotLightPos1;
ID3D10EffectVectorVariable * g_pvSpotLightDirection1;
ID3D10EffectVectorVariable * g_pvViewportOrg;
ID3D10EffectVectorVariable * g_pvEyeVec;
ID3D10EffectVectorVariable * g_pvViewportSizeInv;
ID3D10EffectShaderResourceVariable * g_pbSampleData;

// Effect Techniques
ID3D10EffectTechnique * g_pMarchingTetrahedraSinglePassGS;
ID3D10EffectTechnique * g_pMarchingTetrahedraMultiPassGS;
ID3D10EffectTechnique * g_pMarchingTetrahedraInstancedVS;
ID3D10EffectTechnique * g_pMarchingTetrahedraWireFrame;

ID3D10EffectTechnique * g_pCurrentTechnique;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC             -1
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_ADDMETABALL         4
#define IDC_REMOVEMETABALL      5
#define IDC_NUMMETABALLS        6
#define IDC_GRIDRESOLUTION      7
#define IDC_GRIDSIZE            8
#define IDC_TECHNIQUE           9

void GenerateGridTile(UINT width, UINT height, UINT depth, DWORD *indices, BYTE *vertices, UINT baseX, UINT baseY, UINT baseZ, DWORD baseIndex)
{
    // Generate vertex data first

    for (UINT z = 0; z<=depth; z++) {
        for (UINT y = 0; y<=height; y++) {
            for (UINT x = 0; x<=width; x++) {
                *vertices++ = (BYTE)(x + baseX);
                *vertices++ = (BYTE)(y + baseY);
                *vertices++ = (BYTE)(z + baseZ);
                *vertices++ = 255;
            }
        }
    }

    // Generate indices for all blocks in a tile

#define MAKE_INDEX(x, y, z) (DWORD)(((z)*(height + 1) + (y))*(width + 1) + (x) + baseIndex)

    for (UINT z = 0; z<depth; z++) {
        for (UINT y = 0; y<height; y++) {
            for (UINT x = 0; x<width; x++) {
                
                // T0
                *indices++ = MAKE_INDEX(x + 1, y, z);
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);

                // T1
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z);
                *indices++ = MAKE_INDEX(x, y + 1, z);

                // T2
                *indices++ = MAKE_INDEX(x, y + 1, z);
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x, y + 1, z + 1);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);

                // T3
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x, y, z + 1);
                *indices++ = MAKE_INDEX(x, y + 1, z + 1);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);

                // T4
                *indices++ = MAKE_INDEX(x, y, z + 1);
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x + 1, y, z + 1);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);

                // T5
                *indices++ = MAKE_INDEX(x, y, z);
                *indices++ = MAKE_INDEX(x + 1, y, z);
                *indices++ = MAKE_INDEX(x + 1, y, z + 1);
                *indices++ = MAKE_INDEX(x + 1, y + 1, z + 1);
            }
        }
    }
}

void GenerateGridBuffers(ID3D10Device * pd3dDevice, UINT nTilesX, UINT nTilesY, UINT nTilesZ, UINT nCellsPerTileX, UINT nCellsPerTileY, UINT nCellsPerTileZ)
{
    UINT nTotalTiles = nTilesX * nTilesY * nTilesZ;
    UINT nCellsPerTile = nCellsPerTileX * nCellsPerTileY * nCellsPerTileZ;
    UINT nVerticesPerTile = (nCellsPerTileX + 1) * (nCellsPerTileY + 1) * (nCellsPerTileZ + 1);

    DWORD * pIndices = new DWORD[nTotalTiles * nCellsPerTile * 24];  // 24 indices per cell
    BYTE * pVertices = new BYTE[nTotalTiles * nVerticesPerTile * 4]; // 4 bytes per vertex

    DWORD * indices = pIndices;
    BYTE * vertices = pVertices;

    UINT baseIndex = 0;

    for (UINT z = 0; z<nTilesZ; z++) {
        for (UINT y = 0; y<nTilesY; y++) {
            for (UINT x = 0; x<nTilesX; x++) {

                GenerateGridTile(nCellsPerTileX, nCellsPerTileY, nCellsPerTileZ,
                                indices, vertices,
                                x * nCellsPerTileX, y * nCellsPerTileY, z * nCellsPerTileZ, baseIndex);

                indices += nCellsPerTile * 24;
                vertices += nVerticesPerTile * 4;
                baseIndex += nVerticesPerTile;
            }
        }
    }

    // Upload index buffer for the sampling grid
    D3D10_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = nTotalTiles * nCellsPerTile * 24 * sizeof(DWORD);
    bufferDesc.Usage = D3D10_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER | D3D10_BIND_VERTEX_BUFFER;  // some techniques bind this buffer as a vertex buffer
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem = pIndices;

    HRESULT hr;
    V( pd3dDevice->CreateBuffer( &bufferDesc, &initData, &g_pGridIndices ) );

    delete [] pIndices;

    // Upload vertex buffer for the sampling grid
    bufferDesc.ByteWidth = nTotalTiles * nVerticesPerTile * 4 * sizeof(BYTE);
    bufferDesc.Usage = D3D10_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    initData.pSysMem = pVertices;

    V( pd3dDevice->CreateBuffer( &bufferDesc, &initData, &g_pGridVertices ) );

    delete [] pVertices;

    // Create streamout buffer
    bufferDesc.ByteWidth = nTotalTiles * nVerticesPerTile * 11 * sizeof(float);    // sizeof(SampleData) in .fx
    bufferDesc.Usage = D3D10_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D10_BIND_STREAM_OUTPUT | D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_SHADER_RESOURCE;    // some techniques bind this buffer as a shader resource
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

	V( pd3dDevice->CreateBuffer( &bufferDesc, NULL, &g_pSampleDataBuffer ) );

    // Create a shader resource view for the stream out buffer
    D3D10_SHADER_RESOURCE_VIEW_DESC bufferSRV;
    bufferSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    bufferSRV.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    bufferSRV.Buffer.ElementOffset = 0;
    bufferSRV.Buffer.ElementWidth = nTotalTiles * nVerticesPerTile * 2; // we have 2 float4's per vertex

    V( pd3dDevice->CreateShaderResourceView( g_pSampleDataBuffer, &bufferSRV, &g_pSampleDataBufferSRV ) );
}

void PlaceMetaballs(CMetaBall *m)
{
	    m->Size = frand()* 0.075f + 0.03f; // randomize size

        m->R = m->Size * 2.5f + 0.08f;         // bigger metaballs tend to have bigger radii
        m->Speed = 1.0f +frand() / m->Size * 0.025f * frand();   // the bigger it is, the slower it moves
        m->Phase = frand() * frand() * 6.28f + frand();          // scatter them randomly 6.28f
}



void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{

    switch( nControlID )
    {
    case IDC_TOGGLEFULLSCREEN:  DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:         DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:      g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    case IDC_ADDMETABALL:
        CMetaBall m;

		PlaceMetaballs(&m);

        if (g_Metaballs.GetSize() < MAX_METABALLS) g_Metaballs.Add( m );    // add to the list
        break;

    case IDC_REMOVEMETABALL:
        if (g_Metaballs.GetSize() > 0) g_Metaballs.Remove( g_Metaballs.GetSize() - 1 ); // remove the last one from the list
        break;

    case IDC_GRIDRESOLUTION:
        {
			CDXUTSlider * pSlider = (CDXUTSlider*) pControl;

            g_GridBlocksX = pSlider->GetValue();
            g_GridBlocksY = pSlider->GetValue();
            g_GridBlocksZ = pSlider->GetValue();  

            SAFE_RELEASE( g_pGridIndices );
            SAFE_RELEASE( g_pGridVertices);
            SAFE_RELEASE( g_pSampleDataBuffer );
            SAFE_RELEASE( g_pSampleDataBufferSRV );

            GenerateGridBuffers( DXUTGetD3D10Device(), g_GridBlocksX, g_GridBlocksY, g_GridBlocksZ, TILE_SIZE_X, TILE_SIZE_Y, TILE_SIZE_Z );
        }

        break;
    case IDC_TECHNIQUE:
        {
            CDXUTComboBox* pComboBox = (CDXUTComboBox*) pControl;
            ID3D10EffectTechnique * pTechnique = (ID3D10EffectTechnique*)pComboBox->GetSelectedData();

            g_pCurrentTechnique = pTechnique;

        }
        break;

    }

    WCHAR sz[100];
    StringCchPrintf( sz, 100, L"%d metaballs", g_Metaballs.GetSize() ); 
    g_Interface.GetStatic( IDC_NUMMETABALLS )->SetText( sz );

    StringCchPrintf( sz, 100, L"Grid resolution: %dx%dx%d", g_GridBlocksX * TILE_SIZE_X, g_GridBlocksY * TILE_SIZE_Y, g_GridBlocksZ * TILE_SIZE_Z ); 
    g_Interface.GetStatic( IDC_GRIDSIZE )->SetText( sz );
}

// Initialize the app
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_Interface.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    //  g_Interface.EnableKeyboardInput( true );
    g_Interface.SetCallback( OnGUIEvent );

    iY = 0;
    CDXUTComboBox* pComboBox = NULL;
    g_Interface.AddComboBox( IDC_TECHNIQUE, 35, iY += 48, 125, 24, 'S', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 30 );

    g_Interface.AddStatic( IDC_NUMMETABALLS, L"10 metaballs", 35, iY += 48, 125, 25 );    
    g_Interface.AddButton( IDC_ADDMETABALL, L"(A)dd metaball", 35, iY += 48, 125, 22, 'A');
    g_Interface.AddButton( IDC_REMOVEMETABALL, L"(R)emove metaball", 35, iY += 24, 125, 22, 'R');

    iY += 24;
    g_Interface.AddSlider( IDC_GRIDRESOLUTION, 35, iY += 48, 125, 22, MIN_RES, MAX_RES, g_GridBlocksX );
    g_Interface.AddStatic( IDC_GRIDSIZE, L"Grid resolution:", 35, iY += 24, 125, 25 );

    // Start with 100 metaballs
    for (int i = 0; i<100; i++)
        OnGUIEvent( 0, IDC_ADDMETABALL, NULL, NULL );
}

// Render text
void RenderText()
{
    CDXUTTextHelper txtHelper(g_pFont, g_pSprite, 15);
    
    txtHelper.Begin();
    
    txtHelper.SetInsertionPos( 5, 5 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    txtHelper.DrawTextLine( DXUTGetFrameStats(true) );
    txtHelper.DrawTextLine( DXUTGetDeviceStats() );
    
    if( g_ShowHelp )
    {
        UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;
        txtHelper.SetInsertionPos( 2, nBackBufferHeight- 15 * 6 );
        txtHelper.SetForegroundColor( D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Controls:" );

        txtHelper.SetInsertionPos( 20, nBackBufferHeight - 15 * 5 );
        txtHelper.DrawTextLine( L"'A' to add metaball\n"
                                    L"'R' to remove metaball\n"
                                    L"'P' to toggle animation\n"
                                    L"'W' to toggle wireframe\n");


        txtHelper.SetInsertionPos( 250, nBackBufferHeight - 15 * 5 );
        txtHelper.DrawTextLine( L"Hide help: F1\nQuit: ESC\n" );
    }
    else
    {
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Press F1 for help" );
    }

    txtHelper.End();
}

//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial", &g_pFont ) );

    V_RETURN( g_Skybox.OnCreateDevice( pd3dDevice ) );

    D3D10_STATE_BLOCK_MASK stateMask;
    ZeroMemory( &stateMask, sizeof(stateMask) );
    stateMask.RSRasterizerState = 0xff;
    V_RETURN( D3D10CreateStateBlock( pd3dDevice, &stateMask, &g_pCurrentState ) );
    
    GenerateGridBuffers( pd3dDevice, g_GridBlocksX, g_GridBlocksY, g_GridBlocksZ, TILE_SIZE_X, TILE_SIZE_Y, TILE_SIZE_Z );

    // create effect
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"MetaBalls.fx") );
    ID3D10Blob *pErrors;
    if (D3DX10CreateEffectFromFile(str, NULL, NULL, "fx_4_0", 0, 0, pd3dDevice, NULL, NULL, &g_pEffect, &pErrors, &hr) != S_OK)
    {
        MessageBoxA(NULL, (char *)pErrors->GetBufferPointer(), "Compilation error", MB_OK);
        exit(0);
    }

    g_pMarchingTetrahedraSinglePassGS = g_pEffect->GetTechniqueByName( "MarchingTetrahedraSinglePassGS" );
    g_pMarchingTetrahedraMultiPassGS = g_pEffect->GetTechniqueByName( "MarchingTetrahedraMultiPassGS" );
    g_pMarchingTetrahedraInstancedVS = g_pEffect->GetTechniqueByName( "MarchingTetrahedraInstancedVS" );
    g_pMarchingTetrahedraWireFrame = g_pEffect->GetTechniqueByName( "MarchingTetrahedraWireFrame" );

    // Populate combo-box
    CDXUTComboBox * pComboBox = g_Interface.GetComboBox( IDC_TECHNIQUE );
	pComboBox->RemoveAllItems();
    pComboBox->AddItem( L"GS single-pass", g_pMarchingTetrahedraSinglePassGS );
    pComboBox->AddItem( L"GS multi-pass", g_pMarchingTetrahedraMultiPassGS );
    pComboBox->AddItem( L"Instanced VS", g_pMarchingTetrahedraInstancedVS );
    pComboBox->AddItem( L"Wireframe", g_pMarchingTetrahedraWireFrame );
    
    g_pCurrentTechnique = g_pMarchingTetrahedraMultiPassGS;

    // Input layout will read grid vertices as byte UNORMs
    D3D10_INPUT_ELEMENT_DESC desc;
    desc.AlignedByteOffset = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SemanticName = "POSITION";
    desc.SemanticIndex = 0;
    desc.InputSlot = 0;
    desc.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
    desc.InstanceDataStepRate = 0;

    D3D10_PASS_DESC passDesc;
    g_pMarchingTetrahedraSinglePassGS->GetPassByIndex(0)->GetDesc(&passDesc);

    V( pd3dDevice->CreateInputLayout(&desc, 1, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &g_pGridInputLayout) );

    // Create input layout for the streamout buffer
    D3D10_INPUT_ELEMENT_DESC streamOutLayout[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

    g_pMarchingTetrahedraMultiPassGS->GetPassByIndex(1)->GetDesc(&passDesc);
    V( pd3dDevice->CreateInputLayout(streamOutLayout, sizeof(streamOutLayout) / sizeof(D3D10_INPUT_ELEMENT_DESC), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &g_pSampleDataInputLayout) );

    // Create input layout for the index buffer (instanced VS approach)
    D3D10_INPUT_ELEMENT_DESC indicesInputLayout[] =
    {
        { "TEXCOORD", 0, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD", 2, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD", 3, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 4, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 5, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 6, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
    };

    g_pMarchingTetrahedraInstancedVS->GetPassByIndex(1)->GetDesc(&passDesc);
    V( pd3dDevice->CreateInputLayout(indicesInputLayout, sizeof(indicesInputLayout) / sizeof(D3D10_INPUT_ELEMENT_DESC), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &g_pIndicesInputLayout) );

    g_pmProjInv = g_pEffect->GetVariableByName( "ProjInv" )->AsMatrix();
    g_pmViewIT = g_pEffect->GetVariableByName( "ViewIT" )->AsMatrix();
    g_pmWorldViewProj = g_pEffect->GetVariableByName( "WorldViewProj" )->AsMatrix();
    g_pmWorld = g_pEffect->GetVariableByName( "World" )->AsMatrix();
    g_piNumMetaballs = g_pEffect->GetVariableByName( "NumMetaballs" )->AsScalar();
    g_pvMetaballs = g_pEffect->GetVariableByName( "Metaballs" )->AsVector();
    g_pvLightPos1 = g_pEffect->GetVariableByName( "LightPos1" )->AsVector();
    g_pvLightPos2 = g_pEffect->GetVariableByName( "LightPos2" )->AsVector();
    g_pvLightPos3 = g_pEffect->GetVariableByName( "LightPos3" )->AsVector();
	g_pvSpotLightPos1 = g_pEffect->GetVariableByName( "SpotLightPos1" )->AsVector();
	g_pvSpotLightDirection1 = g_pEffect->GetVariableByName( "SpotLightDirection1" )->AsVector();
    g_pvViewportOrg = g_pEffect->GetVariableByName( "ViewportOrg" )->AsVector();
	g_pvEyeVec = g_pEffect->GetVariableByName( "EyeVec" )->AsVector();
    g_pvViewportSizeInv = g_pEffect->GetVariableByName( "ViewportSizeInv" )->AsVector();
    g_pbSampleData = g_pEffect->GetVariableByName( "SampleDataBuffer" )->AsShaderResource();

    //// Load envmap texture
    //ID3D10Resource * pRes = NULL;
    //V( NVUTFindDXSDKMediaFileCch( str, MAX_PATH, L"nvlobby_cube_mipmap.dds" ) );
    //V( D3DX10CreateTextureFromFile( pd3dDevice, str, NULL, NULL, &pRes, &hr ) );
    //if(pRes)
    //{
    //    V( pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (LPVOID*)&g_pEnvMap ));
    //    if (g_pEnvMap)
    //    {
    //        D3D10_TEXTURE2D_DESC desc;
    //        g_pEnvMap->GetDesc( &desc );
    //        D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    //        ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
    //        SRVDesc.Format = desc.Format;
    //        SRVDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
    //        SRVDesc.TextureCube.MipLevels = desc.MipLevels;
    //        V( pd3dDevice->CreateShaderResourceView( g_pEnvMap, &SRVDesc, &g_pEnvMapSRV ));
    //    }
    //    SAFE_RELEASE(pRes);
    //}

    //V( g_pEffect->GetVariableByName( "EnvMap" )->AsShaderResource()->SetResource( g_pEnvMapSRV ) );

    //g_Skybox.SetTexture( g_pEnvMapSRV );

	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
		L"..\\..\\Media\\PerlinPerm2D.dds", NULL, NULL, &g_permTexture, NULL);

	if(FAILED(hr))
		return false;

	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
		L"..\\..\\Media\\PerlinGrad2D.dds", NULL, NULL, &g_gradTexture, NULL);

	if(FAILED(hr))
		return false;

	
	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
		L"..\\..\\Media\\rockTexture.dds", NULL, NULL, &g_rockTexture, NULL);

	if(FAILED(hr))
		return false;

	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
	L"..\\..\\Media\\rockBump.dds", NULL, NULL, &g_rockBump, NULL);

	if(FAILED(hr))
		return false;

	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
	L"..\\..\\Media\\dirtTexture2.dds", NULL, NULL, &g_dirtTexture, NULL);

	if(FAILED(hr))
		return false;

	hr = D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
	L"..\\..\\Media\\noise.dds", NULL, NULL, &g_noiseTexture, NULL);

	if(FAILED(hr))
		return false;


	g_permEffectVar = g_pEffect->GetVariableByName("texPerlinPerm2D")->AsShaderResource();

	g_gradEffectVar = g_pEffect->GetVariableByName("texPerlinGrad2D")->AsShaderResource();

	g_rockEffectVar = g_pEffect->GetVariableByName("rockTexture")->AsShaderResource();
	g_dirtEffectVar = g_pEffect->GetVariableByName("dirtTexture")->AsShaderResource();
	g_noiseEffectVar = g_pEffect->GetVariableByName("noiseTexture")->AsShaderResource();
	g_rockBumpVar = g_pEffect->GetVariableByName("rockBump")->AsShaderResource();

	g_permEffectVar->SetResource(g_permTexture);
	g_gradEffectVar->SetResource(g_gradTexture);
	g_noiseEffectVar->SetResource(g_noiseTexture);

	g_rockEffectVar->SetResource(g_rockTexture);
	g_dirtEffectVar->SetResource(g_dirtTexture);
	g_rockBumpVar->SetResource(g_rockBump);


	SAFE_RELEASE(g_permTexture);
	SAFE_RELEASE(g_gradTexture);
	SAFE_RELEASE( g_pPermTextureSRV );
	SAFE_RELEASE(g_rockTexture);
	SAFE_RELEASE(g_dirtTexture);
	SAFE_RELEASE(g_rockBump);
	SAFE_RELEASE(g_noiseTexture);

	V( g_pCurrentTechnique->GetPassByIndex( 0 )->Apply( 0 ) );


	D3DX10_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory( &loadInfo, sizeof(D3DX10_IMAGE_LOAD_INFO) );
	loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	loadInfo.Format = DXGI_FORMAT_BC1_UNORM;



    //g_Camera.SetViewParams( &vEye, &vAt );
	g_Camera.SetResetCursorAfterMove(1);
	g_Camera.SetDrag(1, 0.3f);
	g_Camera.SetScalers(0.005f, 0.15f);
    //g_Camera.SetModelCenter( D3DXVECTOR3(0, 0, 0) );
    //g_Camera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_Skybox.OnResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc );

    // Setup the camera's projection parameters
    float aspectRatio = pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams(D3DX_PI / 3, aspectRatio, 0.01f, 1000.0f);
    //g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0);
    g_HUD.SetSize( 170, 170 );
    g_Interface.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height-300 );
    g_Interface.SetSize( 170, 300 );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // Clear render target and the depth stencil 
    float ClearColor[4] = { 255, 255, 255 };
    pd3dDevice->ClearRenderTargetView( DXUTGetD3D10RenderTargetView(), ClearColor );
    pd3dDevice->ClearDepthStencilView( DXUTGetD3D10DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0 );

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    D3DXMATRIX mView = *g_Camera.GetViewMatrix();
    D3DXMATRIX mProj = *g_Camera.GetProjMatrix();

    // Our source vertex data are bytes in the range [0..g_GridBlocks * TILE_SIZE]
    // Since input layout fetches this data as UNORMs, scale our grid appropriately to arrive at a unit cube
    D3DXMATRIX mScaleToUnitCube;
	float cubeScaler = 255.0f;
    D3DXMatrixScaling( &mScaleToUnitCube, cubeScaler / (g_GridBlocksX * TILE_SIZE_X), cubeScaler / (g_GridBlocksY * TILE_SIZE_Y), cubeScaler / (g_GridBlocksZ * TILE_SIZE_Z) );

    // ... translate it such that the cube center is at (0, 0, 0) in world-space
    D3DXMATRIX mTranslate;
    D3DXMatrixTranslation( &mTranslate, -0.5f, -0.5f, -0.5f);

    // ... and scale again to encompass the expected iso-surface extents in world-space
    D3DXMATRIX mScale;
    D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f);
	
	D3DXMATRIX mWorld = mScaleToUnitCube * mTranslate * mScale;


    D3DXMATRIX mWorldView = mWorld * mView;
    D3DXMATRIX mWorldViewProj = mWorldView * mProj;

    D3DXMATRIX mViewIT;

    D3DXMatrixInverse( &mViewIT, NULL, &mView );
    D3DXMatrixTranspose( &mViewIT, &mViewIT );

    g_pmWorld->SetMatrix( mWorld );
    g_pmViewIT->SetMatrix( mViewIT );
    g_pmWorldViewProj->SetMatrix( mWorldViewProj );

    // Get current viewport
    D3D10_VIEWPORT viewport;
    UINT nViewports = 1;
    pd3dDevice->RSGetViewports(&nViewports, &viewport);
    float ViewportOrg[3] = { (float)viewport.TopLeftX, (float)viewport.TopLeftY, viewport.MinDepth };
    float ViewportSizeInv[3] = { 1.0f / viewport.Width, 1.0f / viewport.Height, 1.0f / (viewport.MaxDepth - viewport.MinDepth) };

    g_pvViewportOrg->SetFloatVector( ViewportOrg );
    g_pvViewportSizeInv->SetFloatVector( ViewportSizeInv );
	
	g_pvEyeVec->SetFloatVector(vEye);

    D3DXMATRIX mProjInv;
    D3DXMatrixInverse( &mProjInv, NULL, &mProj );

    g_pmProjInv->SetMatrix(mProjInv);

    static float time = 40.0f;


    if (g_Animate) time += fElapsedTime;

    // Animate metaballs
    /*D3DXVECTOR4 Metaballs[MAX_METABALLS];*/
    for (int i = 0; i<g_Metaballs.GetSize(); i++)
    {
        const CMetaBall& metaBall = g_Metaballs.GetAt(i);

  //      // Metaballs circle around (0, 0, 0) in world space
  //      Metaballs[i].x = metaBall.R * sinf(time * metaBall.Speed + metaBall.Phase);
  //      Metaballs[i].z = metaBall.R * sinf(time * metaBall.Speed + metaBall.Phase);
		//Metaballs[i].y = metaBall.R * cosf(time * metaBall.Speed + metaBall.Phase);

		// Metaballs circle around (0, 0, 0) in world space
        Metaballs[i].x = 0.6f * metaBall.R * cosf(time * metaBall.Speed * metaBall.Phase);
		Metaballs[i].y = 0;
		Metaballs[i].z = metaBall.R * sinf(time * metaBall.Speed * metaBall.Phase);


        // ... and wiggle in Y
        Metaballs[i].y = 0.3f * metaBall.R * sinf(2 * time * metaBall.Speed + metaBall.Phase);

        // the overall amount of "matter" remains approx. constant
        Metaballs[i].w = metaBall.Size / 200 ;
    }

    g_piNumMetaballs->SetInt( g_Metaballs.GetSize() );
    g_pvMetaballs->SetFloatVectorArray( (float*)Metaballs, 0, g_Metaballs.GetSize() * 4 );

    D3DXVECTOR4 ViewLightPos1;
	D3DXVECTOR4 ViewLightPos2;
	D3DXVECTOR4 ViewLightPos3;

	D3DXVECTOR4 ViewSpotLightPos1;
	D3DXVECTOR4 ViewSpotLightDirection1;

	//D3DXVECTOR3 light = *g_Camera.GetEyePt();

	//WorldLightPos1 = (light);

	D3DXVECTOR3 vEye = *g_Camera.GetEyePt();
	D3DXVECTOR3 vAt = *g_Camera.GetWorldAhead();
	WorldSpotLightPos1 = (vEye);
	WorldSpotLightDirection1 = (vAt);


    D3DXVec3Transform( &ViewLightPos1, &WorldLightPos1, &mView );
    g_pvLightPos1->SetFloatVector( ViewLightPos1 );

    D3DXVec3Transform( &ViewLightPos2, &WorldLightPos2, &mView );
    g_pvLightPos2->SetFloatVector( ViewLightPos2 );

    D3DXVec3Transform( &ViewLightPos3, &WorldLightPos3, &mView );
    g_pvLightPos3->SetFloatVector( ViewLightPos3 );

	//D3DXVec3Transform( &ViewSpotLightPos1, &WorldSpotLightPos1, &mView );
    g_pvSpotLightPos1->SetFloatVector( D3DXVECTOR4(0,0,0,1) );

	D3DXVec3Transform( &ViewSpotLightDirection1, &WorldSpotLightDirection1, &mView );
    g_pvSpotLightDirection1->SetFloatVector( ViewSpotLightDirection1 );

	int largestBall = 0;
	for(int i = 0; i < g_Metaballs.GetSize(); i++)
	{
		const CMetaBall& metaBall = g_Metaballs.GetAt(i);

		const CMetaBall& metaBall2 = g_Metaballs.GetAt(largestBall);
		if (metaBall.Size * Metaballs[i].w > metaBall2.Size * Metaballs[largestBall].w)
		{
			largestBall = i;
		}

	}
	if(!bCameraSet)
	{
		vEye.x = Metaballs[0].x;
		vEye.y = Metaballs[0].y;
		vEye.z = Metaballs[0].z;
		g_Camera.SetViewParams( &vEye, &vAt );
		bCameraSet = true;
	}
	
	//D3DXVECTOR3 cameraPosition = *g_Camera.GetEyePt();

	//for(int i=0; i < g_Metaballs.GetSize(); i++)
	//{
	//	//const CMetaBall& metaBall = g_Metaballs.GetAt(i);
	//	D3DXVECTOR3 ballPos(Metaballs[i].x, Metaballs[i].y, Metaballs[i].z);

	//	D3DXVECTOR3 relPos = cameraPosition - ballPos;

	//	float dist = relPos.x * relPos.x + relPos.y * relPos.y + relPos.z * relPos.z;

	//	float cameraRadius = 0.2f;
	//	float minDist = Metaballs[i].w + cameraRadius;
	//	float bur = minDist * minDist;
	//	if(dist > minDist * minDist)
	//	{
	//		//hit this
	//		bur++;
	//	}
	//}

    // Save current state
    g_pCurrentState->Capture();

    // Render skybox
    g_Skybox.OnFrameRender( mWorldViewProj );

    // Render metaballs

    ID3D10Buffer *pVBs[] = { g_pGridVertices };
    UINT gridStrides[] = { sizeof(BYTE) * 4 };
    UINT gridOffsets[] = { 0 };

    pd3dDevice->IASetVertexBuffers( 0, 1, pVBs, gridStrides, gridOffsets );
    pd3dDevice->IASetInputLayout( g_pGridInputLayout );


    if (g_pCurrentTechnique != g_pMarchingTetrahedraSinglePassGS && g_pCurrentTechnique != g_pMarchingTetrahedraWireFrame)
    {
        // Set streamout bufer
        UINT offset[] = { 0 };

        pd3dDevice->SOSetTargets( 1, &g_pSampleDataBuffer, offset );

        // Calculate scalar field at grid sampling points and stream out to memory
        pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
    
          V( g_pCurrentTechnique->GetPassByIndex( 0 )->Apply( 0 ) );
        pd3dDevice->Draw(g_GridBlocksX * (TILE_SIZE_X + 1) * g_GridBlocksY * (TILE_SIZE_Y + 1) * g_GridBlocksZ * (TILE_SIZE_Z + 1), 0);

        // Unset streamout bufer
        ID3D10Buffer *pNullBuffers[] = { NULL };
        pd3dDevice->SOSetTargets( 1, pNullBuffers, offset );
    }
    else
    {
        // Render entire surface in a single pass

        pd3dDevice->IASetIndexBuffer( g_pGridIndices, DXGI_FORMAT_R32_UINT, 0 );
        pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ );

        V( g_pCurrentTechnique->GetPassByIndex( 0 )->Apply( 0 ) );
        pd3dDevice->DrawIndexed( 24 * g_GridBlocksX * g_GridBlocksY * g_GridBlocksZ * TILE_SIZE_X * TILE_SIZE_Y * TILE_SIZE_Z, 0, 0 );
    }

    if (g_pCurrentTechnique == g_pMarchingTetrahedraMultiPassGS)
    {
        // Tessellate tetrahedra
        UINT streamOutStrides[] = { 11 * sizeof(float) }; // == sizeof(SampleData) in .fx
        UINT streamOutOffsets[] = { 0 };
        pd3dDevice->IASetVertexBuffers( 0, 1, &g_pSampleDataBuffer, streamOutStrides, streamOutOffsets );
        pd3dDevice->IASetInputLayout( g_pSampleDataInputLayout );
        pd3dDevice->IASetIndexBuffer( g_pGridIndices, DXGI_FORMAT_R32_UINT, 0 );
        pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ );

        V( g_pCurrentTechnique->GetPassByIndex( 1 )->Apply( 0 ) );
        pd3dDevice->DrawIndexed( 24 * g_GridBlocksX * g_GridBlocksY * g_GridBlocksZ * TILE_SIZE_X * TILE_SIZE_Y * TILE_SIZE_Z, 0, 0 );
    }
    else if (g_pCurrentTechnique == g_pMarchingTetrahedraInstancedVS)
    {
        UINT indicesStrides[] = { 4 * sizeof(UINT) };
        UINT indicesOffsets[] = { 0 };

        pd3dDevice->IASetVertexBuffers( 0, 1, &g_pGridIndices, indicesStrides, indicesOffsets );
        pd3dDevice->IASetInputLayout( g_pIndicesInputLayout );
        pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

        V( g_pbSampleData->SetResource( g_pSampleDataBufferSRV ) );

        V( g_pCurrentTechnique->GetPassByIndex( 1 )->Apply( 0 ) );
        pd3dDevice->DrawInstanced( 4, 6 * g_GridBlocksX * g_GridBlocksY * g_GridBlocksZ * TILE_SIZE_X * TILE_SIZE_Y * TILE_SIZE_Z, 0, 0 );  // 6 tetrahedra per cell

        // Unset sample data buffer
        V( g_pbSampleData->SetResource( NULL ) );
        V( g_pCurrentTechnique->GetPassByIndex( 1 )->Apply( 0 ) );  // to commit changes
    }

    // Restore state modified by .fx
    g_pCurrentState->Apply();


	//HUD Elements Comment as Required

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    RenderText();
    g_HUD.OnRender( fElapsedTime ); 
    g_Interface.OnRender( fElapsedTime );
    DXUT_EndPerfEvent();
}

//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10ReleasingSwapChain();

    g_Skybox.OnReleasingSwapChain();

    ID3D10RenderTargetView * pNullView = { NULL };
    DXUTGetD3D10Device()->OMSetRenderTargets( 1, &pNullView, NULL );
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10DestroyDevice();
    g_SettingsDlg.OnD3D10DestroyDevice();

    g_Skybox.OnDestroyDevice();

    SAFE_RELEASE( g_pCurrentState );
    SAFE_RELEASE( g_pEnvMapSRV );
    SAFE_RELEASE( g_pEnvMap );
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pGridIndices );
    SAFE_RELEASE( g_pGridVertices );
    SAFE_RELEASE( g_pGridInputLayout );

    SAFE_RELEASE( g_pSampleDataBuffer );
    SAFE_RELEASE( g_pSampleDataInputLayout );
    SAFE_RELEASE( g_pSampleDataBufferSRV );

    SAFE_RELEASE( g_pIndicesInputLayout );
    
    SAFE_RELEASE( g_pFont );
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D10_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

#ifdef _DEBUG
    pDeviceSettings->d3d10.CreateFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    return true;
}

void DoNothing()
{
	//return
}
//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    g_Camera.FrameMove( fElapsedTime );

	D3DXVECTOR3 vEye = *g_Camera.GetEyePt();
	D3DXVECTOR3 vAt = *g_Camera.GetLookAtPt();

	float cameraRadius = 0.02f;
	
	int count = 0;

	for(int i = 0; i < g_Metaballs.GetSize(); i++)
	{
		const CMetaBall& metaBall = g_Metaballs.GetAt(i);
		float radiusSum = (cameraRadius + metaBall.Size);
		D3DXVECTOR3 metaball(Metaballs[i].x, Metaballs[i].y, Metaballs[i].z);

		float dx=metaball.x - vEye.x;
		float dy=metaball.y - vEye.y;
		float dz=metaball.z - vEye.z;
		float distance = sqrt(dx*dx + dy*dy + dz*dz);

		if(distance >= radiusSum)
		{
			count++;
		}
		if(count >= g_Metaballs.GetSize()-1)
		{
			DoNothing();
		}
	}

}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                         bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    *pbNoFurtherProcessing = g_Interface.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1: g_ShowHelp = !g_ShowHelp; break;
            case 'P': g_Animate = !g_Animate; break;
			case 'I': WorldLightPos1.z -= 1; break;
			case 'J': WorldLightPos1.x += 1; break;
			case 'K': WorldLightPos1.z += 1; break;
			case 'L': WorldLightPos1.x -= 1; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, 
                      bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, 
                      int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	srand ((unsigned int)time(NULL)); //New rand seed
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D10 DXUT callbacks. Remove these sets if the app doesn't need to support D3D10
    DXUTSetCallbackD3D10DeviceAcceptable( IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );
    DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );

    // IMPORTANT: set SDK media search path to include source directory of this sample, when started from .\Bin
    HRESULT hr;
    V_RETURN( DXUTSetMediaSearchPath( L"..\\Source\\MetaBalls" ) );

    InitApp();

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Cave Simulation" );
    DXUTCreateDevice( true, 800, 600 );  
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}
