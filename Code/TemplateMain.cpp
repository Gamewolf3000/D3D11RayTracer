//--------------------------------------------------------------------------------------
// File: TemplateMain.cpp
//
// BTH-D3D-Template
//
// Copyright (c) Stefan Petersson 2013. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"

#include "ComputeHelp.h"
#include "D3D11Timer.h"
#include "ModelImporter.h"

/*	DirectXTex library - for usage info, see http://directxtex.codeplex.com/
	
	Usage example (may not be the "correct" way, I just wrote it in a hurry):

	DirectX::ScratchImage img;
	DirectX::TexMetadata meta;
	ID3D11ShaderResourceView* srv = nullptr;
	if(SUCCEEDED(hr = DirectX::LoadFromDDSFile(_T("C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Samples\\Media\\Dwarf\\Armor.dds"), 0, &meta, img)))
	{
		//img loaded OK
		if(SUCCEEDED(hr = DirectX::CreateShaderResourceView(g_Device, img.GetImages(), img.GetImageCount(), meta, &srv)))
		{
			//srv created OK
		}
	}
*/
#include <DirectXTex.h>
#include <DirectXMath.h>
#include <iostream>

#if defined( DEBUG ) || defined( _DEBUG )
#pragma comment(lib, "DirectXTexD.lib")
#else
#pragma comment(lib, "DirectXTex.lib")
#endif

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE				g_hInst					= NULL;  
HWND					g_hWnd					= NULL;

IDXGISwapChain*         g_SwapChain				= NULL;
ID3D11Device*			g_Device				= NULL;
ID3D11DeviceContext*	g_DeviceContext			= NULL;

ID3D11UnorderedAccessView*  g_BackBufferUAV		= NULL;  // compute output

ComputeWrap*			g_ComputeSys			= NULL;
ComputeShader*			g_ComputeShader			= NULL;

D3D11Timer*				g_Timer					= NULL;

int g_Width, g_Height;
#define NROFTESTS 100000000
double deltaTimesInSeconds[NROFTESTS];
int counter = 0;
//--------------------------------------------------------------------------------------
// Global Variables	Project Specific
//--------------------------------------------------------------------------------------
struct GeometryData
{
	//Circle!
	DirectX::XMFLOAT4 ballOriginAndRadius[4] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.25f) };
	DirectX::XMFLOAT4 ballColour[4] = { DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };
	DirectX::XMINT4 nrOfBalls_nrOfWalls_nrOfTriangles_nrOfMeshPoints = DirectX::XMINT4(0, 0, 0, 0);

	DirectX::XMFLOAT4 PlaneOriginAndEmpty[6] = {DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)};
	DirectX::XMFLOAT4 PlaneNormalAndEmpty[6] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) };
	DirectX::XMFLOAT4 wallColour[6] = { DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) };

	DirectX::XMFLOAT4 TrianglePointsAndEmpty[8][3] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) };
	DirectX::XMFLOAT4 TriangleColour[8][3] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) };

};

struct PerFrameStruct //"Camera and viewing needed resources"
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	DirectX::XMFLOAT4 cameraPositionAndNrOfLights;

	DirectX::XMFLOAT4 PointLightsPositionAndRange[10] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) };
	DirectX::XMFLOAT4 PointLightsColour[10] = { DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) };
};

struct LightPositionAndDirection
{
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR direction;
	float timeToBounce;
};

float calculateBounceTime(DirectX::XMVECTOR position, DirectX::XMVECTOR direction)
{
	DirectX::XMFLOAT4 floatPosition, floatDirection;
	DirectX::XMStoreFloat4(&floatPosition, position);
	DirectX::XMStoreFloat4(&floatDirection, direction);
	float timeMin = 1500; float timeToTest = 0.0;
	/*Calculate the time to each wall CHECK FOR ZERO!*/
	if (floatDirection.x != 0.0)
	{
		timeToTest = (-49 - floatPosition.x) / floatDirection.x;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
		timeToTest = (49 - floatPosition.x) / floatDirection.x;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
	}
	if(floatDirection.y != 0.0)
	{
		timeToTest = (-49 - floatPosition.y) / floatDirection.y;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
		timeToTest = (49 - floatPosition.y) / floatDirection.y;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
	}
	if (floatDirection.z != 0.0)
	{
		timeToTest = (-49 - floatPosition.z) / floatDirection.z;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
		timeToTest = (49 - floatPosition.z) / floatDirection.z;
		if (timeMin > timeToTest && timeToTest > 0.0)
			timeMin = timeToTest;
	}

	return timeMin;
}

