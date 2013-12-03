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

#define SHADER_FILE_NAME	L"ShadowMapping.fx"


D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.f, 1.0f);
D3DXVECTOR4				gWorldCameraPosition(0.f, 0.f, -200.f, 1.0f);

// D3D 
LPDIRECT3D9				gpD3D			= NULL;
LPDIRECT3DDEVICE9		gpD3DDevice		= NULL;
ID3DXFont*				gpFont			= NULL;

LPD3DXMESH				gpTorus					= NULL;
LPD3DXMESH				gpDisc					= NULL;
LPD3DXEFFECT			gpCreateShadowShader	= NULL;
LPD3DXEFFECT			gpApplyShadowShader		= NULL;

// 물체 색상
D3DXVECTOR4				gTorusColor(1,1,0,1);
D3DXVECTOR4				gDiscColor(0,1,1,1);

// 그림자맵 텍스쳐
LPDIRECT3DTEXTURE9		gpShadowRenderTarget = NULL;
LPDIRECT3DTEXTURE9		gpShadowDepthStencil = NULL;

LPDIRECT3DTEXTURE9		gpFieldStoneDM		= NULL;
LPDIRECT3DTEXTURE9		gpFieldStoneSM		= NULL;

LPD3DXEFFECT LoadShader(TCHAR* szShadarFile)
{
	LPD3DXBUFFER pError = NULL;	
	DWORD dwShaderFlags = 0;
#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif
	LPD3DXEFFECT pShaderEffect;
	D3DXCreateEffectFromFile(gpD3DDevice, szShadarFile, NULL, NULL, dwShaderFlags, NULL, &pShaderEffect, &pError);
	if (!pShaderEffect && pError)
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

		return pShaderEffect;
	}

    return pShaderEffect;
}

LPD3DXMESH LoadModel(const TCHAR* filename)
{
	LPD3DXMESH	ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, NULL, NULL, NULL, NULL, &ret)) )
	{
		OutputDebugString(L"모델 로딩 실패: ");
		OutputDebugString(filename);
		OutputDebugString(L"\n");
	}

	return ret;
}

