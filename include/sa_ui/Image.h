#pragma once

#include "CRGBA.h"
#include "CRect.h"
#include "CSprite2d.h"
#include "CTxdStore.h"
#include "RenderWare.h"
#include <string>
#include <unordered_map>
#include <algorithm>

namespace sa_ui {

enum class ImageFit { Stretch, Contain, Cover };

class ImageManager {
private:
    static inline std::unordered_map<std::string, RwTexture*> s_textures;

public:
    static RwTexture* LoadFromCurrentTxd(const std::string& imageId, const char* textureName) {
        auto existing = s_textures.find(imageId);
        if (existing != s_textures.end()) return existing->second;
        RwTexture* texture = RwTextureRead(textureName, nullptr);
        if (texture) s_textures.emplace(imageId, texture);
        return texture;
    }

    static void Unload(const std::string& imageId) {
        auto it = s_textures.find(imageId);
        if (it == s_textures.end()) return;
        if (it->second) RwTextureDestroy(it->second);
        s_textures.erase(it);
    }

    static void Shutdown() {
        for (auto& [id, texture] : s_textures) {
            if (texture) RwTextureDestroy(texture);
        }
        s_textures.clear();
    }

    static RwTexture* Get(const std::string& imageId) {
        auto it = s_textures.find(imageId);
        return it == s_textures.end() ? nullptr : it->second;
    }
};

inline void Image(const std::string& imageId, CRect rect, CRGBA tint = {255, 255, 255, 255}, ImageFit fit = ImageFit::Stretch) {
    RwTexture* texture = ImageManager::Get(imageId);
    if (!texture || !texture->raster) return;

    const float sourceWidth = static_cast<float>(texture->raster->width);
    const float sourceHeight = static_cast<float>(texture->raster->height);
    const float targetWidth = rect.right - rect.left;
    const float targetHeight = rect.bottom - rect.top;
    CRect drawRect = rect;

    if (fit != ImageFit::Stretch && sourceWidth > 0.0f && sourceHeight > 0.0f) {
        const float scaleX = targetWidth / sourceWidth;
        const float scaleY = targetHeight / sourceHeight;
        const float scale = fit == ImageFit::Contain ? (std::min)(scaleX, scaleY) : (std::max)(scaleX, scaleY);
        const float drawWidth = sourceWidth * scale;
        const float drawHeight = sourceHeight * scale;
        drawRect = CRect(
            rect.left + (targetWidth - drawWidth) * 0.5f,
            rect.top + (targetHeight - drawHeight) * 0.5f,
            rect.left + (targetWidth + drawWidth) * 0.5f,
            rect.top + (targetHeight + drawHeight) * 0.5f
        );
    }

    CSprite2d sprite;
    sprite.m_pTexture = texture;
    sprite.Draw(drawRect, tint);
}

} // namespace sa_ui
