#pragma comment(lib, "cryptlib.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3d9.lib")

#ifdef _DEBUG
#pragma comment(lib, "libcurl_a_debug.lib")
#else
#pragma comment(lib, "libcurl_a.lib")
#endif

#define CURL_STATICLIB

#define _CRT_SECURE_NO_WARNINGS

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_D3D9_IMPLEMENTATION

#include "json.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "nuklear.h"
#include "demo/d3d9/nuklear_d3d9.h"

#include <filesystem>
#include <windows.h>
#include <curl.h>
#include <base64.h>
#include <VersionHelpers.h>
#include <dwmapi.h>

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <chrono>
#include <ctime>  
#include <vector>
#include <future>

const std::string g_sz_username{ "" };
const std::string g_sz_password{ "" };
const std::string g_sz_token{ "wc8j_yBJd20zOmx0" };
const std::string g_sz_client_version{ "1.9.1" };
const std::string g_sz_client_unique_key{ "935d5e6541b34439" };

const std::string g_sz_tidal_user_agent{ "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36" };
const std::string g_sz_tidal_url{ "https://api.tidalhifi.com" };
const std::string g_sz_tidal_login_url{ "https://api.tidalhifi.com/v1/login/username" };
const std::string g_sz_tidal_stream_url{ "https://sp-pr-cf.audio.tidal.com" };

/* C:\Users\main\Desktop\16109913694314243.jpg (1/18/2021 9:37:20 AM)
   StartOffset(h): 00000000, EndOffset(h): 0001E4E3, Length(h): 0001E4E4 */

// put your own image
unsigned char rawData[] = { 0x00 };

const char* GetLastestError() {

	auto lastErrorID = GetLastError();
	if (lastErrorID == 0) {

		return "No error code received.";
	}

	char* error = NULL;
	auto size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lastErrorID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&error), 0, NULL);

	if (size == 0) {

		return "Format message failed.";
	}

	return error;
}

void logg(const std::string_view& msg) {
	
	std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	char str[26] = {};
	ctime_s(str, 26, &end_time);

	std::string msgg = "[";
	msgg.append(str);
	msgg.erase(1, 4);
	msgg.erase(msgg.size() - 6, msgg.size());
	msgg.append("]: ");
	msgg.append(msg);
	msgg.append("\n");

	static std::ofstream ostream;
	ostream.open("test.log", std::ios_base::app | std::ios_base::out);

	if (ostream.is_open()) {

		ostream << msgg;
		ostream.close();
	}

#ifdef _DEBUG

	static bool l_b_console = AllocConsole();
	if(l_b_console) {

		static FILE* ConOut{};
		static auto e1 = freopen_s(&ConOut, "CONOUT$", "w", stdout);
		static auto e2 = freopen_s(&ConOut, "CONOUT$", "w", stderr);
		static auto e3 = freopen_s(&ConOut, "CONIN$", "r", stdin);
	}

	std::cout << msgg;

#endif
}

void logg(const float& msg) {

	logg(std::to_string(msg));
}

void logg(const int& msg) {

	logg(std::to_string(msg));
}

void logg(const bool& msg) {

	logg(std::to_string(msg));
}

struct MemoryStruct {

	char* memory;
	size_t size;
} mem;

