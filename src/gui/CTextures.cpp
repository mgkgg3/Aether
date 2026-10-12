#include "CTextures.h"

#include "../../resources/textures/image_chat.h"
#include "../../resources/textures/image_code.h"
#include "../../resources/textures/image_folder.h"
#include "../../resources/textures/image_home.h"
#include "../../resources/textures/image_settings.h"
#include "../../resources/textures/image_spotify.h"

CTextures::CTextures()
{
    tChat     = nullptr;
    tCode     = nullptr;
    tFolder   = nullptr;
    tHome     = nullptr;
    tSettings = nullptr;
    tSpotify  = nullptr;
    tWaypoint = nullptr;
}

CTextures::~CTextures()
{
}

bool CTextures::Initialize(ID3D11Device* pDevice)
{
    D3DX11_IMAGE_LOAD_INFO info{};
    ID3DX11ThreadPump* pump = nullptr;

    D3DX11CreateShaderResourceViewFromMemory(pDevice, chat_bytes,     sizeof(chat_bytes),     &info, pump, &tChat,     0);
    D3DX11CreateShaderResourceViewFromMemory(pDevice, code_bytes,     sizeof(code_bytes),     &info, pump, &tCode,     0);
    D3DX11CreateShaderResourceViewFromMemory(pDevice, folder_bytes,   sizeof(folder_bytes),   &info, pump, &tFolder,   0);
    D3DX11CreateShaderResourceViewFromMemory(pDevice, home_bytes,     sizeof(home_bytes),     &info, pump, &tHome,     0);
    D3DX11CreateShaderResourceViewFromMemory(pDevice, settings_bytes, sizeof(settings_bytes), &info, pump, &tSettings, 0);
    D3DX11CreateShaderResourceViewFromMemory(pDevice, spotify_bytes,  sizeof(spotify_bytes),  &info, pump, &tSpotify,  0);


    D3DX11CreateShaderResourceViewFromMemory(pDevice, home_bytes,     sizeof(home_bytes),     &info, pump, &tWaypoint, 0);

    return true;
}

void CTextures::Shutdown()
{
    if (tChat)     { tChat->Release();     tChat     = nullptr; }
    if (tCode)     { tCode->Release();     tCode     = nullptr; }
    if (tFolder)   { tFolder->Release();   tFolder   = nullptr; }
    if (tHome)     { tHome->Release();     tHome     = nullptr; }
    if (tSettings) { tSettings->Release(); tSettings = nullptr; }
    if (tSpotify)  { tSpotify->Release();  tSpotify  = nullptr; }
    if (tWaypoint) { tWaypoint->Release(); tWaypoint = nullptr; }
}