HRESULT LoadTexture()
{
	return S_OK;
}
float gRotationY = 0;
void RenderScene()
{
	// 광원-뷰행렬을 만든다.
	D3DXMATRIXA16	matLightView;
	{
		D3DXVECTOR3 vEyePt( gWorldLightPosition.x, gWorldLightPosition.y, gWorldLightPosition.z);
		D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
		D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );

		D3DXMatrixLookAtLH( &matLightView, &vEyePt, &vLookatPt, &vUpVec);
	}

	// 광원-투영행렬을 만든다.
	D3DXMATRIXA16	matLightProjection;
	{
		D3DXMatrixPerspectiveFovLH(&matLightProjection, D3DX_PI / 4.0f, 1, 1, 1000);
	}

	// 뷰-투영행렬을 만든다.
	D3DXMATRIXA16 matViewProjection;
	{
		D3DXMATRIXA16 matView;
		D3DXVECTOR3 vEyePt( gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z );
		D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
		D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
		D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec);

		// 투영행렬을 만든다.
		D3DXMATRIXA16 matProjection;
		D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE );

		D3DXMatrixMultiply(&matViewProjection, &matView, &matProjection);
	}

	// 원환체의 월드행렬을 만든다.
	D3DXMATRIXA16 matTorusWorld;
	{
		// 프레임마다 0.4 도씩 회전 시킨다.
		gRotationY += 0.4f * PI / 180;
		if (gRotationY > 2*PI)
		{
			gRotationY -= 2 * PI;
		}
		D3DXMatrixRotationY(&matTorusWorld, gRotationY);
	}

	// 디스크의 월드행렬을 만든다.
	D3DXMATRIXA16 matDiscWorld;
	{
		D3DXMATRIXA16 matScale;
		D3DXMatrixScaling(&matScale, 2,2,2);

		D3DXMATRIXA16 matTrans;
		D3DXMatrixTranslation(&matTrans, 0, -40, 0);

		D3DXMatrixMultiply( &matDiscWorld, &matScale, &matTrans);
	}

	// 현재 하드웨어 백버퍼와 깊이버퍼
	LPDIRECT3DSURFACE9 pHWbackBuffer = NULL;
	LPDIRECT3DSURFACE9 pHWDepthStencilBuffer = NULL;
	gpD3DDevice->GetRenderTarget(0, &pHWbackBuffer);
	gpD3DDevice->GetDepthStencilSurface(&pHWDepthStencilBuffer);

	/////////////////////////////////////////////
	// 1. 그림자 만들기
	/////////////////////////////////////////////
	
	// 그림자맵의 렌더타깃과 깊이 버퍼를 사용한다.
	LPDIRECT3DSURFACE9 pShadowSurface = NULL;
	if ( SUCCEEDED( gpShadowRenderTarget->GetSurfaceLevel(0, &pShadowSurface)) )
	{
		gpD3DDevice->SetRenderTarget(0, pShadowSurface);
		pShadowSurface->Release();
		pShadowSurface = NULL;
	}

	//if ( SUCCEEDED( gpShadowDepthStencil->GetSurfaceLevel(0, &pShadowSurface)) )
	//{
	//	gpD3DDevice->SetDepthStencilSurface(pShadowSurface);
	//	pShadowSurface->Release();
	//	pShadowSurface = NULL;
	//}

	gpD3DDevice->SetDepthStencilSurface((IDirect3DSurface9*)gpShadowDepthStencil);

	

	// 프레임에 그렸던 그림자 정보를 지운다.
	gpD3DDevice->Clear( 0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), 0xFFFFFFFF, 1.0f, 0 );

	// 그림자 만들기 세이더 전역변수들을 설정

	D3DXHANDLE handle = gpCreateShadowShader->GetParameterByName(NULL, "gWorldMatrix");
	gpCreateShadowShader->SetMatrix(handle, &matTorusWorld);

	handle = gpCreateShadowShader->GetParameterByName(NULL, "gLightViewMatrix");
	gpCreateShadowShader->SetMatrix(handle, &matLightView);

	handle = gpCreateShadowShader->GetParameterByName(NULL, "gLightProjectionMatrix");
	gpCreateShadowShader->SetMatrix(handle, &matLightProjection);

	// 그림자 만들기 세이더 시작
	{
		UINT numPasses = 0;
		gpCreateShadowShader->Begin(&numPasses, NULL);
		{
			for (UINT i = 0; i < numPasses; ++i)
			{
				gpCreateShadowShader->BeginPass(i);
				{
					gpTorus->DrawSubset(0);
				}
				gpCreateShadowShader->EndPass();
			}			
		}
		gpCreateShadowShader->End();
	}

	/////////////////////////////////////////////
	// 2. 그림자 입히기
	/////////////////////////////////////////////

	// 하드웨어 백버퍼 / 깊이 버퍼를 사용한다.
	gpD3DDevice->SetRenderTarget(0, pHWbackBuffer);
	gpD3DDevice->SetDepthStencilSurface(pHWbackBuffer);

	pHWbackBuffer->Release();
	pHWbackBuffer = NULL;
	pHWDepthStencilBuffer->Release();
	pHWDepthStencilBuffer = NULL;

	// 그림자 입히기 세이더 전역변수들을 설정
	handle = gpApplyShadowShader->GetParameterByName(NULL, "gWorldMatrix");
	gpApplyShadowShader->SetMatrix(handle, &matTorusWorld);

	handle = gpApplyShadowShader->GetParameterByName(NULL, "gLightViewMatrix");
	gpApplyShadowShader->SetMatrix(handle, &matLightView);

	handle = gpApplyShadowShader->GetParameterByName(NULL, "gLightProjectionMatrix");
	gpApplyShadowShader->SetMatrix(handle, &matLightProjection);

	handle = gpApplyShadowShader->GetParameterByName(NULL, "gWorldLightPosition");
	gpApplyShadowShader->SetVector(handle, &gWorldLightPosition);

	handle = gpApplyShadowShader->GetParameterByName(NULL, "gObjectColor");
	gpApplyShadowShader->SetVector(handle, &gTorusColor);
	
	// 그림자 맵 전달.
	handle = gpApplyShadowShader->GetParameterByName(NULL, "ShadowMap_Tex");
	gpApplyShadowShader->SetTexture(handle, gpShadowRenderTarget);

	// 세이더 시작
	UINT numPasses = 0;
	gpApplyShadowShader->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			gpApplyShadowShader->BeginPass(i);
			{
				// 원환체를 그린다.
				gpTorus->DrawSubset(0);

				// 디스크를 그린다.
				handle = gpApplyShadowShader->GetParameterByName(NULL, "gWorldMatrix");
				gpApplyShadowShader->SetMatrix(handle, &matDiscWorld);

				handle = gpApplyShadowShader->GetParameterByName(NULL, "gObjectColor");
				gpApplyShadowShader->SetVector(handle, &gDiscColor);

				gpApplyShadowShader->CommitChanges();
				gpDisc->DrawSubset(0);
			}
			gpApplyShadowShader->EndPass();
		}
	}
	gpApplyShadowShader->End();





}