size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {

	auto realsize = size * nmemb;
	auto mem = reinterpret_cast<MemoryStruct*>(userp);

	auto allocMem = realloc(mem->memory, mem->size + realsize + 1);
	if (allocMem == nullptr) {

		return 0;
	}

	mem->memory = static_cast<char*>(allocMem);

	memcpy(&mem->memory[mem->size], contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {

	size_t written = fwrite(ptr, size, nmemb, stream);

	return written;
}

enum Method {

	POST,
	GET
};

struct VRequest {

	Method m_sz_method;
	std::string m_sz_url;
	std::string m_sz_path;
	std::string m_sz_optional;
	std::string m_sz_response;
	std::vector<std::string> m_sz_header;
};

struct VAlbum {

	int m_i_id;
	std::string m_sz_title;
	std::string m_sz_cover;
};

struct VArtist {

	int m_i_id;
	std::string m_sz_name;
};

struct VSong {

	int m_i_id;
	std::string m_sz_title;
	int m_i_duration;
	float m_fl_replay_gain;
	float m_fl_peak;
	bool m_b_allow_streaming;
	bool m_b_stream_ready;
	bool m_b_premium_streaming_only;
	int m_i_track_number;
	int m_i_version;
	int m_i_popularity;
	std::string m_sz_copyright;
	std::string m_sz_url;
	bool m_b_explicit;
	std::string m_sz_audio_quality;
	VAlbum m_sz_album;
	VAlbum m_sz_artist;
	std::string codec;
	//std::string m_sz_audio_modes;
};

class VUrl {

private:

	CURL* handle;
	std::string m_sz_user_agent;

	bool m_b_logged_in;
	int m_i_user_id;
	std::string m_sz_session_id;
	std::string m_sz_country_code;

	void SendRequest(VRequest& p_o_request) {

		mem.memory = static_cast<char*>(malloc(1));
		mem.size = 0;

#ifdef _DEBUG
		curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
#endif
		curl_easy_setopt(handle, CURLOPT_USERAGENT, this->m_sz_user_agent.c_str());
		curl_easy_setopt(handle, CURLOPT_COOKIEJAR, "-");
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&mem);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

		if (p_o_request.m_sz_method == GET) {

			p_o_request.m_sz_url.append(p_o_request.m_sz_path.c_str());
			curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
		}
		else if (p_o_request.m_sz_method == POST) {

			curl_easy_setopt(handle, CURLOPT_POST, 1);
			curl_easy_setopt(handle, CURLOPT_POSTFIELDS, p_o_request.m_sz_path.c_str());
		}

		curl_easy_setopt(handle, CURLOPT_URL, p_o_request.m_sz_url.c_str());

		if (p_o_request.m_sz_header.empty() == false) {

			struct curl_slist* chunk = NULL;

			for (auto& tt : p_o_request.m_sz_header) {

				chunk = curl_slist_append(chunk, tt.c_str());
				curl_easy_setopt(handle, CURLOPT_HTTPHEADER, chunk);
			}
		}

		CURLcode res = curl_easy_perform(handle);

		if (res != CURLE_OK) {

			throw std::exception("Failed to curl_easy_perform.");
		}

		p_o_request.m_sz_response = mem.memory;
	}

	void StreamToFile(VRequest& p_o_request, VSong& p_o_song) {

		std::string bin = "bin";

		CreateDirectory(bin.c_str(), NULL);
		bin.append("\\");
		bin.append(p_o_song.m_sz_artist.m_sz_title);

		CreateDirectory(bin.c_str(), NULL);

		bin.append("\\");

		if (p_o_song.m_i_track_number < 10) {

			bin.append("0");
		}
		
		bin.append(std::to_string(p_o_song.m_i_track_number));

		bin.append(" ");

		bin.append(p_o_song.m_sz_title);
		bin.append(".");
		bin.append(p_o_song.codec);

		FILE* fp = nullptr;
		fopen_s(&fp, bin.c_str(), "wb");

		mem.memory = static_cast<char*>(malloc(1));
		mem.size = 0;

#ifdef _DEBUG
		curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
#endif
		curl_easy_setopt(handle, CURLOPT_USERAGENT, this->m_sz_user_agent.c_str());
		curl_easy_setopt(handle, CURLOPT_COOKIEJAR, "-");
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1);
		curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

		if (p_o_request.m_sz_method == GET) {

			p_o_request.m_sz_url.append(p_o_request.m_sz_path.c_str());
			curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
		}
		else if (p_o_request.m_sz_method == POST) {

			curl_easy_setopt(handle, CURLOPT_POST, 1);
			curl_easy_setopt(handle, CURLOPT_POSTFIELDS, p_o_request.m_sz_path.c_str());
		}

		curl_easy_setopt(handle, CURLOPT_URL, p_o_request.m_sz_url.c_str());

		if (p_o_request.m_sz_header.empty() == false) {

			struct curl_slist* chunk = NULL;

			for (auto& tt : p_o_request.m_sz_header) {

				chunk = curl_slist_append(chunk, tt.c_str());
				curl_easy_setopt(handle, CURLOPT_HTTPHEADER, chunk);
			}
		}

		CURLcode res = curl_easy_perform(handle);

		fclose(fp);

		if (res != CURLE_OK) {

			throw std::exception("Failed to curl_easy_perform.");
		}

		p_o_request.m_sz_response = mem.memory;
	}

public:

	VUrl() : handle{ nullptr } {

	}

	VUrl(const std::string& p_sz_username, const std::string& p_sz_password, const std::string& p_sz_user_agent) : m_b_logged_in{ false }, handle { nullptr } {

		curl_global_init(CURL_GLOBAL_ALL);

		handle = curl_easy_init();

		this->m_sz_user_agent = p_sz_user_agent;

		std::string l_sz_path{ "username=" };
		l_sz_path.append(p_sz_username);
		l_sz_path.append("&password=");
		l_sz_path.append(p_sz_password);
		l_sz_path.append("&token=");
		l_sz_path.append(g_sz_token);
		l_sz_path.append("&clientVersion=");
		l_sz_path.append(g_sz_client_version);
		l_sz_path.append("&clientUniqueKey=");
		l_sz_path.append(g_sz_client_unique_key);

		VRequest l_o_tidal_login{ POST, g_sz_tidal_login_url, l_sz_path };

		this->SendRequest(l_o_tidal_login);

		logg(l_o_tidal_login.m_sz_response);

		auto json = nlohmann::json::parse(l_o_tidal_login.m_sz_response);

		if (l_o_tidal_login.m_sz_response.find("status") != std::string::npos) {

			auto status = json["status"].get<int>();

			if (status == 401) {


			}
		}
		else {

			this->m_b_logged_in = true;
			this->m_i_user_id = json["userId"].get<int>();
			this->m_sz_session_id = json["sessionId"].get<std::string>();
			this->m_sz_country_code = json["countryCode"].get<std::string>();

			logg("user id: ");
			logg(this->m_i_user_id);
			logg("session id: ");
			logg(this->m_sz_session_id);
			logg("country code: ");
			logg(this->m_sz_country_code);

			std::string session = "X-Tidal-SessionId: ";
			session.append(this->m_sz_session_id);

			this->m_sz_session_id = session;
		}
	}

	bool IsLoggedIn() {

		return this->m_b_logged_in;
	}

	VSong GetTrackInfo(const std::string& id) {

		std::string track = "/v1/tracks/";
		track.append(id);
		track.append("?countryCode=");
		track.append(this->m_sz_country_code);

		VRequest l_o_tidal_song_info{

			.m_sz_method{ GET },
			.m_sz_url{ g_sz_tidal_url },
			.m_sz_path{ track },
			.m_sz_header{ this->m_sz_session_id }
		};

		this->SendRequest(l_o_tidal_song_info);

		auto json = nlohmann::json::parse(l_o_tidal_song_info.m_sz_response);

		logg(l_o_tidal_song_info.m_sz_response);

		return {

		.m_i_id{ json["id"].get<int>() },
		.m_sz_title{ json["title"].get<std::string>() },
		.m_i_duration{ json["duration"].get<int>() },
		.m_fl_replay_gain{ json["replayGain"].get<float>() },
		.m_fl_peak{ json["peak"].get<float>() },
		.m_b_allow_streaming{ json["allowStreaming"].get<bool>() },
		.m_b_stream_ready{ json["streamReady"].get<bool>() },
		.m_b_premium_streaming_only{ json["premiumStreamingOnly"].get<bool>() },
		.m_i_track_number{ json["trackNumber"].get<int>() },
		//.m_i_version{ json["version"].get<int>()},
		.m_i_popularity{ json["popularity"].get<int>() },
		.m_sz_copyright{ json["copyright"].get<std::string>() },
		.m_sz_url{ json["url"].get<std::string>() },
		.m_b_explicit{ json["explicit"].get<bool>() },
		.m_sz_audio_quality{ json["audioQuality"].get<std::string>() },
		.m_sz_album{ json["album"]["id"].get<int>(), json["album"]["title"].get<std::string>(), json["album"]["cover"].get<std::string>() },
		.m_sz_artist{ json["artist"]["id"].get<int>(), json["artist"]["name"].get<std::string>() }
		};
	}
	const std::string quality{ "HI_RES" };
	const std::string mode{ "STREAM" };
	const std::string presentation{ "FULL" };

	void DownloadTrack( VSong& song) {

		std::string track = "/v1/tracks/";
		track.append(std::to_string(song.m_i_id));
		track.append("/playbackinfopostpaywall?countryCode=");
		track.append(this->m_sz_country_code);
		track.append("&audioquality=");
		track.append(quality);
		track.append("&playbackmode=");
		track.append(mode);
		track.append("&assetpresentation=");
		track.append(presentation);

		VRequest l_o_tidal_song_info{

			.m_sz_method{ GET },
			.m_sz_url{ g_sz_tidal_url },
			.m_sz_path{ track },
			.m_sz_header{ this->m_sz_session_id }
		};

		this->SendRequest(l_o_tidal_song_info);

		auto json = nlohmann::json::parse(l_o_tidal_song_info.m_sz_response);

		auto ret = json["manifest"].get<std::string>();

		CryptoPP::SecByteBlock _data((unsigned char*)ret.data(), ret.size());

		CryptoPP::Base64Decoder decoder;
		decoder.Put(_data.data(), _data.size());
		decoder.MessageEnd();

		CryptoPP::SecByteBlock decoded(decoder.MaxRetrievable());
		decoder.Get(decoded.data(), decoded.size());

		ret = (char*)decoded.data();

		std::string codec = ret;

		auto zzz = codec.find("\"codecs\":\"");
		if (zzz != std::string::npos) {

			codec.erase(0, zzz + 10);
		}

		zzz = codec.find("\"");
		if (zzz != std::string::npos) {

			codec.erase(zzz, codec.size());
		}

		song.codec = codec;
		

		auto zz = ret.find("\"urls\":[\"");
		if (zz != std::string::npos) {

			ret.erase(0, zz + 9);
		}

		zz = ret.find("\"]}");
		if (zz != std::string::npos) {

			ret.erase(zz, zz + 3);
		}

		VRequest l_o_tidal_download{

			.m_sz_method{ GET },
			.m_sz_url{ ret }
		};

		this->StreamToFile(l_o_tidal_download, song);

		//logg(l_o_tidal_download.m_sz_response);

		return;
	}

	~VUrl() {

		
	}
};

