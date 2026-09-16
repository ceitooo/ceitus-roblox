#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <thread>
#include <atomic>
#include "Offsets.hpp"

#pragma comment(lib, "winhttp.lib")

// ---- estado visible en el panel ----
inline std::string g_offsetStatus = "No actualizado";
inline std::atomic<bool> g_offsetUpdaterRunning{ false };

// ---- fetch HTTPS simple ----
struct FetchResult { int status; std::string body; std::string etag; };

static FetchResult FetchOffsets(const std::wstring& slug, const std::wstring& ifNoneMatch = {})
{
    FetchResult out{};
    HINTERNET sess = WinHttpOpen(L"ceitus-offsets/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!sess) return out;

    HINTERNET conn = WinHttpConnect(sess, L"www.cheatoffsets.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!conn) { WinHttpCloseHandle(sess); return out; }

    std::wstring path = L"/api/games/" + slug + L"/current";
    HINTERNET req = WinHttpOpenRequest(conn, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!req) { WinHttpCloseHandle(conn); WinHttpCloseHandle(sess); return out; }

    std::wstring hdrs;
    if (!ifNoneMatch.empty()) hdrs = L"If-None-Match: " + ifNoneMatch;
    WinHttpSendRequest(req,
        hdrs.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : hdrs.c_str(),
        (DWORD)hdrs.size(), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(req, nullptr);

    DWORD st = 0, sz = sizeof(st);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &st, &sz, WINHTTP_NO_HEADER_INDEX);
    out.status = (int)st;

    wchar_t ebuf[256]{}; DWORD elen = sizeof(ebuf);
    if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CUSTOM, L"ETag",
            ebuf, &elen, WINHTTP_NO_HEADER_INDEX)) {
        char nb[256]{};
        WideCharToMultiByte(CP_UTF8, 0, ebuf, -1, nb, sizeof(nb), nullptr, nullptr);
        out.etag = nb;
    }

    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(req, &avail) && avail) {
        std::string chunk(avail, '\0');
        DWORD read = 0;
        WinHttpReadData(req, chunk.data(), avail, &read);
        out.body.append(chunk, 0, read);
    }
    WinHttpCloseHandle(req); WinHttpCloseHandle(conn); WinHttpCloseHandle(sess);
    return out;
}

// ---- parser JSON minimalista: busca "Key": "0xVALOR" o "Key": VALOR ----
static uintptr_t JsonHex(const std::string& json, const std::string& key)
{
    // busca  "Key": "0x..."  o  "Key": ...
    std::string q = "\"" + key + "\"";
    size_t p = json.find(q);
    if (p == std::string::npos) return 0;
    p = json.find(':', p + q.size());
    if (p == std::string::npos) return 0;
    while (++p < json.size() && (json[p] == ' ' || json[p] == '"'));
    try {
        return (uintptr_t)std::stoull(json.substr(p), nullptr, 16);
    } catch (...) { return 0; }
}

// ---- mapa de nombres JSON → puntero al offset ----
// Ajustá los nombres de la izquierda según lo que devuelva la API
static void ApplyJson(const std::string& json)
{
    struct { const char* key; uintptr_t* ptr; } map[] = {
        // VisualEngine
        { "VisualEngine_Pointer",        &Offsets::VisualEngine::Pointer        },
        { "VisualEngine_ViewMatrix",      &Offsets::VisualEngine::ViewMatrix     },
        { "VisualEngine_RenderView",      &Offsets::VisualEngine::RenderView     },
        // FakeDataModel
        { "FakeDataModel_Pointer",        &Offsets::FakeDataModel::Pointer       },
        { "FakeDataModel_RealDataModel",  &Offsets::FakeDataModel::RealDataModel },
        // TaskScheduler
        { "TaskScheduler_Pointer",        &Offsets::TaskScheduler::Pointer       },
        // DataModel
        { "DataModel_PlaceId",            &Offsets::DataModel::PlaceId           },
        { "DataModel_GameId",             &Offsets::DataModel::GameId            },
        { "DataModel_Workspace",          &Offsets::DataModel::Workspace         },
        { "DataModel_GameLoaded",         &Offsets::DataModel::GameLoaded        },
        { "DataModel_ServerIP",           &Offsets::DataModel::ServerIP          },
        // Players
        { "Players_LocalPlayer",          &Offsets::Players::LocalPlayer         },
        // Player
        { "Player_Character",             &Offsets::Player::Character            },
        { "Player_ModelInstance",         &Offsets::Player::ModelInstance        },
        { "Player_Team",                  &Offsets::Player::Team                 },
        { "Player_TeamColor",             &Offsets::Player::TeamColor            },
        // Humanoid
        { "Humanoid_Health",              &Offsets::Humanoid::Health             },
        { "Humanoid_MaxHealth",           &Offsets::Humanoid::MaxHealth          },
        { "Humanoid_HumanoidRootPart",    &Offsets::Humanoid::HumanoidRootPart   },
        { "Humanoid_HumanoidState",       &Offsets::Humanoid::HumanoidState      },
        { "Humanoid_IsWalking",           &Offsets::Humanoid::IsWalking          },
        // BasePart / Primitive
        { "BasePart_Primitive",           &Offsets::BasePart::Primitive          },
        { "Primitive_Position",           &Offsets::Primitive::Position          },
        { "Primitive_Size",               &Offsets::Primitive::Size              },
        { "Primitive_Rotation",           &Offsets::Primitive::Rotation          },
        // Instance
        { "Instance_ChildrenStart",       &Offsets::Instance::ChildrenStart      },
        { "Instance_NameContainer",       &Offsets::Instance::NameContainer      },
        { "Instance_Parent",              &Offsets::Instance::Parent             },
        // Misc
        { "Misc_StringLength",            &Offsets::Misc::StringLength           },
        // Model
        { "Model_PrimaryPart",            &Offsets::Model::PrimaryPart           },
    };
    int applied = 0;
    for (auto& e : map) {
        uintptr_t v = JsonHex(json, e.key);
        if (v) { *e.ptr = v; ++applied; }
    }
    char buf[64];
    SYSTEMTIME t; GetLocalTime(&t);
    snprintf(buf, sizeof(buf), "OK %02d:%02d (%d offsets)", t.wHour, t.wMinute, applied);
    g_offsetStatus = buf;
}

// ---- thread de actualización (llamar una sola vez) ----
inline void StartOffsetUpdater(const std::wstring& slug = L"roblox-player")
{
    if (g_offsetUpdaterRunning.exchange(true)) return;
    std::thread([slug]() {
        std::wstring etag;
        while (true) {
            g_offsetStatus = "Actualizando...";
            auto r = FetchOffsets(slug, etag);
            if (r.status == 200 && !r.body.empty()) {
                ApplyJson(r.body);
                etag = std::wstring(r.etag.begin(), r.etag.end());
            } else if (r.status == 304) {
                // sin cambios, mantener etag
                char buf[32]; SYSTEMTIME t; GetLocalTime(&t);
                snprintf(buf, sizeof(buf), "Sin cambios %02d:%02d", t.wHour, t.wMinute);
                g_offsetStatus = buf;
            } else {
                g_offsetStatus = "Error HTTP " + std::to_string(r.status);
            }
            // esperar 10 minutos antes del siguiente fetch
            for (int i = 0; i < 600 && g_offsetUpdaterRunning; ++i)
                Sleep(1000);
        }
    }).detach();
}