void RenderInfo()
{
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	
	RECT rect;
	rect.left = 5;
	rect.right = WIN_WIDTH / 3;
	rect.top = 5;
	rect.bottom = WIN_HEIGHT / 3;

	gpFont->DrawText(NULL, L"데모 프레임워크\n\nESC: 데모종료", -1, &rect, 0, fontColor);
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
	gpCreateShadowShader = LoadShader(L"CreateShadow.fx");
	if (!gpCreateShadowShader)
		return S_FALSE;

	gpApplyShadowShader = LoadShader(L"ApplyShadow.fx");
	if (!gpApplyShadowShader)
		return S_FALSE;

	gpDisc = LoadModel(L"disc.x");
	if (!gpDisc)
		return S_FALSE;

	gpTorus = LoadModel(L"torus.x");
	if (!gpDisc)
		return S_FALSE;

	//if (FAILED( LoadTexture() ))
	//	return S_FALSE;

	if (FAILED(D3DXCreateFont(pd3dDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
		(DEFAULT_PITCH|FF_DONTCARE), L"Arial", &gpFont )))
	{
		return S_FALSE;
	}

	const int shadowMapSize = 2048;
	if (FAILED(gpD3DDevice->CreateTexture( 
					shadowMapSize, 
					shadowMapSize, 
					1,
					D3DUSAGE_RENDERTARGET, 
					D3DFMT_R32F,
					D3DPOOL_DEFAULT, 
					&gpShadowRenderTarget, 
					NULL
					)) )
	{
		return S_FALSE;
	}

	if (FAILED(gpD3DDevice->CreateDepthStencilSurface(
					shadowMapSize,	// 깊이 버퍼 너비
					shadowMapSize,	// 깊이 버퍼 높이
					D3DFMT_D24X8,	// 텍스쳐 포맷, 깊이 24bit, 스텐실은 사용안함.
					D3DMULTISAMPLE_NONE,	// 안티알리어싱 사용안함.
					0,				// 멀티샘플 품질
					TRUE,			// 깊이 버퍼를 바꿀 때, 그 안에 있던 내용을 보존하지 않는다.
					(IDirect3DSurface9 **)&gpShadowDepthStencil, 
					NULL
					)) )
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
	gpCreateShadowShader->OnResetDevice();
	gpApplyShadowShader->OnResetDevice();
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

	D3DCOLOR bgColor = 0xFF0000FF;	// 파랑

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
	gpCreateShadowShader->OnLostDevice();
	gpApplyShadowShader->OnLostDevice();
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
	SAFE_RELEASE(gpCreateShadowShader);
	SAFE_RELEASE(gpDisc);
	SAFE_RELEASE(gpTorus);
	SAFE_RELEASE(gpShadowRenderTarget);
	SAFE_RELEASE(gpShadowDepthStencil);
	SAFE_RELEASE(gpTorus);

	SAFE_RELEASE(gpFont);
	

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


