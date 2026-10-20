#pragma once
#include <plugin.h>
#include "CRGBA.h"
#include "Theme.h"
#include "CRect.h"
#include "CFont.h"
#include "common.h"
#include "Events.h"
#include "extensions/Paths.h"
#include <d3d9.h>
#include <d3dx9core.h>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "gdi32.lib")

namespace sa_ui {

enum eFontAlign {
    ALIGN_LEFT    = DT_LEFT,
    ALIGN_CENTER  = DT_CENTER,
    ALIGN_RIGHT   = DT_RIGHT,
    ALIGN_VCENTER = DT_VCENTER | DT_SINGLELINE
};

struct FontDef {
    std::string id;
    std::string filePath;
    std::string faceName;
    int height{16};
    int weight{FW_NORMAL};
    ID3DXFont* pD3dFont{nullptr};
    bool isCustomFile{false};
};

class FontManager {
private:
    static inline std::unordered_map<std::string, FontDef> m_fonts;
    static inline std::string m_defaultFontId = "";
    static inline bool m_initialized = false;

    static IDirect3DDevice9* GetDevice() {
        return reinterpret_cast<IDirect3DDevice9*>(GetD3DDevice());
    }

public:
    static void Init() {
        if (m_initialized) return;
        m_initialized = true;

        plugin::Events::d3dLostEvent += [] {
            for (auto& [id, def] : m_fonts) {
                if (def.pD3dFont) {
                    def.pD3dFont->OnLostDevice();
                }
            }
        };

        plugin::Events::d3dResetEvent += [] {
            for (auto& [id, def] : m_fonts) {
                if (def.pD3dFont) {
                    def.pD3dFont->OnResetDevice();
                }
            }
        };

        plugin::Events::shutdownRwEvent += [] {
            Shutdown();
        };
    }

    // Load TrueType / OpenType font from .ttf or .otf file on disk.
    // Auto-resolves:
    // 1. Relative to .asi folder (e.g. modloader/MyMod/fonts/Font.ttf)
    // 2. Relative to GTA SA game folder
    // 3. Absolute path
    static bool LoadFromFile(const std::string& fontId, const std::string& filePath, const std::string& faceName, int height = 16, int weight = FW_NORMAL) {
        Init();

        std::string resolvedPath = filePath;
        int registered = AddFontResourceExA(resolvedPath.c_str(), FR_PRIVATE, NULL);

        // Try relative to .asi directory (PLUGIN_PATH)
        if (registered <= 0) {
            char pluginRelative[MAX_PATH];
            plugin::paths::GetPluginDirRelativePathA(pluginRelative, filePath.c_str());
            registered = AddFontResourceExA(pluginRelative, FR_PRIVATE, NULL);
            if (registered > 0) {
                resolvedPath = pluginRelative;
            }
        }

        // Try relative to game directory (GAME_PATH)
        if (registered <= 0) {
            char gameRelative[MAX_PATH];
            plugin::paths::GetGameDirRelativePathA(gameRelative, filePath.c_str());
            registered = AddFontResourceExA(gameRelative, FR_PRIVATE, NULL);
            if (registered > 0) {
                resolvedPath = gameRelative;
            }
        }

        ID3DXFont* pFont = nullptr;
        IDirect3DDevice9* dev = GetDevice();
        if (!dev) return false;

        HRESULT hr = D3DXCreateFontA(
            dev,
            height,
            0,
            weight,
            1,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            faceName.c_str(),
            &pFont
        );

        if (FAILED(hr) || !pFont) {
            return false;
        }

        FontDef def;
        def.id = fontId;
        def.filePath = resolvedPath;
        def.faceName = faceName;
        def.height = height;
        def.weight = weight;
        def.pD3dFont = pFont;
        def.isCustomFile = (registered > 0);

        m_fonts[fontId] = def;
        if (m_defaultFontId.empty()) {
            m_defaultFontId = fontId;
        }

        return true;
    }

    // Load system font (e.g. "Segoe UI", "Arial", "Consolas")
    static bool LoadSystem(const std::string& fontId, const std::string& faceName, int height = 16, int weight = FW_NORMAL) {
        Init();
        ID3DXFont* pFont = nullptr;
        IDirect3DDevice9* dev = GetDevice();
        if (!dev) return false;

        HRESULT hr = D3DXCreateFontA(
            dev,
            height,
            0,
            weight,
            1,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            faceName.c_str(),
            &pFont
        );

        if (FAILED(hr) || !pFont) {
            return false;
        }

        FontDef def;
        def.id = fontId;
        def.faceName = faceName;
        def.height = height;
        def.weight = weight;
        def.pD3dFont = pFont;
        def.isCustomFile = false;

        m_fonts[fontId] = def;
        if (m_defaultFontId.empty()) {
            m_defaultFontId = fontId;
        }

        return true;
    }

