
#include "Avatar.h"

#include <windows.h>
#include <winhttp.h>
#include <d3d11.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "../../../thirdparty/stb/stb_image.h"

#include <debug/DebugConsole.h>

#pragma comment(lib, "winhttp.lib")

namespace avatar
{
	namespace
	{
		struct thumb_t
		{
			ID3D11ShaderResourceView* view = nullptr;
			int  w = 0;
			int  h = 0;
			bool loading = false;
		};

		ID3D11Device* g_device = nullptr;
		std::unordered_map<long long, thumb_t> g_thumbs;
		std::unordered_set<long long> g_pending;
		std::mutex g_mtx;

		std::unordered_map<long long, std::uint64_t> g_failed_until;
		constexpr std::uint64_t k_retry_ms = 30000;

		constexpr std::size_t k_max_thumbs = 64;

		std::string http_get(const std::wstring& host, const std::wstring& path)
		{
			std::string out;

			HINTERNET session = WinHttpOpen(L"aether/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				nullptr, nullptr, 0);
			if (!session)
				return out;

			HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
			if (!connect)
			{
				WinHttpCloseHandle(session);
				return out;
			}

			HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
			if (!request)
			{
				WinHttpCloseHandle(connect);
				WinHttpCloseHandle(session);
				return out;
			}

			if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
					WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
				WinHttpReceiveResponse(request, nullptr))
			{
				std::string body;
				for (;;)
				{
					DWORD available = 0;
					if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
						break;

					if (body.size() + available > (8u << 20))
						break;

					const std::size_t old = body.size();
					body.resize(old + available);

					DWORD read = 0;
					if (!WinHttpReadData(request, &body[old], available, &read))
						break;

					body.resize(old + read);
					if (read == 0)
						break;
				}
				out = std::move(body);
			}

			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			return out;
		}

		std::string extract_image_url(const std::string& json)
		{
			const std::size_t key = json.find("\"imageUrl\"");
			if (key == std::string::npos)
				return {};

			const std::size_t start = json.find("http", key);
			if (start == std::string::npos)
				return {};

			std::size_t end = json.find('"', start);
			if (end == std::string::npos)
				end = json.size();

			std::string url = json.substr(start, end - start);
			for (std::size_t pos = url.find("\\u0026"); pos != std::string::npos;
					pos = url.find("\\u0026", pos))
				url.replace(pos, 6, "&");
			return url;
		}

		void split_url(const std::string& url, std::wstring& host, std::wstring& path)
		{
			std::string rest = url;
			if (rest.rfind("https://", 0) == 0)      rest = rest.substr(8);
			else if (rest.rfind("http://", 0) == 0)  rest = rest.substr(7);

			const std::size_t slash = rest.find('/');
			const std::string host_part = slash == std::string::npos ? rest : rest.substr(0, slash);
			const std::string path_part = slash == std::string::npos ? "/" : rest.substr(slash);

			host.assign(host_part.begin(), host_part.end());
			path.assign(path_part.begin(), path_part.end());
		}

		long long thumb_key(long long user_id, bool head)
		{
			return user_id * 2 + (head ? 1 : 0);
		}