std::unique_ptr<VUrl> g_o_vurl;

namespace p5::creations {

	class CProcessInfo {

	private:

		HINSTANCE instance;

		int width;
		int height;

		std::string className;
		std::string windowName;

	public:

		CProcessInfo();
		CProcessInfo(const HINSTANCE& instance, const int& width = 450, const int& height = 350, const std::string& windowName = "wnd", const std::string& className = "cls");

		HINSTANCE getInstance();

		//bool isValid();

		int getWidth();
		int getHeight();

		const char* getClassName();
		const char* getWindowName();
	};
}

p5::creations::CProcessInfo::CProcessInfo() {

	this->instance = nullptr;
	this->width = 0;
	this->height = 0;
	this->className = "";
	this->windowName = "";
}

p5::creations::CProcessInfo::CProcessInfo(const HINSTANCE& instance, const int& width, const int& height, const std::string& windowName, const std::string& className) {

	this->instance = instance;
	this->width = width;
	this->height = height;
	this->className = className;
	this->windowName = windowName;
}

HINSTANCE p5::creations::CProcessInfo::getInstance() {

	return this->instance;
}

int p5::creations::CProcessInfo::getWidth() {

	return this->width;
}

int p5::creations::CProcessInfo::getHeight() {

	return this->height;
}

