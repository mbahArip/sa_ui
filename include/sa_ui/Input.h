#pragma once
#include <plugin.h>
#include "CPad.h"
#include "CCamera.h"
#include "CMenuManager.h"
#include "CVector2D.h"
#include "CRect.h"
#include "CRGBA.h"
#include "RenderWare.h"
#include "CTxdStore.h"
#include <vector>
#include <string>
#include "Renderer.h"
#include "Theme.h"

namespace sa_ui {

class Input {
public:
    static inline CVector2D mousePos{0.0f, 0.0f};
    static inline CVector2D mouseDelta{0.0f, 0.0f};
    static inline bool mouseLeft = false;
    static inline bool mouseLeftDown = false;
    static inline bool mouseLeftUp = false;
    static inline bool mouseRight = false;
    static inline bool mouseRightDown = false;
    static inline int mouseWheel = 0;

    static inline unsigned int hotId = 0;
    static inline unsigned int activeId = 0;
    static inline unsigned int focusId = 0;

    static inline std::string textQueue = "";
    static inline bool keyBackspace = false;
    static inline bool keyDelete = false;
    static inline bool keyEnter = false;
    static inline bool keyTab = false;
    static inline bool keyEscape = false;
    static inline bool keyLeft = false;
    static inline bool keyRight = false;
    static inline bool keyUp = false;
    static inline bool keyDown = false;
    static inline bool keyShift = false;

    // Gamepad & Keyboard unified navigation
    static inline unsigned int navFocusId = 0;
    static inline bool isEditing = false;
    static inline std::vector<unsigned int> navList;
    static inline bool navUp = false;
    static inline bool navDown = false;
    static inline bool navLeft = false;
    static inline bool navRight = false;
    static inline bool navConfirm = false;
    static inline bool navCancel = false;
    static inline bool navTabPrev = false;
    static inline bool navTabNext = false;

    static inline bool uiCaptured = false;
    static inline bool isCursorVisible = true;
    static inline HWND gameHwnd = NULL;
    static inline WNDPROC originalWndProc = NULL;
    static inline bool wndProcHooked = false;
    static inline bool s_isUiOpen = false;

private:
    static inline RwTexture* s_mouseTexture = nullptr;
    static inline bool s_mouseTextureAttempted = false;
    static inline int s_mouseTxdSlot = -1;
    static inline bool s_mouseHookPatched = false;
    static inline unsigned char s_origUpdateMouseByte = 0x56;
    static inline CControllerState s_lastPadState{};
    static inline bool s_lastKeys[256]{};
    static inline bool s_wasPadDisabled = false;

    static LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (s_isUiOpen) {
            if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
                keyEscape = true;
                return 0; // Consume ESC so GTA SA frontend pause menu does not open
            }
            if (uMsg == WM_KEYUP && wParam == VK_ESCAPE) {
                return 0;
            }
            if (uMsg == WM_SYSKEYDOWN && wParam == VK_ESCAPE) {
                keyEscape = true;
                return 0;
            }
        }

        if (uMsg == WM_CHAR && wParam >= 32 && wParam <= 126) {
            textQueue += static_cast<char>(wParam);
        } else if (uMsg == WM_MOUSEWHEEL) {
            mouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        }

        return CallWindowProc(reinterpret_cast<FARPROC>(originalWndProc), hwnd, uMsg, wParam, lParam);
    }

    static void LoadDefaultCursor() {
        if (s_mouseTextureAttempted) return;
        s_mouseTextureAttempted = true;

        s_mouseTxdSlot = CTxdStore::FindTxdSlot("fronten_pc");
        if (s_mouseTxdSlot == -1) {
            s_mouseTxdSlot = CTxdStore::AddTxdSlot("fronten_pc");
            if (s_mouseTxdSlot != -1) {
                CTxdStore::LoadTxd(s_mouseTxdSlot, "models\\fronten_pc.txd");
            }
        }
        if (s_mouseTxdSlot != -1) {
            CTxdStore::AddRef(s_mouseTxdSlot);
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(s_mouseTxdSlot);
            s_mouseTexture = RwTextureRead("mouse", nullptr);
            CTxdStore::PopCurrentTxd();
        }
    }