		void request(long long user_id, bool head)
		{
			if (user_id <= 0)
				return;

			const long long key = thumb_key(user_id, head);

			{
				std::lock_guard<std::mutex> lk(g_mtx);
				if (g_thumbs.find(key) != g_thumbs.end() ||
					g_pending.find(key) != g_pending.end())
					return;
				if (g_thumbs.size() >= k_max_thumbs)
					return;

				auto retry = g_failed_until.find(key);
				if (retry != g_failed_until.end())
				{
					if (GetTickCount64() < retry->second)
						return;
					g_failed_until.erase(retry);
				}
				if (g_failed_until.size() > 256)
					g_failed_until.clear();

				g_pending.insert(key);
				g_thumbs[key] = thumb_t{};
				g_thumbs[key].loading = true;
			}

			auto fetch = [user_id, key, head]
			{

				auto fail = [user_id, key](const char* why)
				{
					{
						std::lock_guard<std::mutex> lk(g_mtx);
						g_pending.erase(key);
						g_thumbs.erase(key);
						g_failed_until[key] = GetTickCount64() + k_retry_ms;
					}
					debug::log("avatar", "thumbnail failed uid=%lld (%s), retrying in %llu s",
						user_id, why, (unsigned long long)(k_retry_ms / 1000));
				};

				const std::wstring kind = head ? L"AvatarHeadShot" : L"AvatarBust";
				const std::string json = http_get(L"thumbnails.roblox.com",
					L"/v1/users/avatar?userIds=" + std::to_wstring(user_id) +
					L"&type=" + kind + L"&size=420x420&format=Png&isCircular=false");

				const std::string url = extract_image_url(json);
				if (url.empty())
				{
					fail(json.empty() ? "the thumbnail request returned nothing"
					                  : "the response has no imageUrl");
					return;
				}

				std::wstring host, path;
				split_url(url, host, path);
				if (host.empty())
				{
					fail("the image URL has no host");
					return;
				}

				const std::string png = http_get(host, path);
				if (png.empty())
				{
					fail("the image itself did not download");
					return;
				}

				int w = 0, h = 0, channels = 0;
				unsigned char* pixels = stbi_load_from_memory(
					(const unsigned char*)png.data(), (int)png.size(), &w, &h, &channels, 4);
				if (!pixels || w <= 0 || h <= 0)
				{
					if (pixels) stbi_image_free(pixels);
					fail("the PNG did not decode");
					return;
				}

				ID3D11Device* device = nullptr;
				{
					std::lock_guard<std::mutex> lk(g_mtx);
					device = g_device;
					if (device)
						device->AddRef();
				}

				if (!device)
				{
					stbi_image_free(pixels);
					fail("no D3D11 device");
					return;
				}

				D3D11_TEXTURE2D_DESC desc{};
				desc.Width = (UINT)w;
				desc.Height = (UINT)h;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				D3D11_SUBRESOURCE_DATA data{};
				data.pSysMem = pixels;
				data.SysMemPitch = (UINT)w * 4;

				ID3D11Texture2D* texture = nullptr;
				ID3D11ShaderResourceView* view = nullptr;
				if (SUCCEEDED(device->CreateTexture2D(&desc, &data, &texture)) && texture)
				{
					if (FAILED(device->CreateShaderResourceView(texture, nullptr, &view)))
						view = nullptr;
					texture->Release();
				}

				stbi_image_free(pixels);
				device->Release();

				if (!view)
					debug::log("avatar", "thumbnail uid=%lld: the device would not take the texture", user_id);
				else
					debug::log("avatar", "thumbnail ready uid=%lld (%dx%d)", user_id, w, h);

				{
					std::lock_guard<std::mutex> lk(g_mtx);
					g_pending.erase(key);

					auto it = g_thumbs.find(key);
					if (it == g_thumbs.end())
					{

						if (view)
							view->Release();
						return;
					}

					if (view)
					{
						if (it->second.view)
							it->second.view->Release();
						it->second.view = view;
						it->second.w = w;
						it->second.h = h;
					}
					it->second.loading = false;
				}
			};

			try
			{
				std::thread(fetch).detach();
			}
			catch (...)
			{
				std::lock_guard<std::mutex> lk(g_mtx);
				g_pending.erase(key);
				g_thumbs.erase(key);
			}
		}
	}

	void init(ID3D11Device* device)
	{
		g_device = device;
	}

	void shutdown()
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		for (auto& entry : g_thumbs)
		{
			if (entry.second.view)
				entry.second.view->Release();
		}
		g_thumbs.clear();
		g_pending.clear();
		g_failed_until.clear();
		g_device = nullptr;
	}

	static bool draw_kind(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max,
		ImU32 tint, float rounding, bool head)
	{
		if (!dl || user_id <= 0)
			return false;

		request(user_id, head);

		ID3D11ShaderResourceView* view = nullptr;
		{
			std::lock_guard<std::mutex> lk(g_mtx);
			auto it = g_thumbs.find(thumb_key(user_id, head));
			if (it != g_thumbs.end())
				view = it->second.view;
		}

		if (!view || max.x <= min.x || max.y <= min.y)
			return false;

		{
			const float rw = max.x - min.x;
			const float rh = max.y - min.y;
			const float side = (rw < rh) ? rw : rh;
			if (side < rw || side < rh)
			{
				const ImVec2 c((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
				min = ImVec2(c.x - side * 0.5f, c.y - side * 0.5f);
				max = ImVec2(c.x + side * 0.5f, c.y + side * 0.5f);
			}
		}

		dl->AddImageRounded((ImTextureID)(intptr_t)view, min, max, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f),
			tint, rounding);
		return true;
	}

	bool draw(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint)
	{
		return draw_kind(dl, user_id, min, max, tint, 2.f, false);
	}

	bool draw_rounded(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint, float rounding)
	{
		return draw_kind(dl, user_id, min, max, tint, rounding, false);
	}

	bool draw_head(ImDrawList* dl, long long user_id, ImVec2 min, ImVec2 max, ImU32 tint, float rounding)
	{
		return draw_kind(dl, user_id, min, max, tint, rounding, true);
	}
}