const char* p5::creations::CProcessInfo::getClassName() {

	return this->className.c_str();
}

const char* p5::creations::CProcessInfo::getWindowName() {

	return this->windowName.c_str();
}

namespace p5::creations {

	class CProcessWindow {

	private:

		HWND processHandle;

	public:

		CProcessWindow();
		CProcessWindow(CProcessInfo& processInfo);
		~CProcessWindow();

		bool isValid();
		//bool handleMessage(struct nk_context* ctx, int sleep = 1);

		HWND getProcessHandle();

		static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}

MARGINS Margin{ 0,0, 1920, 1080 };

NK_API int
nk_d3d9_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

LRESULT CALLBACK p5::creations::CProcessWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		//DwmExtendFrameIntoClientArea(hwnd, &Margin);
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	if (nk_d3d9_handle_event(hwnd, msg, wParam, lParam))
		return 0;

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

p5::creations::CProcessWindow::CProcessWindow() {

	this->processHandle = nullptr;
}

p5::creations::CProcessWindow::CProcessWindow(CProcessInfo& processInfo) {

	const auto className = processInfo.getClassName();

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = processInfo.getInstance();
	//wc.hIcon = LoadIcon(processInfo.getInstance(), MAKEINTRESOURCE(IDI_ICON1));
	//wc.hIconSm = LoadIcon(processInfo.getInstance(), MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(processInfo.getInstance(), IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszClassName = processInfo.getClassName();

	if (RegisterClassEx(&wc) == 0) {

		throw std::exception("Failed to RegisterClassEx.");
	}

	this->processHandle = CreateWindowEx(WS_EX_TRANSPARENT | 0, processInfo.getClassName(), processInfo.getWindowName(), /*WS_POPUP|*/ WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, processInfo.getWidth(), processInfo.getHeight(), nullptr, nullptr, processInfo.getInstance(), nullptr);

	if (this->processHandle == nullptr) {

		throw std::exception("Failed to CreateWindowEx.");
	}
}

p5::creations::CProcessWindow::~CProcessWindow() {

	if (this->processHandle != nullptr) {

		TerminateProcess(processHandle, 0);
	}
}

bool p5::creations::CProcessWindow::isValid() {

	return this->processHandle != nullptr;
}

//bool p5::creations::CProcessWindow::handleMessage(struct nk_context* ctx, int sleep) {
//
//	std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
//
//	if (this->isValid() != true) {
//
//		return false;
//	}
//
//	static MSG msg;
//	nk_input_begin(ctx);
//
//	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
//		if (msg.message == WM_QUIT)
//			return false;
//		TranslateMessage(&msg);
//		DispatchMessageW(&msg);
//	}
//
//	nk_input_end(ctx);
//
//	return true;
//}

HWND p5::creations::CProcessWindow::getProcessHandle() {

	return this->processHandle;
}

namespace p5::directx {

