#pragma once
#include <plugin.h>
#include "RenderWare.h"
#include "CRGBA.h"
#include "CRect.h"
#include "CVector2D.h"
#include "CSprite2d.h"
#include "common.h"
#include "Events.h"
#include <d3d9.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace sa_ui {

class Renderer {
private:
    static inline std::vector<RECT> m_clipStack;
    static constexpr float kPi = 3.14159265358979323846f;

    static void SetVertex(RwIm2DVertex& v, float x, float y, const CRGBA& col) {
        RwIm2DVertexSetScreenX(&v, x);
        RwIm2DVertexSetScreenY(&v, y);
        RwIm2DVertexSetScreenZ(&v, RwIm2DGetNearScreenZ());
        RwIm2DVertexSetRecipCameraZ(&v, 1.0f / (RwIm2DGetNearScreenZ() > 0.0f ? RwIm2DGetNearScreenZ() : 1.0f));
        RwIm2DVertexSetIntRGBA(&v, col.r, col.g, col.b, col.a);
        RwIm2DVertexSetU(&v, 0.0f, 1.0f);
        RwIm2DVertexSetV(&v, 0.0f, 1.0f);
    }

    static void PrepareRenderState() {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    }

public:
    static void Init() {}
    static void Shutdown() {}
    static void OnDeviceLost() {}

    static void DrawRect(CRect rect, CRGBA col) {
        CSprite2d::DrawRect(rect, col);
    }

    static void DrawBorder(CRect rect, CRGBA col, float thickness = 1.0f) {
        // Top
        DrawRect(CRect(rect.left, rect.top, rect.right, rect.top + thickness), col);
        // Bottom
        DrawRect(CRect(rect.left, rect.bottom - thickness, rect.right, rect.bottom), col);
        // Left
        DrawRect(CRect(rect.left, rect.top + thickness, rect.left + thickness, rect.bottom - thickness), col);
        // Right
        DrawRect(CRect(rect.right - thickness, rect.top + thickness, rect.right, rect.bottom - thickness), col);
    }