    static void SetDefault(const std::string& fontId) {
        m_defaultFontId = fontId;
    }

    static FontDef* Get(const std::string& fontId = "") {
        const std::string& target = fontId.empty() ? m_defaultFontId : fontId;
        auto it = m_fonts.find(target);
        if (it != m_fonts.end()) return &it->second;
        return nullptr;
    }

    static void Draw(const std::string& fontId, const char* text, CRect rect, CRGBA color, DWORD alignFlags = ALIGN_LEFT | DT_NOCLIP) {
        if (!text || !text[0]) return;

        FontDef* f = Get(fontId);
        if (f && f->pD3dFont) {
            RECT rc = { static_cast<LONG>(rect.left), static_cast<LONG>(rect.top), static_cast<LONG>(rect.right), static_cast<LONG>(rect.bottom) };
            D3DCOLOR d3dCol = D3DCOLOR_RGBA(color.r, color.g, color.b, color.a);
            f->pD3dFont->DrawTextA(nullptr, text, -1, &rc, alignFlags, d3dCol);
            return;
        }

        // Native GTA SA CFont fallback
        CFont::SetScale(0.35f, 0.95f);
        CFont::SetColor(color);
        if (alignFlags & DT_CENTER) {
            CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
            CFont::PrintString(rect.left + (rect.right - rect.left) * 0.5f, rect.top, const_cast<char*>(text));
        } else if (alignFlags & DT_RIGHT) {
            CFont::SetOrientation(eFontAlignment::ALIGN_RIGHT);
            CFont::PrintString(rect.right, rect.top, const_cast<char*>(text));
        } else {
            CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
            CFont::PrintString(rect.left, rect.top, const_cast<char*>(text));
        }
    }

    static void DrawGameFont(GameFontStyle style, const char* text, CRect rect, CRGBA color, float scaleX = 0.35f, float scaleY = 0.95f, DWORD alignFlags = ALIGN_LEFT) {
        if (!text || !text[0]) return;

        CFont::SetScale(scaleX, scaleY);
        CFont::SetColor(color);
        CFont::SetBackground(false, false);
        CFont::SetProportional(true);
        CFont::SetDropShadowPosition(0);
        CFont::SetFontStyle(style == GameFontStyle::Gothic ? FONT_GOTHIC :
                            style == GameFontStyle::Subtitle ? FONT_SUBTITLES :
                            style == GameFontStyle::Menu ? FONT_MENU : FONT_PRICEDOWN);

        if (alignFlags & DT_CENTER) {
            CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
            CFont::PrintString(rect.left + (rect.right - rect.left) * 0.5f, rect.top, const_cast<char*>(text));
        } else if (alignFlags & DT_RIGHT) {
            CFont::SetOrientation(eFontAlignment::ALIGN_RIGHT);
            CFont::PrintString(rect.right, rect.top, const_cast<char*>(text));
        } else {
            CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
            CFont::PrintString(rect.left, rect.top, const_cast<char*>(text));
        }
    }

    static void Measure(const std::string& fontId, const char* text, float* outW, float* outH) {
        if (!text || !text[0]) {
            if (outW) *outW = 0.0f;
            if (outH) *outH = 0.0f;
            return;
        }

        FontDef* f = Get(fontId);
        if (f && f->pD3dFont) {
            RECT rc = { 0, 0, 0, 0 };
            f->pD3dFont->DrawTextA(nullptr, text, -1, &rc, DT_CALCRECT, 0);
            if (outW) *outW = static_cast<float>(rc.right - rc.left);
            if (outH) *outH = static_cast<float>(rc.bottom - rc.top);
            return;
        }

        if (outW) *outW = static_cast<float>(strlen(text) * 8.0f);
        if (outH) *outH = 14.0f;
    }

    static void Shutdown() {
        for (auto& [id, def] : m_fonts) {
            if (def.pD3dFont) {
                def.pD3dFont->Release();
                def.pD3dFont = nullptr;
            }
            if (def.isCustomFile && !def.filePath.empty()) {
                RemoveFontResourceExA(def.filePath.c_str(), FR_PRIVATE, NULL);
            }
        }
        m_fonts.clear();
        m_initialized = false;
    }
};

} // namespace sa_ui