	class CD3D9Sprite {

	private:

		LPD3DXSPRITE sprite;
		LPDIRECT3DTEXTURE9 texture;

	public:

		CD3D9Sprite();

		CD3D9Sprite(IDirect3DDevice9Ex* device, const std::string& imageName);
		CD3D9Sprite(IDirect3DDevice9Ex* device, const char* memArray, size_t size);

		~CD3D9Sprite();

		LPD3DXSPRITE getSprite();

		LPDIRECT3DTEXTURE9 getTexture();

		void drawSprite(const float& posX = 0, const float& posY = 0, const D3DCOLOR& color = 0xFFFFFFFF);
	};

	class CD3D9 {

	private:

		const char* className;
		HINSTANCE instance;
		HWND processHandle;
		IDirect3DDevice9Ex* device;
		IDirect3D9Ex* pD3D;

	public:

		CD3D9();
		CD3D9(creations::CProcessInfo& processInfo, creations::CProcessWindow& processWindow, bool vsync = true);
		~CD3D9();

		CD3D9Sprite createSprite(const std::string& imageName);
		CD3D9Sprite createSprite(const char* memArray, size_t size);

		IDirect3DDevice9Ex* getDevice();

		void beginScene();

		void endScene();
	};
}


p5::directx::CD3D9Sprite::CD3D9Sprite() {

	this->sprite = nullptr;
	this->texture = nullptr;
}

p5::directx::CD3D9Sprite::CD3D9Sprite(IDirect3DDevice9Ex* device, const std::string& imageName) {

	this->sprite = nullptr;
	this->texture = nullptr;

	D3DXCreateTextureFromFile(device, imageName.c_str(), &this->texture);
	D3DXCreateSprite(device, &this->sprite);
}
#include <vector>
#include <iostream>
p5::directx::CD3D9Sprite::CD3D9Sprite(IDirect3DDevice9Ex* device, const char* memArray, size_t size) {

	this->sprite = nullptr;
	this->texture = nullptr;

	D3DXCreateTextureFromFileInMemory(device, memArray, size, &this->texture);
	D3DXCreateSprite(device, &this->sprite);
}

p5::directx::CD3D9Sprite::~CD3D9Sprite() {

	if (this->texture != nullptr) {

		this->texture->Release();

		this->texture = nullptr;
	}

	if (this->sprite != nullptr) {

		this->sprite->Release();

		this->sprite = nullptr;
	}
}

LPD3DXSPRITE p5::directx::CD3D9Sprite::getSprite() {

	return this->sprite;
}

LPDIRECT3DTEXTURE9 p5::directx::CD3D9Sprite::getTexture() {

	return this->texture;
}

void p5::directx::CD3D9Sprite::drawSprite(const float& posX, const float& posY, const D3DCOLOR& color) {

	if (sprite != nullptr
		&& this->texture != nullptr) {

		D3DXVECTOR3 position(posX, posY, 0);

		sprite->Begin(D3DXSPRITE_ALPHABLEND);
		sprite->Draw(this->texture, NULL, NULL, &position, color);
		sprite->End();
	}
}

p5::directx::CD3D9Sprite p5::directx::CD3D9::createSprite(const std::string& imageName) {

	return CD3D9Sprite{ device, imageName };
}

p5::directx::CD3D9Sprite p5::directx::CD3D9::createSprite(const char* memArray, size_t size) {

	return CD3D9Sprite{ device, memArray, size };
}

p5::directx::CD3D9::CD3D9() {

	this->className = nullptr;
	this->instance = nullptr;
	this->processHandle = nullptr;
	this->device = nullptr;
	this->pD3D = nullptr;
}

p5::directx::CD3D9::CD3D9(p5::creations::CProcessInfo& processInfo, p5::creations::CProcessWindow& processWindow, bool vsync) {

	this->className = processInfo.getClassName();
	this->instance = processInfo.getInstance();
	this->processHandle = processWindow.getProcessHandle();

	Direct3DCreate9Ex(D3D_SDK_VERSION, &this->pD3D);

	if (this->pD3D == nullptr) {

		throw std::exception("Failed to Direct3DCreate9Ex.");
	}

	D3DPRESENT_PARAMETERS pD3DParameters = { 0 };

	ZeroMemory(&pD3DParameters, sizeof(pD3DParameters));
	pD3DParameters.Windowed = TRUE;
	pD3DParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pD3DParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
	pD3DParameters.EnableAutoDepthStencil = TRUE;
	pD3DParameters.AutoDepthStencilFormat = D3DFMT_D16;
	pD3DParameters.PresentationInterval = vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE; // vsync / max fps

	if (this->pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->processHandle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pD3DParameters, 0, &this->device)) {

		this->pD3D->Release();

		UnregisterClass(this->className, this->instance);

		throw std::exception("Failed to CreateDeviceEx.");
	}

