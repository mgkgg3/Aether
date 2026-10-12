#pragma once


#include <d3d11.h>

#if defined(_WIN64)
#pragma comment(lib, "d3dx11.lib")
#endif
#include "../../thirdparty/Microsoft DirectX SDK/Include/D3DX11tex.h"

class CTextures
{
public:
	CTextures();
	~CTextures();

	bool Initialize(ID3D11Device* pDevice);
	void Shutdown();

public:
	ID3D11ShaderResourceView* tChat;
	ID3D11ShaderResourceView* tCode;
	ID3D11ShaderResourceView* tFolder;
	ID3D11ShaderResourceView* tHome;
	ID3D11ShaderResourceView* tSettings;
	ID3D11ShaderResourceView* tSpotify;

	ID3D11ShaderResourceView* tWaypoint;
};
