#pragma once

#include "CRGBA.h"
#include "CRect.h"
#include "Theme.h"
#include "Font.h"
#include "Renderer.h"
#include "CTimer.h"
#include <string>
#include <vector>
#include <algorithm>

namespace sa_ui {

enum class ToastVariant {
    Default,
    Success,
    Warning,
    Destructive
};

struct ToastProps {
    std::string title{};
    std::string description{};
    ToastVariant variant{ToastVariant::Default};
    unsigned int durationMs{3500};
};

class Toast {
private:
    struct ToastItem {
        std::string title;
        std::string description;
        ToastVariant variant;
        unsigned int startTime;
        unsigned int durationMs;
    };

    static inline std::vector<ToastItem> s_toasts;

public:
    static void Show(const ToastProps& props) {
        if (s_toasts.size() >= 5) {
            s_toasts.erase(s_toasts.begin());
        }
        s_toasts.push_back({
            props.title,
            props.description,
            props.variant,
            GetTickCount(),
            props.durationMs
        });
    }

    static void Show(const char* title, const char* description = nullptr, ToastVariant variant = ToastVariant::Default, unsigned int durationMs = 3500) {
        Show(ToastProps{
            title ? title : "",
            description ? description : "",
            variant,
            durationMs
        });
    }

    static void Render() {
        if (s_toasts.empty()) return;

        unsigned int now = GetTickCount();
        // Purge expired toasts
        s_toasts.erase(std::remove_if(s_toasts.begin(), s_toasts.end(), [&](const ToastItem& t) {
            return (now - t.startTime) >= t.durationMs;
        }), s_toasts.end());

        float sW = static_cast<float>(RsGlobal.maximumWidth);
        float toastW = 320.0f;
        float toastH = 58.0f;
        float marginX = 24.0f;
        float curY = 24.0f;

        const Theme& theme = GetTheme();

        for (size_t i = 0; i < s_toasts.size(); ++i) {
            const auto& item = s_toasts[i];
            unsigned int elapsed = now - item.startTime;

            // Slide in animation (first 250ms)
            float slideProgress = (elapsed < 250) ? (static_cast<float>(elapsed) / 250.0f) : 1.0f;
            float slideX = (1.0f - slideProgress) * (toastW + marginX);
            float left = sW - toastW - marginX + slideX;
            float right = left + toastW;
            CRect rc(left, curY, right, curY + toastH);

            // Background & Border colors based on variant
            CRGBA bg = theme.card;
            CRGBA borderCol = theme.border;
            CRGBA indicatorCol = theme.primary;

            switch (item.variant) {
            case ToastVariant::Success:
                indicatorCol = CRGBA{34, 197, 94, 255}; // green-500
                break;
            case ToastVariant::Warning:
                indicatorCol = CRGBA{234, 179, 8, 255}; // yellow-500
                break;
            case ToastVariant::Destructive:
                indicatorCol = theme.destructive;
                break;
            default:
                indicatorCol = theme.primary;
                break;
            }

            Renderer::DrawShadow(rc, theme.radiusMd, 8.0f, CRGBA{0, 0, 0, 100});
            Renderer::DrawRoundedRect(rc, bg, theme.radiusMd);
            Renderer::DrawRoundedBorder(rc, borderCol, theme.radiusMd, 1.0f);

            // Left vertical status accent pill
            Renderer::DrawRoundedRect(CRect(rc.left + 4.0f, rc.top + 8.0f, rc.left + 8.0f, rc.bottom - 8.0f), indicatorCol, 2.0f);

            // Title
            CRect titleRc(rc.left + 16.0f, rc.top + 10.0f, rc.right - 12.0f, rc.top + 26.0f);
            FontManager::Draw("", item.title.c_str(), titleRc, theme.cardForeground, ALIGN_LEFT | DT_NOCLIP);

            // Description
            if (!item.description.empty()) {
                CRect descRc(rc.left + 16.0f, rc.top + 28.0f, rc.right - 12.0f, rc.bottom - 8.0f);
                FontManager::Draw("", item.description.c_str(), descRc, theme.mutedForeground, ALIGN_LEFT | DT_NOCLIP);
            }

            curY += toastH + 10.0f;
        }
    }
};

} // namespace sa_ui