	this->device = device;
}

p5::directx::CD3D9::~CD3D9() {

	if (this->device != nullptr) {

		this->device->Release();

		this->device = nullptr;
	}

	if (this->pD3D != nullptr) {

		this->pD3D->Release();

		this->pD3D = nullptr;
	}
}

IDirect3DDevice9Ex* p5::directx::CD3D9::getDevice() {

	return this->device;
}

void p5::directx::CD3D9::beginScene() {

	this->device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);
	this->device->BeginScene();
}

void p5::directx::CD3D9::endScene() {

	this->device->EndScene();
	this->device->PresentEx(0, 0, 0, 0, 0);
}

namespace p5::nuklear {

	class CNuklear {

	private:

		nk_context* ctx;
		nk_font_atlas* atlas;

	public:
		CNuklear();
		CNuklear(p5::directx::CD3D9& direct3D9, p5::creations::CProcessInfo& processInfo);
		~CNuklear();

		nk_context* getContext();

		void renderScene();

		bool handleMessage(int sleep = 20);
	};
}

p5::nuklear::CNuklear::CNuklear() {

	this->ctx = nullptr;
	this->atlas = nullptr;
}

p5::nuklear::CNuklear::CNuklear(p5::directx::CD3D9& direct3D9, p5::creations::CProcessInfo& processInfo) {

	this->ctx = nk_d3d9_init(direct3D9.getDevice(), processInfo.getWidth(), processInfo.getHeight());

	if (this->ctx == nullptr) {

		throw std::exception("Failed to nk_d3d9_init.");
	}

	nk_d3d9_font_stash_begin(&this->atlas);
	nk_d3d9_font_stash_end();
}

p5::nuklear::CNuklear::~CNuklear() {

	nk_d3d9_shutdown();
}

nk_context* p5::nuklear::CNuklear::getContext() {

	return this->ctx;
}

void p5::nuklear::CNuklear::renderScene() {

	nk_d3d9_render(NK_ANTI_ALIASING_ON);
}

