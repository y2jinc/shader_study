//--------------------------------------------------------------------------------------
// File: ShaderFramework.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include <tchar.h>

#define WIN_WIDTH	800
#define WIN_HEIGHT	600


#define PI					3.14159265f
#define FOV					(PI/4.0f)
#define ASPECT_RATIO		(WIN_WIDTH/(float)WIN_HEIGHT)
#define NEAR_PLANE			1
#define FAR_PLANE			10000

#define SHADER_FILE_NAME	L"TextureMappingWithSpecular.fx"


D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.f, 1.0f);
D3DXVECTOR4				gWorldCameraPosition(0.f, 0.f, -200.f, 1.0f);

// D3D 
LPDIRECT3D9				gpD3D			= NULL;
LPDIRECT3DDEVICE9		gpD3DDevice		= NULL;
ID3DXFont*				gpFont			= NULL;

LPD3DXMESH				gpSphere		= NULL;
ID3DXEffect*			gpShader		= NULL;

LPDIRECT3DTEXTURE9		gpFieldStoneDM		= NULL;
LPDIRECT3DTEXTURE9		gpFieldStoneSM		= NULL;

LPD3DXEFFECT LoadShader(IDirect3DDevice9* pd3dDevice)
{
	LPD3DXBUFFER pError = NULL;	
	DWORD dwShaderFlags = 0;
#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

	D3DXCreateEffectFromFile(pd3dDevice, SHADER_FILE_NAME, NULL, NULL, dwShaderFlags, NULL, &gpShader, &pError);
	if (!gpShader && pError)
	{
		int size = pError->GetBufferSize();
		void* ack = pError->GetBufferPointer();

		if (ack)
		{
			char* str = new char[size];
			sprintf_s(str, size, (const char*)ack );
			OutputDebugStringA(str);
			delete [] str;
		}

		return gpShader;
	}

    return gpShader;
}

LPD3DXMESH LoadModel(const TCHAR* filename)
{
	LPD3DXMESH	ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, NULL, NULL, NULL, NULL, &ret)) )
	{
		OutputDebugString(L"�� �ε� ����: ");
		OutputDebugString(filename);
		OutputDebugString(L"\n");
	}

	return ret;
}

HRESULT LoadTexture()
{
	if ( FAILED( D3DXCreateTextureFromFile(gpD3DDevice, L"FieldStone.bmp", &gpFieldStoneDM)) )
	{
		return S_FALSE;
	}

	if ( FAILED(D3DXCreateTextureFromFile(gpD3DDevice, L"FieldStone_SM.tga", &gpFieldStoneSM)) )
	{
		return S_FALSE;
	}

	return S_OK;
}
float gRotationY = 0;
void RenderScene()
{
	gRotationY += 0.4f * PI / 180.f;
	if (gRotationY > 2 * PI)
	{
		gRotationY -= 2 * PI;
	}

	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixRotationY(&matWorld, gRotationY);

	D3DXMATRIXA16 matView;
	D3DXVECTOR3 vEyePt(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
	D3DXVECTOR3 vLookatPt(0.f, 0.f, 0.f);
	D3DXVECTOR3 vUpVec( 0.f, 1.f, 0.f);
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec);

	D3DXMATRIXA16 matProjection;
	D3DXMatrixPerspectiveFovLH( &matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE );

	D3DXHANDLE handle = gpShader->GetParameterByName(NULL, "gWorldMatrix");
	gpShader->SetMatrix(handle, &matWorld);

	handle = gpShader->GetParameterByName(NULL, "gViewMatrix");
	gpShader->SetMatrix(handle, &matView);

	handle = gpShader->GetParameterByName(NULL, "gProjMatrix");
	gpShader->SetMatrix(handle, &matProjection);

	handle = gpShader->GetParameterByName(NULL, "DiffuseMap_Tex");
	gpShader->SetTexture(handle, gpFieldStoneDM);

	handle = gpShader->GetParameterByName(NULL, "SpecularMap_Tex");
	gpShader->SetTexture(handle, gpFieldStoneSM);

	handle = gpShader->GetParameterByName(NULL, "gLightPosition");
	gpShader->SetValue(handle, &gWorldLightPosition, sizeof(D3DXVECTOR4));

	handle = gpShader->GetParameterByName(NULL, "gCameraPosition");
	gpShader->SetValue(handle, &gWorldCameraPosition, sizeof(D3DXVECTOR4));

	UINT nPass = 0;
	gpShader->Begin(&nPass, NULL);
	for (UINT i = 0; i < nPass; ++i)
	{
		gpShader->BeginPass(i);
		{
			gpSphere->DrawSubset(0);
		}
		gpShader->EndPass();
	}
	gpShader->End();
}

void RenderInfo()
{
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	
	RECT rect;
	rect.left = 5;
	rect.right = WIN_WIDTH / 3;
	rect.top = 5;
	rect.bottom = WIN_HEIGHT / 3;

	gpFont->DrawText(NULL, L"���� �����ӿ�ũ\n\nESC: ��������", -1, &rect, 0, fontColor);
}

//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext )
{
    // Typically want to skip back buffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Before a device is created, modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
	gpD3DDevice = pd3dDevice;
	gpShader = LoadShader(pd3dDevice);
	if (!gpShader)
		return S_FALSE;

	gpSphere = LoadModel(L"sphere.x");
	if (!gpSphere)
		return S_FALSE;

	if (FAILED( LoadTexture() ))
		return S_FALSE;

	if (FAILED(D3DXCreateFont(pd3dDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
		(DEFAULT_PITCH|FF_DONTCARE), L"Arial", &gpFont )))
	{
		return S_FALSE;
	}

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
	gpFont->OnResetDevice();
	gpShader->OnResetDevice();
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

	D3DCOLOR bgColor = 0xFF0000FF;	// �Ķ�

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgColor, 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
		RenderScene();
		RenderInfo();

        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    return 0;
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
	gpFont->OnLostDevice();
	gpShader->OnLostDevice();
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
	SAFE_RELEASE(gpFieldStoneDM);
	SAFE_RELEASE(gpFieldStoneSM);
	SAFE_RELEASE(gpFont);
	SAFE_RELEASE(gpSphere);
	SAFE_RELEASE(gpShader);
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set the callback functions
    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    // TODO: Perform any application-level initialization here

    // Initialize DXUT and create the desired Win32 window and Direct3D device for the application
    DXUTInit( true, true ); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling( true, true, true );  // handle the default hotkeys
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"ShaderFramework" );
    DXUTCreateDevice( true, WIN_WIDTH, WIN_HEIGHT );

    // Start the render loop
    DXUTMainLoop();

    // TODO: Perform any application-level cleanup here

    return DXUTGetExitCode();
}