    // Single-pass convex polygon TRIFAN rounded rectangle (zero seams, supports alpha blending)
    static void DrawRoundedRect(CRect rect, CRGBA col, float radius, int segments = 6) {
        if (radius <= 1.0f) {
            DrawRect(rect, col);
            return;
        }

        segments = (std::max)(1, segments);

        float w = rect.right - rect.left;
        float h = rect.bottom - rect.top;
        radius = (std::min)({radius, w * 0.5f, h * 0.5f});

        PrepareRenderState();

        std::vector<RwIm2DVertex> verts;
        verts.reserve(segments * 4 + 3);

        RwIm2DVertex centerV;
        SetVertex(centerV, rect.left + w * 0.5f, rect.top + h * 0.5f, col);
        verts.push_back(centerV);

        auto AddArc = [&](float cx, float cy, float startAngle, float endAngle) {
            float step = (endAngle - startAngle) / static_cast<float>(segments);
            for (int i = 0; i <= segments; ++i) {
                float a = startAngle + step * i;
                RwIm2DVertex v;
                SetVertex(v, cx + std::cos(a) * radius, cy + std::sin(a) * radius, col);
                verts.push_back(v);
            }
        };

        // 1. Top-Right Corner (-PI/2 -> 0)
        AddArc(rect.right - radius, rect.top + radius, -kPi * 0.5f, 0.0f);
        // 2. Bottom-Right Corner (0 -> PI/2)
        AddArc(rect.right - radius, rect.bottom - radius, 0.0f, kPi * 0.5f);
        // 3. Bottom-Left Corner (PI/2 -> PI)
        AddArc(rect.left + radius, rect.bottom - radius, kPi * 0.5f, kPi);
        // 4. Top-Left Corner (PI -> 3PI/2)
        AddArc(rect.left + radius, rect.top + radius, kPi, kPi * 1.5f);

        // Close perimeter fan
        verts.push_back(verts[1]);

        RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts.data(), static_cast<int32_t>(verts.size()));
    }

    // Single-pass continuous TRISTRIP ribbon rounded border (clean corners, zero gap)
    static void DrawRoundedBorder(CRect rect, CRGBA col, float radius, float thickness = 1.0f, int segments = 6) {
        if (radius <= 1.0f) {
            DrawBorder(rect, col, thickness);
            return;
        }

        segments = (std::max)(1, segments);
        thickness = (std::max)(0.0f, thickness);

        float w = rect.right - rect.left;
        float h = rect.bottom - rect.top;
        radius = (std::min)({radius, w * 0.5f, h * 0.5f});
        float innerRadius = (std::max)(0.0f, radius - thickness);

        PrepareRenderState();

        std::vector<RwIm2DVertex> verts;
        verts.reserve((segments * 4 + 5) * 2);

        auto AddArcPair = [&](float cx, float cy, float startAngle, float endAngle) {
            float step = (endAngle - startAngle) / static_cast<float>(segments);
            for (int i = 0; i <= segments; ++i) {
                float a = startAngle + step * i;
                float cosA = std::cos(a);
                float sinA = std::sin(a);

                RwIm2DVertex vOuter, vInner;
                SetVertex(vOuter, cx + cosA * radius, cy + sinA * radius, col);
                SetVertex(vInner, cx + cosA * innerRadius, cy + sinA * innerRadius, col);

                verts.push_back(vOuter);
                verts.push_back(vInner);
            }
        };

        // 1. Top-Right Corner (-PI/2 -> 0)
        AddArcPair(rect.right - radius, rect.top + radius, -kPi * 0.5f, 0.0f);
        // 2. Bottom-Right Corner (0 -> PI/2)
        AddArcPair(rect.right - radius, rect.bottom - radius, 0.0f, kPi * 0.5f);
        // 3. Bottom-Left Corner (PI/2 -> PI)
        AddArcPair(rect.left + radius, rect.bottom - radius, kPi * 0.5f, kPi);
        // 4. Top-Left Corner (PI -> 3PI/2)
        AddArcPair(rect.left + radius, rect.top + radius, kPi, kPi * 1.5f);

        // Close the ribbon loop
        verts.push_back(verts[0]);
        verts.push_back(verts[1]);

        RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts.data(), static_cast<int32_t>(verts.size()));
    }

    // Procedural Multi-Band Gaussian Feathered Drop Shadow (100% native RenderWare 2D, zero artifacts)
    static void DrawShadow(CRect rect, float radius, float spread = 0.0f, CRGBA shadowCol = {0, 0, 0, 160}, float blur = 20.0f) {
        PrepareRenderState();

        const int kPasses = 8;
        const int kSegments = 6;
        float w = rect.right - rect.left;
        float h = rect.bottom - rect.top;
        radius = (std::min)({radius, w * 0.5f, h * 0.5f});

        // Downward offset for depth
        float offsetY = 4.0f;
        CRect baseRc(rect.left, rect.top + offsetY, rect.right, rect.bottom + offsetY);

        for (int pass = 0; pass < kPasses; ++pass) {
            float t0 = static_cast<float>(pass) / static_cast<float>(kPasses);
            float t1 = static_cast<float>(pass + 1) / static_cast<float>(kPasses);

            // Smooth cubic / quadratic gaussian decay
            float a0 = shadowCol.a * (1.0f - t0 * t0);
            float a1 = shadowCol.a * (1.0f - t1 * t1);
            if (pass == 0) a0 *= 0.85f;

            CRGBA col0{ shadowCol.r, shadowCol.g, shadowCol.b, static_cast<unsigned char>(a0) };
            CRGBA col1{ shadowCol.r, shadowCol.g, shadowCol.b, static_cast<unsigned char>(a1) };

            float d0 = spread + t0 * blur;
            float d1 = spread + t1 * blur;

            float r0 = radius + d0;
            float r1 = radius + d1;

            CRect rc0(baseRc.left - d0, baseRc.top - d0, baseRc.right + d0, baseRc.bottom + d0);
            CRect rc1(baseRc.left - d1, baseRc.top - d1, baseRc.right + d1, baseRc.bottom + d1);

            std::vector<RwIm2DVertex> verts;
            verts.reserve((kSegments * 4 + 5) * 2);

            auto AddRibbonArc = [&](float cx, float cy, float startAngle, float endAngle) {
                float step = (endAngle - startAngle) / static_cast<float>(kSegments);
                for (int i = 0; i <= kSegments; ++i) {
                    float a = startAngle + step * i;
                    float cosA = std::cos(a);
                    float sinA = std::sin(a);

                    RwIm2DVertex vInner, vOuter;
                    SetVertex(vInner, cx + cosA * r0, cy + sinA * r0, col0);
                    SetVertex(vOuter, cx + cosA * r1, cy + sinA * r1, col1);

                    verts.push_back(vInner);
                    verts.push_back(vOuter);
                }
            };

            // 1. Top-Right Corner (-PI/2 -> 0)
            AddRibbonArc(baseRc.right - radius, baseRc.top + radius, -kPi * 0.5f, 0.0f);
            // 2. Bottom-Right Corner (0 -> PI/2)
            AddRibbonArc(baseRc.right - radius, baseRc.bottom - radius, 0.0f, kPi * 0.5f);
            // 3. Bottom-Left Corner (PI/2 -> PI)
            AddRibbonArc(baseRc.left + radius, baseRc.bottom - radius, kPi * 0.5f, kPi);
            // 4. Top-Left Corner (PI -> 3PI/2)
            AddRibbonArc(baseRc.left + radius, baseRc.top + radius, kPi, kPi * 1.5f);

            verts.push_back(verts[0]);
            verts.push_back(verts[1]);

            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, verts.data(), static_cast<int32_t>(verts.size()));
        }
    }

    static void DrawBackdropOverlay(CRect screenRect, CRGBA tintCol = {6, 8, 12, 50}) {
        DrawRect(screenRect, tintCol);
    }

    static void DrawCheckmark(CVector2D pos, float size, CRGBA col, float thickness = 1.8f) {
        PrepareRenderState();
        float x1 = pos.x + size * 0.22f, y1 = pos.y + size * 0.52f;
        float x2 = pos.x + size * 0.44f, y2 = pos.y + size * 0.76f;
        float x3 = pos.x + size * 0.80f, y3 = pos.y + size * 0.26f;

        auto DrawSegment = [&](float ax, float ay, float bx, float by) {
            float dx = bx - ax;
            float dy = by - ay;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len <= 0.0001f) return;
            float nx = (-dy / len) * (thickness * 0.5f);
            float ny = (dx / len) * (thickness * 0.5f);

            RwIm2DVertex v[4];
            SetVertex(v[0], ax - nx, ay - ny, col);
            SetVertex(v[1], ax + nx, ay + ny, col);
            SetVertex(v[2], bx - nx, by - ny, col);
            SetVertex(v[3], bx + nx, by + ny, col);
            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, v, 4);
        };

        DrawSegment(x1, y1, x2, y2);
        DrawSegment(x2, y2, x3, y3);
    }

    static void DrawDot(CVector2D center, float radius, CRGBA col, int segments = 12) {
        if (radius <= 0.0f) return;
        segments = (std::max)(3, segments);
        PrepareRenderState();
        std::vector<RwIm2DVertex> verts;
        verts.reserve(segments + 2);

        RwIm2DVertex centerV;
        SetVertex(centerV, center.x, center.y, col);
        verts.push_back(centerV);

        float step = (kPi * 2.0f) / static_cast<float>(segments);
        for (int i = 0; i <= segments; ++i) {
            float a = step * i;
            RwIm2DVertex v;
            SetVertex(v, center.x + std::cos(a) * radius, center.y + std::sin(a) * radius, col);
            verts.push_back(v);
        }
        RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts.data(), static_cast<int32_t>(verts.size()));
    }

    static void PushClipRect(CRect rect) {
        auto dev = reinterpret_cast<IDirect3DDevice9*>(GetD3DDevice());
        if (!dev) return;

        RECT rc = { static_cast<LONG>(rect.left), static_cast<LONG>(rect.top), static_cast<LONG>(rect.right), static_cast<LONG>(rect.bottom) };
        m_clipStack.push_back(rc);

        dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        dev->SetScissorRect(&rc);
    }

    static void PopClipRect() {
        auto dev = reinterpret_cast<IDirect3DDevice9*>(GetD3DDevice());
        if (!dev) return;

        if (!m_clipStack.empty()) {
            m_clipStack.pop_back();
        }

        if (m_clipStack.empty()) {
            dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        } else {
            dev->SetScissorRect(&m_clipStack.back());
        }
    }
};

} // namespace sa_ui