DirectX::XMVECTOR reflect(DirectX::XMVECTOR position, DirectX::XMVECTOR direction)
{
	DirectX::XMFLOAT4 floatPosition;
	DirectX::XMStoreFloat4(&floatPosition, position);
	DirectX::XMVECTOR reflection;

	int closestWall = 1;
	float distance;
	float distanceToWall;
	
	distanceToWall = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(50, 0.0f, 0.0f, 0.0f))));
	
	distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(-50, 0.0f, 0.0f, 0.0f))));
	if (distance < distanceToWall)
	{
		closestWall = 2;
		distanceToWall = distance;
	}

	distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(0, 50.0f, 0.0f, 0.0f))));
	if (distance < distanceToWall)
	{
		closestWall = 3;
		distanceToWall = distance;
	}

	distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(0, -50.0f, 0.0f, 0.0f))));
	if (distance < distanceToWall)
	{
		closestWall = 4;
		distanceToWall = distance;
	}

	distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(0, 0.0f, 50.0f, 0.0f))));
	if (distance < distanceToWall)
	{
		closestWall = 5;
		distanceToWall = distance;
	}


	distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(position, DirectX::XMVectorSet(0, 0.0f, -50.0f, 0.0f))));
	if (distance < distanceToWall)
	{
		closestWall = 6;
		distanceToWall = distance;
	}




	switch (closestWall)
	{
	case 1:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(-1.0, 0.0, 0.0, 0.0));
		break;
	case 2:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(1.0, 0.0, 0.0, 0.0));
		break;
	case 3:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(0.0, -1.0, 0.0, 0.0));
		break;

	case 4:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(0.0, 1.0, 0.0, 0.0));
		break;

	case 5:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(0.0, 0.0, -1.0, 0.0));
		break;

	case 6:
		reflection = DirectX::XMVector3Reflect(direction, DirectX::XMVectorSet(0.0, 0.0, 1.0, 0.0));
		break;

	default:
		reflection = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return reflection;
}

ID3D11Buffer* pg_ConstBufferData;
ID3D11Buffer* pg_ConstBufferViewProj;
ModelImporter* pg_objLoader;

ID3D11SamplerState* sampler = nullptr;

DirectX::XMVECTOR pg_Rotation = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
DirectX::XMVECTOR pg_LookAt = DirectX::XMVector4Normalize(DirectX::XMVectorSet(0.0f, .0f, 1.0f, 0.0f));
DirectX::XMVECTOR pg_CamPos = DirectX::XMVectorSet(0.0f, 0.0f, -20.0f, 1.0f);
DirectX::XMVECTOR pg_Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
DirectX::XMVECTOR pg_Right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

UINT nrOFLights = 2;
DirectX::XMVECTOR PointLightsPosition[10] = { DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f) };
DirectX::XMVECTOR PointLightsColour[10] = { DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f) };

LightPositionAndDirection lights[10];

/*Not used as of now*/
DirectX::XMVECTOR pg_CamDir = DirectX::XMVector4Normalize(DirectX::XMVectorSet(-20.0f, 0.0f, 20.0f, 0.0f));

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT             InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT				Render(float deltaTime);
HRESULT				Update(float deltaTime);

char* FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel)
{
	if(featureLevel == D3D_FEATURE_LEVEL_11_0)
		return "11.0";
	if(featureLevel == D3D_FEATURE_LEVEL_10_1)
		return "10.1";
	if(featureLevel == D3D_FEATURE_LEVEL_10_0)
		return "10.0";

	return "Unknown";
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT Init()
{
	HRESULT hr = S_OK;;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	g_Width = rc.right - rc.left;;
	g_Height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverType;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_Width;
	sd.BufferDesc.Height = g_Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevelsToTry[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL initiatedFeatureLevel;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			driverType,
			NULL,
			createDeviceFlags,
			featureLevelsToTry,
			ARRAYSIZE(featureLevelsToTry),
			D3D11_SDK_VERSION,
			&sd,
			&g_SwapChain,
			&g_Device,
			&initiatedFeatureLevel,
			&g_DeviceContext);

		if (SUCCEEDED(hr))
		{
			char title[256];
			sprintf_s(
				title,
				sizeof(title),
				"BTH - Direct3D 11.0 Template | Direct3D 11.0 device initiated with Direct3D %s feature level",
				FeatureLevelToString(initiatedFeatureLevel)
			);
			SetWindowTextA(g_hWnd, title);

			break;
		}
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer;
	hr = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	// create shader unordered access view on back buffer for compute shader to write into texture
	hr = g_Device->CreateUnorderedAccessView(pBackBuffer, NULL, &g_BackBufferUAV);

	//create helper sys and compute shader instance
	g_ComputeSys = new ComputeWrap(g_Device, g_DeviceContext);
	g_ComputeShader = g_ComputeSys->CreateComputeShader(_T("../Shaders/BasicCompute.fx"), NULL, "main", NULL);
	g_Timer = new D3D11Timer(g_Device, g_DeviceContext);

	// Load obj model
	pg_objLoader = new ModelImporter(g_Device, "Cube.obj");

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	g_Device->CreateSamplerState(&samplerDesc, &sampler);

	g_DeviceContext->CSSetSamplers(0, 1, &sampler);

	g_DeviceContext->CSSetShaderResources(0, 1, &pg_objLoader->vertexView);
	g_DeviceContext->CSSetShaderResources(1, 1, &pg_objLoader->texCoordsView);
	g_DeviceContext->CSSetShaderResources(2, 1, pg_objLoader->getTexture());
	g_DeviceContext->CSSetUnorderedAccessViews(1, 1, &pg_objLoader->uavVertex, NULL);
	g_DeviceContext->CSSetUnorderedAccessViews(2, 1, &pg_objLoader->uavTexCoords, NULL);

	//PG creation
	GeometryData temp;
	temp.ballOriginAndRadius[0].x = 0.0f;
	temp.ballOriginAndRadius[0].y = -20.0f;
	temp.ballOriginAndRadius[0].z = 0.f;
	temp.ballOriginAndRadius[0].w = 5.0f;
	temp.ballColour[0] = DirectX::XMFLOAT4(1.0f, 215.0f / 255.0f, 0.0f, 1.0f);

	temp.ballOriginAndRadius[1].x = 0.0f;
	temp.ballOriginAndRadius[1].y = 0.0f;
	temp.ballOriginAndRadius[1].z = 20.f;
	temp.ballOriginAndRadius[1].w = 10.0f;
	temp.ballColour[1] = DirectX::XMFLOAT4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

	temp.ballOriginAndRadius[2].x = 0.0f;
	temp.ballOriginAndRadius[2].y = 20.0f;
	temp.ballOriginAndRadius[2].z = 0.f;
	temp.ballOriginAndRadius[2].w = 1.5f;
	temp.ballColour[2] = DirectX::XMFLOAT4(1.f, 0.0f, 0.f, 1.0f);

	temp.ballOriginAndRadius[3].x = -20.0f;
	temp.ballOriginAndRadius[3].y = 0.0f;
	temp.ballOriginAndRadius[3].z = 0.f;
	temp.ballOriginAndRadius[3].w = 7.5f;
	temp.ballColour[3] = DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);

	temp.PlaneOriginAndEmpty[0] = DirectX::XMFLOAT4(50.0f, 0.0f, 0.0f, 1.0f);
	temp.PlaneNormalAndEmpty[0] = DirectX::XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	temp.wallColour[0] = DirectX::XMFLOAT4(0.5f, 0.0f, 0.0f, 1.f);

	temp.PlaneOriginAndEmpty[1] = DirectX::XMFLOAT4(-50.0f, 0.0f, 0.0f, 1.0f);
	temp.PlaneNormalAndEmpty[1] = DirectX::XMFLOAT4(1.0, 0.0f, 0.0f, 0.0f);
	temp.wallColour[1] = DirectX::XMFLOAT4(0.55f, 0.0f, 0.0f, 1.f);

	temp.PlaneOriginAndEmpty[2] = DirectX::XMFLOAT4(0.0f, 50.0f, 0.0f, 1.0f);
	temp.PlaneNormalAndEmpty[2] = DirectX::XMFLOAT4(.0f, -1.0f, .0f, 0.0f);
	temp.wallColour[2] = DirectX::XMFLOAT4(0.f, 0.55f, 0.f, 1.f);

	temp.PlaneOriginAndEmpty[3] = DirectX::XMFLOAT4(0.0f, -50.0f, 0.0f, 1.0f);
	temp.PlaneNormalAndEmpty[3] = DirectX::XMFLOAT4(.0f, 1.0f, .0f, 0.0f);
	temp.wallColour[3] = DirectX::XMFLOAT4(0.f, 0.55f, 0.f, 1.f);

	temp.PlaneOriginAndEmpty[4] = DirectX::XMFLOAT4(0.0f, 0.0f, 50.f, 1.0f);
	temp.PlaneNormalAndEmpty[4] = DirectX::XMFLOAT4(.0f, .0f, -1.0f, 0.0f);
	temp.wallColour[4] = DirectX::XMFLOAT4(0.f, 0.f, 0.5f, 1.f);

	temp.PlaneOriginAndEmpty[5] = DirectX::XMFLOAT4(0.0f, 0.0f, -50.0f, 1.0f);
	temp.PlaneNormalAndEmpty[5] = DirectX::XMFLOAT4(.0f, .0f, 1.0f, 0.0f);
	temp.wallColour[5] = DirectX::XMFLOAT4(0.f, 0.f, 0.5f, 1.f);

	temp.TrianglePointsAndEmpty[0][2] = DirectX::XMFLOAT4(15.0f, 50.0f, 15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[0][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[0][0] = DirectX::XMFLOAT4(-15.0f, 50.0f, 15.0f, 1.0f);
	temp.TriangleColour[0][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[0][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[0][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[1][2] = DirectX::XMFLOAT4(-15.0f, 50.0f, -15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[1][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[1][0] = DirectX::XMFLOAT4(15.0f, 50.0f, -15.0f, 1.0f);
	temp.TriangleColour[1][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[1][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[1][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[2][2] = DirectX::XMFLOAT4(15.0f, 50.0f, -15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[2][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[2][0] = DirectX::XMFLOAT4(15.0f, 50.0f, 15.0f, 1.0f);
	temp.TriangleColour[2][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[2][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[2][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[3][2] = DirectX::XMFLOAT4(-15.0f, 50.0f, 15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[3][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[3][0] = DirectX::XMFLOAT4(-15.0f, 50.0f, -15.0f, 1.0f);
	temp.TriangleColour[3][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[3][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[3][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);


	temp.TrianglePointsAndEmpty[4][2] = DirectX::XMFLOAT4(-15.0f, -50.0f, -15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[4][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[4][0] = DirectX::XMFLOAT4(15.0f, -50.0f, -15.0f, 1.0f);
	temp.TriangleColour[4][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[4][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[4][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[5][2] = DirectX::XMFLOAT4(15.0f, -50.0f, 15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[5][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[5][0] = DirectX::XMFLOAT4(-15.0f, -50.0f, 15.0f, 1.0f);
	temp.TriangleColour[5][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[5][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[5][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[6][2] = DirectX::XMFLOAT4(-15.0f, -50.0f, 15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[6][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[6][0] = DirectX::XMFLOAT4(-15.0f, -50.0f, -15.0f, 1.0f);
	temp.TriangleColour[6][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[6][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[6][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.TrianglePointsAndEmpty[7][2] = DirectX::XMFLOAT4(15.0f, -50.0f, -15.0f, 1.0f);
	temp.TrianglePointsAndEmpty[7][1] = DirectX::XMFLOAT4(0.0f, 25.0f, 0.0f, 0.0f);
	temp.TrianglePointsAndEmpty[7][0] = DirectX::XMFLOAT4(15.0f, -50.0f, 15.0f, 1.0f);
	temp.TriangleColour[7][0] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	temp.TriangleColour[7][1] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	temp.TriangleColour[7][2] = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	temp.nrOfBalls_nrOfWalls_nrOfTriangles_nrOfMeshPoints.x = 4;
	
	temp.nrOfBalls_nrOfWalls_nrOfTriangles_nrOfMeshPoints.y = 6;

	temp.nrOfBalls_nrOfWalls_nrOfTriangles_nrOfMeshPoints.z = 4;

	//temp.nrOfBalls_nrOfWalls_nrOfTriangles_nrOfMeshPoints.w = pg_objLoader->getNrOfIndices();
	
	pg_ConstBufferData = g_ComputeSys->CreateConstantBuffer(sizeof(temp), &temp, "Circle");

	for (int i = 0; i < nrOFLights; i++)
	{
		lights[i].position = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		lights[i].direction = DirectX::XMVector3Normalize(DirectX::XMVectorSet(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100, 0.0f));
		lights[i].timeToBounce = calculateBounceTime(lights[0].position, lights[0].direction);
	}
	g_DeviceContext->CSSetConstantBuffers(0, 1, &pg_ConstBufferData);

	return S_OK;
}

HRESULT Update(float deltaTime)
{
	if (GetAsyncKeyState('W'))
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(25 * deltaTime);
		pg_CamPos = DirectX::XMVectorMultiplyAdd(s, pg_LookAt, pg_CamPos);

	}
	if (GetAsyncKeyState('S'))
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(25 * -deltaTime);
		pg_CamPos = DirectX::XMVectorMultiplyAdd(s, pg_LookAt, pg_CamPos);
	}
	if (GetAsyncKeyState('A'))
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(25 * -deltaTime);
		pg_CamPos = DirectX::XMVectorMultiplyAdd(s, pg_Right, pg_CamPos);
	}
	if (GetAsyncKeyState('D'))
	{
		DirectX::XMVECTOR s = DirectX::XMVectorReplicate(25 * deltaTime);
		pg_CamPos = DirectX::XMVectorMultiplyAdd(s, pg_Right, pg_CamPos);
	}
	if (GetAsyncKeyState('Q'))
	{
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(-deltaTime * 50));

		pg_Right = DirectX::XMVector3TransformNormal(pg_Right, R);
		pg_Up = DirectX::XMVector3TransformNormal(pg_Up, R);
		pg_LookAt = DirectX::XMVector3TransformNormal(pg_LookAt, R);
		
	}
	if (GetAsyncKeyState('E'))
	{

		DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(deltaTime * 50));

		pg_Right = DirectX::XMVector3TransformNormal(pg_Right, R);
		pg_Up = DirectX::XMVector3TransformNormal(pg_Up, R);
		pg_LookAt = DirectX::XMVector3TransformNormal(pg_LookAt, R);
	}

	if (DirectX::XMVectorGetX(pg_CamPos) > 50.0f)
		pg_CamPos = DirectX::XMVectorSetX(pg_CamPos, -49.0f);
	if (DirectX::XMVectorGetX(pg_CamPos) < -50.0f)
		pg_CamPos = DirectX::XMVectorSetX(pg_CamPos, 49.0f);
	if (DirectX::XMVectorGetY(pg_CamPos) > 50.0f)
		pg_CamPos = DirectX::XMVectorSetY(pg_CamPos, -49.0f);
	if (DirectX::XMVectorGetY(pg_CamPos) < -50.0f)
		pg_CamPos = DirectX::XMVectorSetY(pg_CamPos, 49.0f);
	if (DirectX::XMVectorGetZ(pg_CamPos) > 50.0f)
		pg_CamPos = DirectX::XMVectorSetZ(pg_CamPos, -49.0f);
	if (DirectX::XMVectorGetZ(pg_CamPos) < -50.0f)
		pg_CamPos = DirectX::XMVectorSetZ(pg_CamPos, 49.0f);
	
	pg_LookAt = DirectX::XMVector3Normalize(pg_LookAt);
	pg_Up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(pg_LookAt, pg_Right));
	pg_Right = DirectX::XMVector3Cross(pg_Up, pg_LookAt);

	float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(pg_CamPos, pg_Right));
	float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(pg_CamPos, pg_Up));
	float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(pg_CamPos, pg_LookAt));



	PerFrameStruct temp;
	DirectX::XMFLOAT4X4 temp2;

	temp2(0, 0) = DirectX::XMVectorGetX(pg_Right);
	temp2(1, 0) = DirectX::XMVectorGetY(pg_Right);
	temp2(2, 0) = DirectX::XMVectorGetZ(pg_Right);
	temp2(3, 0) = x;

	temp2(0, 1) = DirectX::XMVectorGetX(pg_Up);
	temp2(1, 1) = DirectX::XMVectorGetY(pg_Up);
	temp2(2, 1) = DirectX::XMVectorGetZ(pg_Up);
	temp2(3, 1) = y;

	temp2(0, 2) = DirectX::XMVectorGetX(pg_LookAt);
	temp2(1, 2) = DirectX::XMVectorGetY(pg_LookAt);
	temp2(2, 2) = DirectX::XMVectorGetZ(pg_LookAt);
	temp2(3, 2) = z;

	temp2(0, 3) = 0;
	temp2(1, 3) = 0;
	temp2(2, 3) = 0;
	temp2(3, 3) = 1;
	
	DirectX::XMStoreFloat4x4(&temp.viewMatrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&temp2))));
	DirectX::XMStoreFloat4x4(&temp.projMatrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1, 0.1f, 10.0f))));
	DirectX::XMStoreFloat4(&temp.cameraPositionAndNrOfLights, pg_CamPos);
	temp.cameraPositionAndNrOfLights.w = (float)nrOFLights;

	for (int i = 0; i < nrOFLights; i++)
	{
		DirectX::XMStoreFloat4(&temp.PointLightsPositionAndRange[i], lights[i].position);
		temp.PointLightsPositionAndRange[i].w = 2500;
		temp.PointLightsColour[i] = DirectX::XMFLOAT4(.25f, .25f, .25f, 0.0f);
		if (lights[i].timeToBounce > 15*deltaTime)
		{
			lights[i].position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(15*deltaTime), lights[i].direction, lights[i].position);
			lights[i].timeToBounce -= 15*deltaTime;
		}
		else
		{
			/*Reflect*/
			lights[i].direction = reflect(lights[i].position, lights[i].direction);
			lights[i].timeToBounce = calculateBounceTime(lights[i].position, lights[i].direction);
		}
	}

	pg_ConstBufferViewProj = g_ComputeSys->CreateConstantBuffer(sizeof(temp), &temp, "view");

	return S_OK;
}

HRESULT Render(float deltaTime)
{
	ID3D11UnorderedAccessView* uav[] = { g_BackBufferUAV };
	g_DeviceContext->CSSetUnorderedAccessViews(0, 1, uav, NULL);

	g_ComputeShader->Set();

	g_DeviceContext->CSSetConstantBuffers(1, 1, &pg_ConstBufferViewProj);
	g_Timer->Start();
	g_DeviceContext->Dispatch(30, 30, 1 );
	g_ComputeShader->Unset();
	g_Timer->Stop();


	if(FAILED(g_SwapChain->Present( 0, 0 )))
		return E_FAIL;

	char title[256];
	sprintf_s(
		title,
		sizeof(title),
		"BTH - DirectCompute DEMO - Dispatch time: %f",
		g_Timer->GetTime()
	);
	deltaTimesInSeconds[counter] = g_Timer->GetTime();
	SetWindowTextA(g_hWnd, title);

	pg_ConstBufferViewProj->Release();

	return S_OK;
}

void dummyFunction(int &a)
{
	a++;
	a++;
	a--;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		return 0;

	if( FAILED( Init() ) )
		return 0;

	__int64 cntsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cntsPerSec);
	float secsPerCnt = 1.0f / (float)cntsPerSec;

	__int64 prevTimeStamp = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&prevTimeStamp);

	// Main message loop
	MSG msg = {0};
	while(WM_QUIT != msg.message && counter < NROFTESTS)
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			__int64 currTimeStamp = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTimeStamp);
			float dt = (currTimeStamp - prevTimeStamp) * secsPerCnt;

			//render
			Update(dt);
			Render(dt);
			counter++;

			prevTimeStamp = currTimeStamp;
		}
	}
	ofstream dataSave;
	dataSave.open("Time.txt");
	double time = 0.0;

	for (int i = 0; i < counter; i++)
	{
		time += deltaTimesInSeconds[i];
		dataSave << std::to_string(deltaTimesInSeconds[i]) << std::endl;
	}
	time /= counter;
	dataSave << time;



	return (int) msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = 0;
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = _T("BTH_D3D_Template");
	wcex.hIconSm        = 0;
	if( !RegisterClassEx(&wcex) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance; 
	RECT rc = { 0, 0, 960, 960 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	
	if(!(g_hWnd = CreateWindow(
							_T("BTH_D3D_Template"),
							_T("BTH - Direct3D 11.0 Template"),
							WS_OVERLAPPEDWINDOW,
							0,
							0,
							rc.right - rc.left,
							rc.bottom - rc.top,
							NULL,
							NULL,
							hInstance,
							NULL)))
	{
		return E_FAIL;
	}

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:

		switch(wParam)
		{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}