bool p5::nuklear::CNuklear::handleMessage(int sleep) {

	std::this_thread::sleep_for(std::chrono::milliseconds(sleep));

	static MSG msg;
	nk_input_begin(this->ctx);

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

		if (msg.message == WM_QUIT) {

			return false;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	nk_input_end(this->ctx);

	return true;
}
enum EReturns : int {

	LOGIN = -1,
	PASSWORD_MISMATCH = 0,
	HWID_MISMATCH = 1,
	HWID_SET_SUCCESS = 2,
	HWID_SET_FAIL = 3,
	LOGIN_SUCCESS = 4
};

struct CLogin {

	EReturns ret;
	int userGroup;
	std::vector<int> additionalGroup;
};
namespace p5::menu {

	class CMenu {

	private:

		nk_context* ctx;

		struct nk_image csgoImage;

		directx::CD3D9 direct3D9;
		directx::CD3D9Sprite csgoSprite;
		creations::CProcessInfo processInfo;
		//p5::web::CCurl curl;
		CLogin log;
		std::string menuText;
		//creations::CProcessWindow processWindow;

		void loginMenu();
		void mainMenu();

	public:
		CMenu();
		CMenu(nuklear::CNuklear& nklr, directx::CD3D9& direct3D9, creations::CProcessInfo& processInfo/*, creations::CProcessWindow& processWindow*/);

		void createScene();
	};
}

p5::menu::CMenu::CMenu() {

	this->ctx = nullptr;

	this->csgoImage = struct nk_image();

	this->direct3D9 = directx::CD3D9();
	this->csgoSprite = directx::CD3D9Sprite();
	this->processInfo = creations::CProcessInfo();
	//this->curl = web::CCurl();

	this->log = { EReturns::LOGIN, 0, std::vector<int>{} };

	this->menuText = "";

	//this->processWindow = creations::CProcessWindow();
}

p5::menu::CMenu::CMenu(p5::nuklear::CNuklear& nklr, p5::directx::CD3D9& direct3D9, p5::creations::CProcessInfo& processInfo/*, creations::CProcessWindow& processWindow*/) {

	this->ctx = nklr.getContext();

	this->csgoImage = struct nk_image();

	this->direct3D9 = direct3D9;
	this->processInfo = processInfo;

	//this->curl = web::CCurl();

	this->log = { EReturns::LOGIN, 0, std::vector<int>{} };

	this->menuText = "";
}

void p5::menu::CMenu::loginMenu() {

	static char username[256] = { "" };
	static char password[256] = { "" };

	//static auto exiled = this->direct3D9.createSprite((const char*)rawData, sizeof(rawData));
	//static auto exiledImage = nk_image_id(reinterpret_cast<int>(exiled.getTexture()));

	if (menuText.size() > 1) {

		nk_layout_row_dynamic(this->ctx, 182, 1); //100, 3
		//nk_image(this->ctx, exiledImage);

		nk_layout_row_dynamic(this->ctx, 20, 1);
		nk_label(this->ctx, menuText.c_str(), NK_TEXT_CENTERED);
	}
	else {

		nk_layout_row_dynamic(this->ctx, 202, 1); //100, 3
		//nk_image(this->ctx, exiledImage);
	}

	nk_layout_row_dynamic(this->ctx, 30, 1);
	nk_edit_string_zero_terminated(this->ctx, NK_EDIT_FIELD, username, sizeof(username) - 1, nk_filter_default);

	nk_layout_row_dynamic(this->ctx, 30, 1);
	nk_edit_string_zero_terminated(this->ctx, NK_EDIT_FIELD, password, sizeof(password) - 1, nk_filter_default);

	nk_layout_row_dynamic(this->ctx, 30, 1);
	if (nk_button_label(this->ctx, "LOGIN")) {

		g_o_vurl = std::make_unique<VUrl>(username, password, g_sz_tidal_user_agent);

		if (g_o_vurl->IsLoggedIn()) {

			this->log.ret = EReturns::LOGIN_SUCCESS;

			ZeroMemory(username, sizeof(username));
			ZeroMemory(password, sizeof(password));
		}
		else {

			menuText = "Username or password is wrong";
		}
		//this->log = this->curl.login(username, password, winMisc::generateUniqueHWID());
		//this->log = 4;
	}
}

void p5::menu::CMenu::mainMenu() {

	//todo add this to the class and do stuff on cunstruction
	//static auto csgoSpritea = this->direct3D9.createSprite(rawData, sizeof(rawData));
	//static auto csgoImagea = nk_image_id(reinterpret_cast<int>(csgoSpritea.getTexture()));
	static char username[256] = { "track ID" };
	static const char* MenuTitle = "build: " __DATE__ " " __TIME__;
	static const float ratio[] = { 212, 212 };
	static int selectedCheat = -1;
	static bool cshoe = false;
	static bool injcsgo = false;
	//static p5::web::CCurl curl;
	static std::future<void> Init;

	for (auto fug : log.additionalGroup) {

		if (fug == 10) {

			cshoe = true;
		}
	}

	nk_layout_row(this->ctx, NK_STATIC, 240, 2, ratio);

	if (nk_group_begin(this->ctx, "info", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

		nk_layout_row_dynamic(this->ctx, 15, 1);
		nk_label(this->ctx, MenuTitle, NK_TEXT_CENTERED);
		nk_layout_row_dynamic(this->ctx, 10, 1);

		//if (selectedCheat == 0) {
			std::vector<std::string> ass{ "It is done! I am the Alpha and the Omega, the Beginning and the End. To the thirsty I will give freely from the spring of the water of life." };
			//static auto gameInfo = "hello"/*this->curl.getGameInfo("csgo")*/;

			nk_layout_row_static(this->ctx, 20, 200, 1);

			for (auto info : ass) {

				nk_label_wrap(this->ctx, info.c_str());
			}
		//}

		nk_group_end(this->ctx);
	}

	if (nk_group_begin(this->ctx, "games", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) { // NK_WINDOW_NO_SCROLLBAR

		nk_layout_row_dynamic(this->ctx, 96, 1);

		//if (cshoe && nk_button_image(this->ctx, csgoImagea)) {

		//	selectedCheat = 0;
		//}

		nk_group_end(this->ctx);
	}

	nk_layout_row_dynamic(this->ctx, 20, 1);
	nk_edit_string_zero_terminated(this->ctx, NK_EDIT_FIELD, username, sizeof(username) - 1, nk_filter_default);

	//if (selectedCheat != -1) {

		nk_layout_row_dynamic(this->ctx, 20, 4);

		if (injcsgo) {

			//auto prog = curl.getTotalDownloaded();
			//nk_progress(ctx, &prog, curl.getTotalSize(), NK_MODIFIABLE);
		}
		else {

			if (nk_button_label(this->ctx, "Download")) {

				auto l_o_track_info = g_o_vurl->GetTrackInfo(username);

				g_o_vurl->DownloadTrack(l_o_track_info);

				ZeroMemory(username, sizeof(username));
				//auto processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, p5::winMisc::getProcessId("csgo.exe"));

				//curl.dlc(processHandle);

				//CloseHandle(processHandle);
			}
		}
	//}
}

void p5::menu::CMenu::createScene() {

	if (nk_begin(this->ctx, "Login", nk_rect(-5.0f, -5.0f, static_cast<float>(this->processInfo.getWidth() + 4), static_cast<float>(this->processInfo.getHeight() + 4)), 0)) {

		switch (this->log.ret) {

		case LOGIN: {

			this->loginMenu();

			break;
		}
		case PASSWORD_MISMATCH: {

			this->log.ret = LOGIN;

			this->menuText = "Incorrect login.";

			break;
		}
		case HWID_MISMATCH: {

			this->log.ret = LOGIN;

			this->menuText = "HWID Mismatch.";

			break;
		}
		case HWID_SET_SUCCESS: {

			ExitProcess(0);

			break;
		}
		case HWID_SET_FAIL: {

			this->log.ret = LOGIN;

			this->menuText = "HWID set error.";

			break;
		}
		case LOGIN_SUCCESS: {

			this->mainMenu();

			break;
		}
		default: {

			this->menuText = "Incorrect login.";

			break;
		}
		}

		nk_end(this->ctx);
	}
}

using namespace p5;
INT APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ INT cmdShow) {

	while (true) {

		/*std::cout << "Enter https://tidal.com/ track ID: ";

		int l_i_track_id = 0;
		std::cin >> l_i_track_id;

		auto l_o_track_info = vurl.GetTrackInfo(std::to_string(l_i_track_id));

		vurl.DownloadTrack(l_o_track_info);*/

		creations::CProcessInfo processInfo(hInstance, 450, 350, "Test");
		creations::CProcessWindow processWindow(processInfo);
		directx::CD3D9 direct3D9(processInfo, processWindow);
		nuklear::CNuklear nklr(direct3D9, processInfo);
		menu::CMenu menu(nklr, direct3D9, processInfo/*, processWindow*/);

		while (nklr.handleMessage(12)) {

			menu.createScene();
			direct3D9.beginScene();
			nklr.renderScene();
			direct3D9.endScene();
		}
	}

	return 0;
}
