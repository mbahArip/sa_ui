#pragma once

#include "Theme.h"
#include "Font.h"
#include "Renderer.h"
#include "Input.h"
#include "Image.h"
#include "Components.h"
#include "Layout.h"
#include "Toast.h"
#include "CTimer.h"
#include "CCamera.h"

namespace sa_ui {

inline bool s_wasUiOpen = false;
inline float s_originalTimeScale = 1.0f;

inline void Init(const Theme& theme = Theme::ZincDark()) {
    SetTheme(theme);
    FontManager::Init();
    Input::Init();
}

inline void BeginFrame(bool uiOpen) {
    const Theme& theme = GetTheme();
    Input::Update(uiOpen);

    // Time scale slowing / pausing management
    if (uiOpen && !s_wasUiOpen) {
        s_originalTimeScale = CTimer::ms_fTimeScale;
        if (theme.slowTimeWhenOpen) {
            CTimer::ms_fTimeScale = theme.timeScaleOnOpen;
        }
    } else if (!uiOpen && s_wasUiOpen) {
        if (theme.slowTimeWhenOpen) {
            CTimer::ms_fTimeScale = (s_originalTimeScale > 0.0f) ? s_originalTimeScale : 1.0f;
        }
        TheCamera.m_nMotionBlur = 0;
        TheCamera.m_nMotionBlurAddAlpha = 0;

        // Recenter cursor to screen center to protect flight / driving mouse look from snapping
        if (Input::gameHwnd && RsGlobal.maximumWidth > 0 && RsGlobal.maximumHeight > 0) {
            POINT pt{ RsGlobal.maximumWidth / 2, RsGlobal.maximumHeight / 2 };
            ClientToScreen(Input::gameHwnd, &pt);
            SetCursorPos(pt.x, pt.y);
            Input::mousePos = { static_cast<float>(pt.x), static_cast<float>(pt.y) };
            Input::mouseDelta = { 0.0f, 0.0f };
        }
    }
    s_wasUiOpen = uiOpen;

    // Full-screen backdrop dim overlay
    if (uiOpen && theme.showBackdropOverlay) {
        float sW = (float)RsGlobal.maximumWidth;
        float sH = (float)RsGlobal.maximumHeight;
        Renderer::DrawBackdropOverlay(CRect(0.0f, 0.0f, sW, sH), theme.backdropOverlayCol);
    }
}

inline void EndFrame(bool uiOpen) {
    if (uiOpen) {
        FlushOverlays();
        Input::DrawCursor(GetTheme());
    }
    Toast::Render();
    Input::PostFrame();
}

inline void Shutdown() {
    if (s_wasUiOpen && GetTheme().slowTimeWhenOpen) {
        CTimer::ms_fTimeScale = 1.0f;
    }
    TheCamera.m_nMotionBlur = 0;
    TheCamera.m_nMotionBlurAddAlpha = 0;
    Renderer::Shutdown();
    Input::Shutdown();
    FontManager::Shutdown();
    ImageManager::Shutdown();
}

} // namespace sa_ui
// ponytail: immediate-mode direct submission to RwIm2D; add auto-flexbox layout manager when complex nested responsive grids requested.