public:
    static void Init(HWND hwnd = NULL) {
        if (!hwnd && RsGlobal.ps) {
            hwnd = RsGlobal.ps->window;
        }
        if (hwnd && !wndProcHooked) {
            gameHwnd = hwnd;
            originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
            wndProcHooked = true;
        }
        s_origUpdateMouseByte = *reinterpret_cast<unsigned char*>(0x53F3C0);
        LoadDefaultCursor();
    }

    static void Shutdown() {
        if (wndProcHooked && gameHwnd && originalWndProc) {
            SetWindowLongPtr(gameHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc));
            wndProcHooked = false;
        }
        if (s_mouseHookPatched) {
            plugin::patch::SetUChar(0x53F3C0, s_origUpdateMouseByte);
            s_mouseHookPatched = false;
        }
        if (s_wasPadDisabled) {
            CPad* pad = CPad::GetPad(0);
            if (pad) {
                pad->bDisablePlayerCycleWeapon = false;
                pad->bDisablePlayerJump = false;
                pad->bDisablePlayerDuck = false;
                pad->bDisablePlayerFireWeapon = false;
                pad->Clear(true, true);
            }
            s_wasPadDisabled = false;
        }
        if (s_mouseTexture) {
            RwTextureDestroy(s_mouseTexture);
            s_mouseTexture = nullptr;
        }
        if (s_mouseTxdSlot != -1) {
            CTxdStore::RemoveRef(s_mouseTxdSlot);
            s_mouseTxdSlot = -1;
        }
        s_mouseTextureAttempted = false;
    }

    static void Update(bool uiOpen) {
        s_isUiOpen = uiOpen;

        if (!wndProcHooked && RsGlobal.ps && RsGlobal.ps->window) {
            Init(RsGlobal.ps->window);
        }

        // DirectInput mouse lock patch (0x53F3C0 = CPad::UpdateMouse)
        if (uiOpen) {
            if (!s_mouseHookPatched) {
                plugin::patch::SetUChar(0x53F3C0, 0xC3); // RET: stops GTA SA from reading DirectInput mouse look
                s_mouseHookPatched = true;
            }
        } else {
            if (s_mouseHookPatched) {
                plugin::patch::SetUChar(0x53F3C0, s_origUpdateMouseByte);
                s_mouseHookPatched = false;
            }
        }

        // Get mouse position relative to GTA SA window
        POINT p;
        GetCursorPos(&p);
        if (RsGlobal.ps && RsGlobal.ps->window) {
            ScreenToClient(RsGlobal.ps->window, &p);
        }
        CVector2D newPos{ static_cast<float>(p.x), static_cast<float>(p.y) };
        mouseDelta = { newPos.x - mousePos.x, newPos.y - mousePos.y };
        mousePos = newPos;

        // Button states
        bool isLeft = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        mouseLeftDown = isLeft && !mouseLeft;
        mouseLeftUp   = !isLeft && mouseLeft;
        mouseLeft     = isLeft;

        bool isRight = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        mouseRightDown = isRight && !mouseRight;
        mouseRight     = isRight;

        // Key states with dedicated edge detection (avoids fragile & 1 latch bit)
        auto CheckKeyJustDown = [](int vk) -> bool {
            bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
            bool justDown = isDown && !s_lastKeys[vk & 0xFF];
            s_lastKeys[vk & 0xFF] = isDown;
            return justDown;
        };

        keyBackspace = CheckKeyJustDown(VK_BACK);
        keyDelete    = CheckKeyJustDown(VK_DELETE);
        keyEnter     = CheckKeyJustDown(VK_RETURN);
        keyTab       = CheckKeyJustDown(VK_TAB);
        keyLeft      = CheckKeyJustDown(VK_LEFT);
        keyRight     = CheckKeyJustDown(VK_RIGHT);
        keyUp        = CheckKeyJustDown(VK_UP);
        keyDown      = CheckKeyJustDown(VK_DOWN);
        keyShift     = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        // Controller / Gamepad navigation state polling (CPad 0)
        CPad* pad = CPad::GetPad(0);
        bool padUp = false, padDown = false, padLeft = false, padRight = false;
        bool padCross = false, padCircle = false, padTriangle = false, padL1 = false, padR1 = false;

        if (pad) {
            bool currUp = (pad->NewState.DPadUp != 0 || pad->NewState.LeftStickY < -64);
            bool prevUp = (s_lastPadState.DPadUp != 0 || s_lastPadState.LeftStickY < -64);
            padUp = currUp && !prevUp;

            bool currDown = (pad->NewState.DPadDown != 0 || pad->NewState.LeftStickY > 64);
            bool prevDown = (s_lastPadState.DPadDown != 0 || s_lastPadState.LeftStickY > 64);
            padDown = currDown && !prevDown;

            bool currLeft = (pad->NewState.DPadLeft != 0 || pad->NewState.LeftStickX < -64);
            bool prevLeft = (s_lastPadState.DPadLeft != 0 || s_lastPadState.LeftStickX < -64);
            padLeft = currLeft && !prevLeft;

            bool currRight = (pad->NewState.DPadRight != 0 || pad->NewState.LeftStickX > 64);
            bool prevRight = (s_lastPadState.DPadRight != 0 || s_lastPadState.LeftStickX > 64);
            padRight = currRight && !prevRight;

            padCross = (pad->NewState.ButtonCross != 0 && s_lastPadState.ButtonCross == 0);
            padCircle = (pad->NewState.ButtonCircle != 0 && s_lastPadState.ButtonCircle == 0);
            padTriangle = (pad->NewState.ButtonTriangle != 0 && s_lastPadState.ButtonTriangle == 0);
            padL1 = (pad->NewState.LeftShoulder1 != 0 && s_lastPadState.LeftShoulder1 == 0);
            padR1 = (pad->NewState.RightShoulder1 != 0 && s_lastPadState.RightShoulder1 == 0);

            s_lastPadState = pad->NewState;
        }

        navUp = keyUp || padUp;
        navDown = keyDown || padDown;
        navLeft = keyLeft || padLeft;
        navRight = keyRight || padRight;
        navConfirm = keyEnter || padCross || padCircle;
        navCancel = keyEscape || padTriangle;
        navTabPrev = padL1;
        navTabNext = padR1;

        if (uiOpen && !isEditing && !navList.empty()) {
            if (navFocusId == 0) {
                navFocusId = navList[0];
            } else {
                auto it = std::find(navList.begin(), navList.end(), navFocusId);
                int idx = (it != navList.end()) ? static_cast<int>(std::distance(navList.begin(), it)) : 0;
                if (navDown) {
                    idx = (idx + 1) % static_cast<int>(navList.size());
                    navFocusId = navList[idx];
                } else if (navUp) {
                    idx = (idx - 1 + static_cast<int>(navList.size())) % static_cast<int>(navList.size());
                    navFocusId = navList[idx];
                }
            }
        }

        // Block GTA SA player controls & camera mouse look when UI is open & captured
        if (uiOpen) {
            s_wasPadDisabled = true;
            if (pad) {
                pad->bDisablePlayerCycleWeapon = true;
                pad->bDisablePlayerJump = true;
                pad->bDisablePlayerDuck = true;
                pad->bDisablePlayerFireWeapon = true;
                pad->DisablePlayerControls = 1;
                pad->Clear(false, true); // Keep DisablePlayerControls=1 while clearing buffered hardware inputs
            }

            // Zero out mouse controller states
            CPad::NewMouseControllerState.x = 0.0f;
            CPad::NewMouseControllerState.y = 0.0f;
            CPad::PCTempMouseControllerState.x = 0.0f;
            CPad::PCTempMouseControllerState.y = 0.0f;
            CPad::OldMouseControllerState.x = 0.0f;
            CPad::OldMouseControllerState.y = 0.0f;
            CPad::NewMouseControllerState.z = 0.0f;
            CPad::PCTempMouseControllerState.z = 0.0f;
            CPad::OldMouseControllerState.z = 0.0f;
            CPad::NewMouseControllerState.lmb = false;
            CPad::NewMouseControllerState.rmb = false;
            CPad::NewMouseControllerState.mmb = false;
            CPad::NewMouseControllerState.wheelUp = 0;
            CPad::NewMouseControllerState.wheelDown = 0;

            // Zero camera angular velocities so follow camera freezes firmly in place
            if (TheCamera.m_nActiveCam < 3) {
                TheCamera.m_aCams[TheCamera.m_nActiveCam].m_fAlphaSpeedOverOneFrame = 0.0f;
                TheCamera.m_aCams[TheCamera.m_nActiveCam].m_fBetaSpeedOverOneFrame = 0.0f;
                TheCamera.m_aCams[TheCamera.m_nActiveCam].m_fBufferedTargetOrientationSpeed = 0.0f;
            }

            // Prevent pause menu trigger
            FrontEndMenuManager.m_bStartUpFrontEndRequested = false;
            FrontEndMenuManager.m_bMenuActive = false;
            CPad::NewKeyState.esc = 0;
            CPad::OldKeyState.esc = 0;
            CPad::NewKeyState.standardKeys[VK_ESCAPE] = 0;
            CPad::OldKeyState.standardKeys[VK_ESCAPE] = 0;
        } else if (s_wasPadDisabled) {
            // Restore player controls on UI close
            if (pad) {
                pad->bDisablePlayerCycleWeapon = false;
                pad->bDisablePlayerJump = false;
                pad->bDisablePlayerDuck = false;
                pad->bDisablePlayerFireWeapon = false;
                pad->Clear(true, true); // Resets DisablePlayerControls back to 0
            }
            s_wasPadDisabled = false;
        }
    }

    static void PostFrame() {
        textQueue.clear();
        mouseWheel = 0;
        hotId = 0;
        navList.clear();
        keyEscape = false;
    }

    static bool IsHovered(CRect rect) {
        return (mousePos.x >= rect.left && mousePos.x <= rect.right &&
                mousePos.y >= rect.top  && mousePos.y <= rect.bottom);
    }

    static void DrawCursor(const Theme& theme) {
        if (!isCursorVisible) return;

        if (!s_mouseTextureAttempted) {
            LoadDefaultCursor();
        }

        float x = mousePos.x;
        float y = mousePos.y;

        // Native GTA SA frontend mouse cursor (models/fronten_pc.txd > mouse)
        if (theme.useGameDefaultCursor && s_mouseTexture && s_mouseTexture->raster) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(s_mouseTexture));
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

            float cursorW = 28.0f;
            float cursorH = 28.0f;

            RwIm2DVertex v[4];
            auto SetTexV = [&](int idx, float vx, float vy, float u, float vCoord) {
                RwIm2DVertexSetScreenX(&v[idx], vx);
                RwIm2DVertexSetScreenY(&v[idx], vy);
                RwIm2DVertexSetScreenZ(&v[idx], RwIm2DGetNearScreenZ());
                RwIm2DVertexSetRecipCameraZ(&v[idx], 1.0f / (RwIm2DGetNearScreenZ() > 0.0f ? RwIm2DGetNearScreenZ() : 1.0f));
                RwIm2DVertexSetIntRGBA(&v[idx], 255, 255, 255, 255);
                RwIm2DVertexSetU(&v[idx], u, 1.0f);
                RwIm2DVertexSetV(&v[idx], vCoord, 1.0f);
            };

            SetTexV(0, x, y, 0.0f, 0.0f);
            SetTexV(1, x + cursorW, y, 1.0f, 0.0f);
            SetTexV(2, x, y + cursorH, 0.0f, 1.0f);
            SetTexV(3, x + cursorW, y + cursorH, 1.0f, 1.0f);

            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, v, 4);
            return;
        }

        // Procedural arrow cursor fallback
        RwIm2DVertex v[8];
        auto SetV = [&](int idx, float vx, float vy, CRGBA col) {
            RwIm2DVertexSetScreenX(&v[idx], vx);
            RwIm2DVertexSetScreenY(&v[idx], vy);
            RwIm2DVertexSetScreenZ(&v[idx], RwIm2DGetNearScreenZ());
            RwIm2DVertexSetRecipCameraZ(&v[idx], 1.0f / RwIm2DGetNearScreenZ());
            RwIm2DVertexSetIntRGBA(&v[idx], col.r, col.g, col.b, col.a);
            RwIm2DVertexSetU(&v[idx], 0.0f, 1.0f);
            RwIm2DVertexSetV(&v[idx], 0.0f, 1.0f);
        };

        CRGBA borderCol{0, 0, 0, 220};
        CRGBA fillCol = theme.foreground;

        // Shadow
        Renderer::DrawRect(CRect(x + 2, y + 2, x + 12, y + 16), CRGBA{0, 0, 0, 80});

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

        // Body triangle
        SetV(0, x, y, fillCol);
        SetV(1, x, y + 15.0f, fillCol);
        SetV(2, x + 11.0f, y + 11.0f, fillCol);
        RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, v, 3);

        // Stem
        Renderer::DrawRect(CRect(x + 4.0f, y + 9.0f, x + 8.0f, y + 17.0f), fillCol);
        Renderer::DrawBorder(CRect(x + 4.0f, y + 9.0f, x + 8.0f, y + 17.0f), borderCol, 1.0f);
    }
};

} // namespace sa_ui
