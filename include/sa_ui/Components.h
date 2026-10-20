#pragma once
#include <plugin.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <algorithm>
#include "Theme.h"
#include "Font.h"
#include "Renderer.h"
#include "Input.h"
#include "Image.h"

namespace sa_ui {

enum class ButtonVariant {
    Default,
    Primary,
    Secondary,
    Outline,
    Ghost,
    Destructive,
    Link
};

enum class ButtonSize {
    Sm,
    Md,
    Lg
};

enum class BadgeVariant {
    Default,
    Secondary,
    Outline,
    Destructive
};

// String hash helper for widget IDs
inline unsigned int HashId(const char* str, unsigned int seed = 2166136261u) {
    if (!str) return 0;
    unsigned int hash = seed;
    while (*str) {
        hash = (hash ^ static_cast<unsigned char>(*str++)) * 16777619u;
    }
    return hash;
}

// Global overlay deferred draw list (for Select popovers & Dialogs to stay on top)
struct OverlayItem {
    std::function<void()> drawFunc;
};
inline std::vector<OverlayItem> g_overlayDrawList;

inline void PushOverlay(std::function<void()> drawFunc) {
    g_overlayDrawList.push_back({ drawFunc });
}

inline void FlushOverlays() {
    for (auto& item : g_overlayDrawList) {
        if (item.drawFunc) item.drawFunc();
    }
    g_overlayDrawList.clear();
}

// -------------------------------------------------------------
// 1. CARD
// -------------------------------------------------------------
enum class CardPosition {
    Manual,
    Center,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    LeftCenter,
    RightCenter
};

enum class CardHeaderAlign {
    Left,
    Center,
    Right
};

struct CardBannerProps {
    const char* imageId{nullptr};
    CRGBA color{0, 0, 0, 0};
    float height{70.0f};
    CardHeaderAlign align{CardHeaderAlign::Left};
};

inline CRect ResolveCardRect(CardPosition pos, float width, float height, float marginX = 40.0f, float marginY = 40.0f, CRect manualRect = {}) {
    if (pos == CardPosition::Manual) return manualRect;
    float sW = static_cast<float>(RsGlobal.maximumWidth);
    float sH = static_cast<float>(RsGlobal.maximumHeight);
    float left = marginX;
    float top = marginY;

    switch (pos) {
    case CardPosition::Center:
        left = (sW - width) * 0.5f;
        top = (sH - height) * 0.5f;
        break;
    case CardPosition::TopLeft:
        left = marginX;
        top = marginY;
        break;
    case CardPosition::TopRight:
        left = sW - width - marginX;
        top = marginY;
        break;
    case CardPosition::BottomLeft:
        left = marginX;
        top = sH - height - marginY;
        break;
    case CardPosition::BottomRight:
        left = sW - width - marginX;
        top = sH - height - marginY;
        break;
    case CardPosition::LeftCenter:
        left = marginX;
        top = (sH - height) * 0.5f;
        break;
    case CardPosition::RightCenter:
        left = sW - width - marginX;
        top = (sH - height) * 0.5f;
        break;
    default:
        break;
    }
    return CRect(left, top, left + width, top + height);
}

inline void BeginCard(CRect rect, const char* title = nullptr, const char* description = nullptr, const CardBannerProps& banner = {}) {
    const Theme& t = GetTheme();
    // Drop shadow
    Renderer::DrawShadow(rect, t.radiusLg, 6.0f, CRGBA{0, 0, 0, 80});
    // Card background & border
    Renderer::DrawRoundedRect(rect, t.card, t.radiusLg);
    Renderer::DrawRoundedBorder(rect, t.border, t.radiusLg, 1.0f);

    float curY = rect.top;

    if (banner.height > 0.0f && (banner.imageId || banner.color.a > 0)) {
        CRect bannerRc(rect.left, rect.top, rect.right, rect.top + banner.height);
        if (banner.color.a > 0) {
            Renderer::DrawRoundedRect(bannerRc, banner.color, t.radiusLg);
        }
        if (banner.imageId && banner.imageId[0]) {
            Image(banner.imageId, bannerRc, CRGBA{255, 255, 255, 255}, ImageFit::Cover);
        }
        // Banner bottom divider
        Renderer::DrawRect(CRect(bannerRc.left, bannerRc.bottom - 1.0f, bannerRc.right, bannerRc.bottom), t.border);

        if (title && title[0]) {
            DWORD fontAlign = (banner.align == CardHeaderAlign::Center) ? ALIGN_CENTER : (banner.align == CardHeaderAlign::Right) ? ALIGN_RIGHT : ALIGN_LEFT;
            CRect titleRc(bannerRc.left + 16.0f, bannerRc.top, bannerRc.right - 16.0f, bannerRc.bottom);
            FontManager::DrawGameFont(t.fontHeader, title, titleRc, CRGBA{255, 255, 255, 255}, 0.55f, 1.4f, fontAlign);
        }
        curY = rect.top + banner.height + 12.0f;
    } else {
        curY = rect.top + 16.0f;
        if (title && title[0]) {
            DWORD fontAlign = (banner.align == CardHeaderAlign::Center) ? ALIGN_CENTER : (banner.align == CardHeaderAlign::Right) ? ALIGN_RIGHT : ALIGN_LEFT;
            FontManager::Draw("", title, CRect(rect.left + 16.0f, curY, rect.right - 16.0f, curY + 20.0f), t.cardForeground, fontAlign | DT_NOCLIP);
            curY += 20.0f;
        }
    }

    if (description && description[0]) {
        DWORD fontAlign = (banner.align == CardHeaderAlign::Center) ? ALIGN_CENTER : (banner.align == CardHeaderAlign::Right) ? ALIGN_RIGHT : ALIGN_LEFT;
        FontManager::Draw("", description, CRect(rect.left + 16.0f, curY, rect.right - 16.0f, curY + 16.0f), t.mutedForeground, fontAlign | DT_NOCLIP);
    }
}

// -------------------------------------------------------------
// 2. BUTTON
// -------------------------------------------------------------
inline bool Button(const char* label, CRect rect, ButtonVariant variant = ButtonVariant::Default, ButtonSize size = ButtonSize::Md, bool disabled = false, bool loading = false) {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool isInteractive = !disabled && !loading;

    bool hovered = isInteractive && Input::IsHovered(rect);
    if (hovered) Input::hotId = id;
    if (hovered && Input::mouseLeftDown) Input::activeId = id;

    bool mouseClicked = (Input::activeId == id && Input::mouseLeftUp && hovered);
    if (Input::mouseLeftUp && Input::activeId == id) Input::activeId = 0;

    bool navClicked = isInteractive && isNavFocused && Input::navConfirm;
    bool clicked = (mouseClicked || navClicked);

    CRGBA bg = t.secondary;
    CRGBA fg = t.secondaryForeground;
    CRGBA borderCol = t.border;
    bool drawBorder = false;

    switch (variant) {
    case ButtonVariant::Default:
    case ButtonVariant::Primary:
        bg = hovered ? CRGBA{230, 230, 235, 255} : t.primary;
        fg = t.primaryForeground;
        break;
    case ButtonVariant::Secondary:
        bg = hovered ? CRGBA{55, 55, 60, 255} : t.secondary;
        fg = t.secondaryForeground;
        break;
    case ButtonVariant::Outline:
        bg = hovered ? t.muted : CRGBA{0, 0, 0, 0};
        fg = t.foreground;
        borderCol = t.border;
        drawBorder = true;
        break;
    case ButtonVariant::Ghost:
        bg = hovered ? t.muted : CRGBA{0, 0, 0, 0};
        fg = t.foreground;
        break;
    case ButtonVariant::Destructive:
        bg = hovered ? CRGBA{220, 38, 38, 255} : t.destructive;
        fg = t.destructiveForeground;
        break;
    case ButtonVariant::Link:
        bg = CRGBA{0, 0, 0, 0};
        fg = hovered ? t.primary : t.mutedForeground;
        break;
    }

    if (disabled || loading) {
        bg = t.muted;
        fg = t.mutedForeground;
    }

    float rad = (size == ButtonSize::Sm) ? t.radiusSm : t.radiusMd;
    if (bg.a > 0) {
        Renderer::DrawRoundedRect(rect, bg, rad);
    }
    if (drawBorder || isNavFocused) {
        Renderer::DrawRoundedBorder(rect, isNavFocused ? t.ring : borderCol, rad, isNavFocused ? 2.0f : 1.0f);
    }

    if (loading) {
        // Render 3 animated pulsing dots
        int ticks = static_cast<int>(GetTickCount() / 200);
        char dots[4] = ".";
        if (ticks % 3 == 1) strcpy(dots, "..");
        else if (ticks % 3 == 2) strcpy(dots, "...");
        std::string loadText = std::string(label ? label : "") + " " + dots;
        FontManager::Draw("", loadText.c_str(), rect, fg, ALIGN_CENTER | ALIGN_VCENTER);
    } else {
        // Text rendering with automatic vertical and horizontal centering
        FontManager::Draw("", label, rect, fg, ALIGN_CENTER | ALIGN_VCENTER);
    }

    return clicked;
}

// -------------------------------------------------------------
// 3. INPUT (Single-Line Text Box)
// -------------------------------------------------------------
inline bool InputText(const char* label, char* buffer, size_t bufferSize, CRect rect, const char* placeholder = "") {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool hovered = Input::IsHovered(rect);
    if (hovered) Input::hotId = id;
    if (hovered && Input::mouseLeftDown) {
        Input::focusId = id;
        Input::isEditing = true;
    } else if (!hovered && Input::mouseLeftDown && Input::focusId == id) {
        Input::focusId = 0;
        Input::isEditing = false;
    }

    if (isNavFocused && Input::navConfirm && Input::focusId != id) {
        Input::focusId = id;
        Input::isEditing = true;
    } else if (Input::focusId == id && (Input::keyEnter || Input::navCancel)) {
        Input::focusId = 0;
        Input::isEditing = false;
    }

    bool isFocused = (Input::focusId == id);

    // Keyboard typing logic
    if (isFocused) {
        size_t len = strlen(buffer);
        if (Input::keyBackspace && len > 0) {
            buffer[len - 1] = '\0';
        }
        for (char c : Input::textQueue) {
            if (len + 1 < bufferSize) {
                buffer[len++] = c;
                buffer[len] = '\0';
            }
        }
    }

    // Draw input background & border
    Renderer::DrawRoundedRect(rect, t.card, t.radiusMd);
    if (isFocused) {
        Renderer::DrawRoundedBorder(rect, t.ring, t.radiusMd, 2.0f);
    } else {
        Renderer::DrawRoundedBorder(rect, hovered ? t.mutedForeground : t.input, t.radiusMd, 1.0f);
    }

    // Draw text / placeholder
    float textY = rect.top + (rect.bottom - rect.top) * 0.5f - 7.0f;
    CRect textRc(rect.left + 10.0f, textY, rect.right - 10.0f, rect.bottom);

    if (buffer[0] != '\0') {
        FontManager::Draw("", buffer, textRc, t.foreground, ALIGN_LEFT | DT_NOCLIP);
        // Blinking caret
        if (isFocused && (CTimer::m_snTimeInMilliseconds / 500) % 2 == 0) {
            float textW = 0.0f;
            FontManager::Measure("", buffer, &textW, nullptr);
            float caretX = textRc.left + textW + 2.0f;
            if (caretX < rect.right - 8.0f) {
                Renderer::DrawRect(CRect(caretX, rect.top + 7.0f, caretX + 1.5f, rect.bottom - 7.0f), t.foreground);
            }
        }
    } else if (placeholder && placeholder[0]) {
        FontManager::Draw("", placeholder, textRc, t.mutedForeground, ALIGN_LEFT | DT_NOCLIP);
    }

    return isFocused;
}

// -------------------------------------------------------------
// 4. CHECKBOX
// -------------------------------------------------------------
inline bool Checkbox(const char* label, bool* value, CVector2D pos) {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    CRect box(pos.x, pos.y, pos.x + 18.0f, pos.y + 18.0f);

    float textW = 0.0f;
    FontManager::Measure("", label, &textW, nullptr);
    CRect totalHitbox(pos.x, pos.y, pos.x + 24.0f + textW, pos.y + 18.0f);

    bool hovered = Input::IsHovered(totalHitbox);
    if (hovered) Input::hotId = id;

    bool toggled = false;
    if (hovered && Input::mouseLeftDown) {
        *value = !*value;
        toggled = true;
    }
    if (isNavFocused && Input::navConfirm) {
        *value = !*value;
        toggled = true;
    }

    // Checkbox square
    CRGBA boxBg = *value ? t.primary : t.card;
    CRGBA checkCol = t.primaryForeground;

    Renderer::DrawRoundedRect(box, boxBg, t.radiusSm);
    Renderer::DrawRoundedBorder(box, isNavFocused ? t.ring : (hovered ? t.mutedForeground : t.border), t.radiusSm, isNavFocused ? 2.0f : 1.0f);

    if (*value) {
        Renderer::DrawCheckmark({box.left, box.top}, 16.0f, checkCol, 2.0f);
    }

    // Label
    FontManager::Draw("", label, CRect(pos.x + 26.0f, pos.y, pos.x + 400.0f, pos.y + 18.0f), t.foreground, ALIGN_LEFT | ALIGN_VCENTER);

    return toggled;
}

// -------------------------------------------------------------
// 5. RADIO BUTTON
// -------------------------------------------------------------
inline bool RadioButton(const char* label, int* currentSelection, int value, CVector2D pos) {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label) + value;
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    float radius = 9.0f;
    CVector2D center{ pos.x + radius, pos.y + radius };

    float textW = 0.0f;
    FontManager::Measure("", label, &textW, nullptr);
    CRect totalHitbox(pos.x, pos.y, pos.x + 24.0f + textW, pos.y + 18.0f);

    bool hovered = Input::IsHovered(totalHitbox);
    if (hovered) Input::hotId = id;

    bool selected = (*currentSelection == value);
    bool changed = false;

    if (hovered && Input::mouseLeftDown) {
        *currentSelection = value;
        changed = true;
    }
    if (isNavFocused && Input::navConfirm) {
        *currentSelection = value;
        changed = true;
    }

    // Outer circle and inset fill
    Renderer::DrawDot(center, radius, isNavFocused ? t.ring : (hovered ? t.mutedForeground : t.border));
    Renderer::DrawDot(center, radius - 1.0f, t.card);

    // Inner Dot
    if (selected) {
        Renderer::DrawDot(center, 4.5f, t.primary);
    }

    // Label
    FontManager::Draw("", label, CRect(pos.x + 26.0f, pos.y, pos.x + 400.0f, pos.y + 18.0f), t.foreground, ALIGN_LEFT | ALIGN_VCENTER);

    return changed;
}

// -------------------------------------------------------------
// 6. TABS
// -------------------------------------------------------------
inline int Tabs(const std::vector<std::string>& tabNames, int activeIdx, CRect rect) {
    const Theme& t = GetTheme();
    size_t count = tabNames.size();
    if (count == 0) return activeIdx;
    if (activeIdx < 0) activeIdx = 0;
    if (activeIdx >= static_cast<int>(count)) activeIdx = static_cast<int>(count) - 1;

    if (Input::navTabPrev && count > 0) {
        activeIdx = (activeIdx - 1 + static_cast<int>(count)) % static_cast<int>(count);
    }
    if (Input::navTabNext && count > 0) {
        activeIdx = (activeIdx + 1) % static_cast<int>(count);
    }

    // Tabs container pill
    Renderer::DrawRoundedRect(rect, t.muted, t.radiusMd);

    float padding = 3.0f;
    float tabW = ((rect.right - rect.left) - padding * (count + 1)) / static_cast<float>(count);
    float tabH = (rect.bottom - rect.top) - padding * 2.0f;

    for (size_t i = 0; i < count; ++i) {
        CRect tabRc(
            rect.left + padding + i * (tabW + padding),
            rect.top + padding,
            rect.left + padding + i * (tabW + padding) + tabW,
            rect.top + padding + tabH
        );

        bool isSelected = (static_cast<int>(i) == activeIdx);
        bool hovered = Input::IsHovered(tabRc);

        if (hovered && Input::mouseLeftDown) {
            activeIdx = static_cast<int>(i);
        }

        if (isSelected) {
            Renderer::DrawShadow(tabRc, t.radiusSm, 2.0f, CRGBA{0, 0, 0, 40});
            Renderer::DrawRoundedRect(tabRc, t.card, t.radiusSm);
        }

        CRGBA textCol = isSelected ? t.cardForeground : (hovered ? t.foreground : t.mutedForeground);
        FontManager::Draw("", tabNames[i].c_str(), tabRc, textCol, ALIGN_CENTER | ALIGN_VCENTER);
    }

    return activeIdx;
}

// -------------------------------------------------------------
// 7. COLLAPSIBLE (Accordion)
// -------------------------------------------------------------
inline bool Collapsible(const char* title, bool* isOpen, CRect headerRect) {
    const Theme& t = GetTheme();
    unsigned int id = HashId(title);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool hovered = Input::IsHovered(headerRect);
    if (hovered) Input::hotId = id;

    if (hovered && Input::mouseLeftDown) {
        *isOpen = !*isOpen;
    }
    if (isNavFocused && Input::navConfirm) {
        *isOpen = !*isOpen;
    }

    Renderer::DrawRoundedRect(headerRect, hovered ? t.muted : t.card, t.radiusMd);
    Renderer::DrawRoundedBorder(headerRect, isNavFocused ? t.ring : t.border, t.radiusMd, isNavFocused ? 2.0f : 1.0f);

    // Title
    FontManager::Draw("", title, CRect(headerRect.left + 12.0f, headerRect.top, headerRect.right - 30.0f, headerRect.bottom), t.foreground, ALIGN_LEFT | ALIGN_VCENTER);

    // Arrow indicator
    const char* arrow = *isOpen ? "v" : ">";
    FontManager::Draw("", arrow, CRect(headerRect.right - 26.0f, headerRect.top, headerRect.right - 6.0f, headerRect.bottom), t.mutedForeground, ALIGN_CENTER | ALIGN_VCENTER);

    return *isOpen;
}

// -------------------------------------------------------------
// 8. SELECT / DROPDOWN
// -------------------------------------------------------------
inline bool Select(const char* label, const std::vector<std::string>& options, int* selectedIdx, bool* isOpen, CRect rect, const char* placeholder = "Select option...") {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool hovered = Input::IsHovered(rect);
    if (hovered) Input::hotId = id;

    if (hovered && Input::mouseLeftDown) {
        *isOpen = !*isOpen;
    }
    if (isNavFocused && Input::navConfirm) {
        *isOpen = !*isOpen;
        Input::isEditing = *isOpen;
    }

    if (*isOpen && isNavFocused && !options.empty()) {
        if (Input::navDown) {
            *selectedIdx = (*selectedIdx + 1) % static_cast<int>(options.size());
        } else if (Input::navUp) {
            *selectedIdx = (*selectedIdx - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
        }
    }
    if (*isOpen && Input::navCancel) {
        *isOpen = false;
        Input::isEditing = false;
    }

    if (*isOpen && Input::mouseLeftDown && !hovered) {
        float itemH = 32.0f;
        float menuH = options.size() * itemH + 8.0f;
        CRect menuRc(rect.left, rect.bottom + 4.0f, rect.right, rect.bottom + 4.0f + menuH);
        if (!Input::IsHovered(menuRc)) {
            *isOpen = false;
            Input::isEditing = false;
        }
    }

    // Trigger box
    Renderer::DrawRoundedRect(rect, t.card, t.radiusMd);
    Renderer::DrawRoundedBorder(rect, *isOpen ? t.ring : (isNavFocused ? t.ring : (hovered ? t.mutedForeground : t.input)), t.radiusMd, (isNavFocused || *isOpen) ? 2.0f : 1.0f);

    CRect textRc(rect.left + 12.0f, rect.top, rect.right - 26.0f, rect.bottom);

    if (*selectedIdx >= 0 && *selectedIdx < static_cast<int>(options.size())) {
        FontManager::Draw("", options[*selectedIdx].c_str(), textRc, t.foreground, ALIGN_LEFT | ALIGN_VCENTER);
    } else {
        FontManager::Draw("", placeholder, textRc, t.mutedForeground, ALIGN_LEFT | ALIGN_VCENTER);
    }

    // Caret icon
    FontManager::Draw("", "v", CRect(rect.right - 22.0f, rect.top, rect.right - 6.0f, rect.bottom), t.mutedForeground, ALIGN_CENTER | ALIGN_VCENTER);

    bool changed = false;

    // Resolve selection before enqueueing the draw callback. The callback runs
    // during EndFrame, after this function has returned, so it must not write
    // through a reference to this stack-local result variable.
    if (*isOpen && Input::mouseLeftDown) {
        float itemH = 32.0f;
        float menuH = options.size() * itemH + 8.0f;
        CRect menuRc(rect.left, rect.bottom + 4.0f, rect.right, rect.bottom + 4.0f + menuH);
        for (size_t i = 0; i < options.size(); ++i) {
            CRect itemRc(menuRc.left + 4.0f, menuRc.top + 4.0f + i * itemH, menuRc.right - 4.0f, menuRc.top + 4.0f + (i + 1) * itemH);
            if (Input::IsHovered(itemRc)) {
                *selectedIdx = static_cast<int>(i);
                *isOpen = false;
                changed = true;
                break;
            }
        }
    }

    // Popover dropdown list (rendered in overlay layer to stay on top)
    if (*isOpen) {
        float itemH = 32.0f;
        float menuH = options.size() * itemH + 8.0f;
        CRect menuRc(rect.left, rect.bottom + 4.0f, rect.right, rect.bottom + 4.0f + menuH);

        PushOverlay([=]() {
            Renderer::DrawShadow(menuRc, t.radiusMd, 8.0f, CRGBA{0, 0, 0, 120});
            Renderer::DrawRoundedRect(menuRc, t.popover, t.radiusMd);
            Renderer::DrawRoundedBorder(menuRc, t.border, t.radiusMd, 1.0f);

            for (size_t i = 0; i < options.size(); ++i) {
                CRect itemRc(menuRc.left + 4.0f, menuRc.top + 4.0f + i * itemH, menuRc.right - 4.0f, menuRc.top + 4.0f + (i + 1) * itemH);
                bool itemHover = Input::IsHovered(itemRc);

                if (itemHover) {
                    Renderer::DrawRoundedRect(itemRc, t.accent, t.radiusSm);
                }

                CRGBA col = (static_cast<int>(i) == *selectedIdx) ? t.primary : (itemHover ? t.accentForeground : t.popoverForeground);
                FontManager::Draw("", options[i].c_str(), CRect(itemRc.left + 10.0f, itemRc.top, itemRc.right - 10.0f, itemRc.bottom), col, ALIGN_LEFT | ALIGN_VCENTER);
            }
        });
    }

    return changed;
}

// -------------------------------------------------------------
// 9. DIALOG (Modal)
// -------------------------------------------------------------
inline void Dialog(const char* title, bool* isOpen, const std::function<void(CRect contentRect)>& contentCallback, float width = 420.0f, float height = 280.0f) {
    if (!*isOpen) return;
    const Theme& t = GetTheme();

    PushOverlay([=]() {
        // Backdrop overlay
        float sW = (float)RsGlobal.maximumWidth;
        float sH = (float)RsGlobal.maximumHeight;
        CRect screenRc(0.0f, 0.0f, sW, sH);
        Renderer::DrawRect(screenRc, CRGBA{0, 0, 0, 160});

        // Centered Card
        float left = (sW - width) * 0.5f;
        float top = (sH - height) * 0.5f;
        CRect dialogRc(left, top, left + width, top + height);

        Renderer::DrawShadow(dialogRc, t.radiusLg, 16.0f, CRGBA{0, 0, 0, 140});
        Renderer::DrawRoundedRect(dialogRc, t.card, t.radiusLg);
        Renderer::DrawRoundedBorder(dialogRc, t.border, t.radiusLg, 1.0f);

        // Header Title
        FontManager::Draw("", title, CRect(dialogRc.left + 20.0f, dialogRc.top + 14.0f, dialogRc.right - 50.0f, dialogRc.top + 40.0f), t.cardForeground, ALIGN_LEFT | ALIGN_VCENTER);

        // Close button (X)
        CRect closeRc(dialogRc.right - 36.0f, dialogRc.top + 14.0f, dialogRc.right - 14.0f, dialogRc.top + 36.0f);
        if (Button("X", closeRc, ButtonVariant::Ghost, ButtonSize::Sm)) {
            *isOpen = false;
        }

        // Content slot
        CRect contentArea(dialogRc.left + 20.0f, dialogRc.top + 55.0f, dialogRc.right - 20.0f, dialogRc.bottom - 20.0f);
        if (contentCallback) {
            contentCallback(contentArea);
        }
    });
}

// -------------------------------------------------------------
// 10. TEXTAREA (Multi-Line Text Box)
// -------------------------------------------------------------
inline bool Textarea(const char* label, char* buffer, size_t bufferSize, CRect rect, const char* placeholder = "") {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label);
    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool hovered = Input::IsHovered(rect);
    if (hovered) Input::hotId = id;
    if (hovered && Input::mouseLeftDown) {
        Input::focusId = id;
        Input::isEditing = true;
    } else if (!hovered && Input::mouseLeftDown && Input::focusId == id) {
        Input::focusId = 0;
        Input::isEditing = false;
    }

    if (isNavFocused && Input::navConfirm && Input::focusId != id) {
        Input::focusId = id;
        Input::isEditing = true;
    } else if (Input::focusId == id && !Input::keyShift && (Input::keyEnter || Input::navCancel)) {
        Input::focusId = 0;
        Input::isEditing = false;
    }

    bool isFocused = (Input::focusId == id);

    if (isFocused) {
        size_t len = strlen(buffer);
        if (Input::keyBackspace && len > 0) {
            buffer[len - 1] = '\0';
        }
        // Shift+Enter creates line break, single Enter submits & closes editing
        if (Input::keyEnter && Input::keyShift && len + 1 < bufferSize) {
            buffer[len++] = '\n';
            buffer[len] = '\0';
        }
        for (char c : Input::textQueue) {
            if (len + 1 < bufferSize) {
                buffer[len++] = c;
                buffer[len] = '\0';
            }
        }
    }

    Renderer::DrawRoundedRect(rect, t.card, t.radiusMd);
    Renderer::DrawRoundedBorder(rect, isFocused ? t.ring : (isNavFocused ? t.ring : (hovered ? t.mutedForeground : t.input)), t.radiusMd, isFocused || isNavFocused ? 2.0f : 1.0f);

    CRect textRc(rect.left + 10.0f, rect.top + 8.0f, rect.right - 10.0f, rect.bottom - 8.0f);
    if (buffer[0] != '\0') {
        FontManager::Draw("", buffer, textRc, t.foreground, ALIGN_LEFT);
    } else if (placeholder && placeholder[0]) {
        FontManager::Draw("", placeholder, textRc, t.mutedForeground, ALIGN_LEFT);
    }

    return isFocused;
}

// -------------------------------------------------------------
// 11. SLIDER
// -------------------------------------------------------------
enum class SliderType {
    Number,
    Percent
};

struct SliderProps {
    SliderType type{SliderType::Number};
    float min{0.0f};
    float max{0.0f};      // If min == max: Number default 0..100, Percent default 0..1
    float step{0.0f};     // <= 0: Number default 1.0, Percent default 0.05
    float defaultValue{0.0f};
    bool hasDefault{false};
    float width{0.0f};
    float height{34.0f};
    bool showValue{true};
    const char* format{nullptr};
};

inline bool Slider(const char* label, float* value, const SliderProps& props, CRect rect) {
    if (!value) return false;
    const Theme& t = GetTheme();
    unsigned int id = HashId(label ? label : "Slider");
    Input::navList.push_back(id);
    bool isNavFocused = (Input::navFocusId == id);

    float minVal = props.min;
    float maxVal = props.max;
    if (minVal == maxVal) {
        if (props.type == SliderType::Percent) {
            minVal = 0.0f;
            maxVal = 1.0f;
        } else {
            minVal = 0.0f;
            maxVal = 100.0f;
        }
    }
    if (maxVal < minVal) {
        std::swap(minVal, maxVal);
    }

    float step = props.step;
    if (step <= 0.0f) {
        step = (props.type == SliderType::Percent) ? 0.05f : 1.0f;
    }

    if (*value < minVal) *value = minVal;
    if (*value > maxVal) *value = maxVal;

    const bool isTall = (rect.bottom - rect.top >= 30.0f);
    const bool showHeader = isTall && ((label && label[0] != '\0') || props.showValue);

    CRect trackRc;
    unsigned int pillId = id ^ 0x12345678;
    static char s_sliderInputBuf[32] = "";
    static unsigned int s_activeSliderEditId = 0;

    if (showHeader) {
        CRect headerRc(rect.left, rect.top, rect.right, rect.top + 16.0f);
        if (label && label[0] != '\0') {
            FontManager::Draw("", label, headerRc, isNavFocused ? t.primary : t.foreground, ALIGN_LEFT | DT_NOCLIP);
        }
        if (props.showValue) {
            float pillW = 60.0f;
            CRect valRc(rect.right - pillW, headerRc.top - 2.0f, rect.right, headerRc.bottom + 2.0f);

            if (s_activeSliderEditId == pillId) {
                // Inline InputText editing
                InputText("##slider_val", s_sliderInputBuf, sizeof(s_sliderInputBuf), valRc);

                // Up / down arrow stepping when focused
                if (Input::keyUp) {
                    float v = static_cast<float>(atof(s_sliderInputBuf));
                    v += (Input::keyShift ? step * 10.0f : step);
                    if (v > maxVal) v = maxVal;
                    *value = v;
                    snprintf(s_sliderInputBuf, sizeof(s_sliderInputBuf), props.type == SliderType::Percent ? "%.2f" : (step >= 1.0f ? "%.0f" : "%.2f"), *value);
                }
                if (Input::keyDown) {
                    float v = static_cast<float>(atof(s_sliderInputBuf));
                    v -= (Input::keyShift ? step * 10.0f : step);
                    if (v < minVal) v = minVal;
                    *value = v;
                    snprintf(s_sliderInputBuf, sizeof(s_sliderInputBuf), props.type == SliderType::Percent ? "%.2f" : (step >= 1.0f ? "%.0f" : "%.2f"), *value);
                }

                if (Input::keyEnter || (Input::mouseLeftDown && !Input::IsHovered(valRc))) {
                    float v = static_cast<float>(atof(s_sliderInputBuf));
                    if (props.type == SliderType::Percent && v > 1.0f && maxVal <= 1.0f) {
                        v /= 100.0f; // Handle player typing 50 instead of 0.50
                    }
                    if (v < minVal) v = minVal;
                    if (v > maxVal) v = maxVal;
                    *value = v;
                    s_activeSliderEditId = 0;
                    Input::focusId = 0;
                    Input::isEditing = false;
                }
            } else {
                bool pillHovered = Input::IsHovered(valRc);
                if (pillHovered && Input::mouseLeftDown) {
                    s_activeSliderEditId = pillId;
                    snprintf(s_sliderInputBuf, sizeof(s_sliderInputBuf), props.type == SliderType::Percent ? "%.2f" : (step >= 1.0f ? "%.0f" : "%.2f"), *value);
                    Input::focusId = HashId("##slider_val");
                    Input::isEditing = true;
                    Input::textQueue.clear();
                }

                Renderer::DrawRoundedRect(valRc, pillHovered ? t.secondary : t.card, t.radiusSm);
                Renderer::DrawRoundedBorder(valRc, pillHovered ? t.primary : t.border, t.radiusSm, 1.0f);

                char valBuf[32];
                if (props.format) {
                    snprintf(valBuf, sizeof(valBuf), props.format, *value);
                } else if (props.type == SliderType::Percent) {
                    int pct = static_cast<int>(std::round(*value * 100.0f));
                    snprintf(valBuf, sizeof(valBuf), "%d%%", pct);
                } else {
                    if (step >= 1.0f) {
                        snprintf(valBuf, sizeof(valBuf), "%.0f", *value);
                    } else if (step >= 0.1f) {
                        snprintf(valBuf, sizeof(valBuf), "%.1f", *value);
                    } else {
                        snprintf(valBuf, sizeof(valBuf), "%.2f", *value);
                    }
                }
                FontManager::Draw("", valBuf, valRc, pillHovered ? t.primary : t.mutedForeground, ALIGN_CENTER | ALIGN_VCENTER);
            }
        }
        float midY = rect.top + 24.0f;
        trackRc = CRect(rect.left, midY - 3.0f, rect.right, midY + 3.0f);
    } else {
        float midY = rect.top + (rect.bottom - rect.top) * 0.5f;
        trackRc = CRect(rect.left, midY - 3.0f, rect.right, midY + 3.0f);
    }

    bool hovered = Input::IsHovered(rect);
    if (hovered) Input::hotId = id;

    // Double click to snap to default value
    static unsigned int s_lastClickTime = 0;
    static unsigned int s_lastClickId = 0;
    if (hovered && Input::mouseLeftDown && props.hasDefault) {
        unsigned int now = GetTickCount();
        if (s_lastClickId == id && (now - s_lastClickTime) < 300) {
            *value = props.defaultValue;
        }
        s_lastClickId = id;
        s_lastClickTime = now;
    }

    if (hovered && Input::mouseLeftDown) {
        Input::activeId = id;
    }
    if (Input::mouseLeftUp && Input::activeId == id) {
        Input::activeId = 0;
    }

    // Keyboard / Controller modification
    if (isNavFocused) {
        if (Input::navLeft) {
            float raw = *value - step;
            if (raw < minVal) raw = minVal;
            *value = raw;
        }
        if (Input::navRight) {
            float raw = *value + step;
            if (raw > maxVal) raw = maxVal;
            *value = raw;
        }
    }

    bool dragging = (Input::activeId == id);
    float oldVal = *value;

    if (dragging) {
        float trackWidth = trackRc.right - trackRc.left;
        float normalized = trackWidth > 0.0f ? (Input::mousePos.x - trackRc.left) / trackWidth : 0.0f;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;

        float raw = minVal + normalized * (maxVal - minVal);
        if (step > 0.0f) {
            float steps = std::round((raw - minVal) / step);
            raw = minVal + steps * step;
        }

        // Magnetic snap to default value if near
        if (props.hasDefault && std::abs(raw - props.defaultValue) <= step * 0.5f) {
            raw = props.defaultValue;
        }

        if (raw < minVal) raw = minVal;
        if (raw > maxVal) raw = maxVal;
        *value = raw;
    }

    if (hovered && Input::mouseWheel != 0) {
        float raw = *value + (Input::mouseWheel > 0 ? step : -step);
        if (step > 0.0f) {
            float steps = std::round((raw - minVal) / step);
            raw = minVal + steps * step;
        }
        if (props.hasDefault && std::abs(raw - props.defaultValue) <= step * 0.5f) {
            raw = props.defaultValue;
        }
        if (raw < minVal) raw = minVal;
        if (raw > maxVal) raw = maxVal;
        *value = raw;
    }

    float valueRange = maxVal - minVal;
    float norm = valueRange > 0.0f ? (*value - minVal) / valueRange : 0.0f;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    // Track Background
    Renderer::DrawRoundedRect(trackRc, t.secondary, 3.0f);
    if (isNavFocused) {
        Renderer::DrawRoundedBorder(CRect(trackRc.left - 2.0f, trackRc.top - 2.0f, trackRc.right + 2.0f, trackRc.bottom + 2.0f), t.ring, 4.0f, 1.5f);
    }

    // Track Filled Bar
    CRect fillRc(trackRc.left, trackRc.top, trackRc.left + norm * (trackRc.right - trackRc.left), trackRc.bottom);
    Renderer::DrawRoundedRect(fillRc, t.primary, 3.0f);

    // Thumb Knob
    float thumbY = trackRc.top + (trackRc.bottom - trackRc.top) * 0.5f;
    CVector2D thumbCenter{ trackRc.left + norm * (trackRc.right - trackRc.left), thumbY };
    Renderer::DrawShadow(CRect(thumbCenter.x - 7.0f, thumbCenter.y - 7.0f, thumbCenter.x + 7.0f, thumbCenter.y + 7.0f), 7.0f, 2.0f, CRGBA{0, 0, 0, 80});
    Renderer::DrawDot(thumbCenter, 7.5f, isNavFocused ? t.ring : t.border);
    Renderer::DrawDot(thumbCenter, 6.5f, (dragging || hovered || isNavFocused) ? t.foreground : t.primary);

    return (std::abs(oldVal - *value) > 1e-6f) || dragging;
}

inline bool Slider(const char* label, float* value, float minVal, float maxVal, CRect rect) {
    SliderProps props;
    props.type = SliderType::Number;
    props.min = minVal;
    props.max = maxVal;
    props.step = 0.0f;
    props.showValue = false;
    return Slider(label, value, props, rect);
}

// -------------------------------------------------------------
// 12. BADGE
// -------------------------------------------------------------
inline void Badge(const char* label, CRect rect, BadgeVariant variant = BadgeVariant::Default) {
    const Theme& t = GetTheme();
    CRGBA bg = t.primary;
    CRGBA fg = t.primaryForeground;
    bool drawBorder = false;

    switch (variant) {
    case BadgeVariant::Default:
        bg = t.primary;
        fg = t.primaryForeground;
        break;
    case BadgeVariant::Secondary:
        bg = t.secondary;
        fg = t.secondaryForeground;
        break;
    case BadgeVariant::Outline:
        bg = CRGBA{0, 0, 0, 0};
        fg = t.foreground;
        drawBorder = true;
        break;
    case BadgeVariant::Destructive:
        bg = t.destructive;
        fg = t.destructiveForeground;
        break;
    }

    Renderer::DrawRoundedRect(rect, bg, t.radiusSm);
    if (drawBorder) {
        Renderer::DrawRoundedBorder(rect, t.border, t.radiusSm, 1.0f);
    }

    FontManager::Draw("", label, rect, fg, ALIGN_CENTER | ALIGN_VCENTER);
}

// -------------------------------------------------------------
// 13. SEPARATOR
// -------------------------------------------------------------
inline void Separator(CVector2D start, float width) {
    const Theme& t = GetTheme();
    Renderer::DrawRect(CRect(start.x, start.y, start.x + width, start.y + 1.0f), t.border);
}

// -------------------------------------------------------------
// 14. INDEX SELECT / STEPPER (Enum left-right switch)
// -------------------------------------------------------------
struct IndexSelectProps {
    float width{0.0f};
    float height{32.0f};
    bool wrap{false}; // Loop disabled by default
    bool disabled{false};
};

inline bool IndexSelect(const char* label, int* selectedIndex, const char* const* items, int itemCount, CRect rect, const IndexSelectProps& props = {}) {
    if (!selectedIndex || itemCount <= 0 || !items) return false;
    const Theme& t = GetTheme();
    unsigned int id = HashId(label ? label : "IndexSelect");
    Input::navList.push_back(id);
    bool isNavFocused = (Input::navFocusId == id);

    unsigned int prevId = id ^ 0x55AA55AA;
    unsigned int nextId = id ^ 0xAA55AA55;

    if (*selectedIndex < 0) *selectedIndex = 0;
    if (*selectedIndex >= itemCount) *selectedIndex = itemCount - 1;
    int oldIdx = *selectedIndex;

    const bool hasLabel = (label && label[0] != '\0');
    float totalW = rect.right - rect.left;
    float boxW = hasLabel ? (std::min)(totalW * 0.55f, 240.0f) : totalW;

    if (hasLabel) {
        CRect labelRc(rect.left, rect.top, rect.right - boxW - 8.0f, rect.bottom);
        FontManager::Draw("", label, labelRc, props.disabled ? t.mutedForeground : t.foreground, ALIGN_LEFT | ALIGN_VCENTER);
    }

    CRect boxRc(rect.right - boxW, rect.top, rect.right, rect.bottom);
    bool boxHovered = !props.disabled && Input::IsHovered(boxRc);

    Renderer::DrawRoundedRect(boxRc, t.card, t.radiusMd);
    Renderer::DrawRoundedBorder(boxRc, isNavFocused ? t.ring : (boxHovered ? t.mutedForeground : t.border), t.radiusMd, isNavFocused ? 2.0f : 1.0f);

    float btnW = boxRc.bottom - boxRc.top;
    CRect leftBtnRc(boxRc.left, boxRc.top, boxRc.left + btnW, boxRc.bottom);
    CRect rightBtnRc(boxRc.right - btnW, boxRc.top, boxRc.right, boxRc.bottom);
    CRect centerRc(boxRc.left + btnW, boxRc.top, boxRc.right - btnW, boxRc.bottom);

    // Left button interaction
    bool leftHovered = !props.disabled && Input::IsHovered(leftBtnRc);
    if (leftHovered) Input::hotId = prevId;
    if (leftHovered && Input::mouseLeftDown) Input::activeId = prevId;
    bool leftClicked = (Input::activeId == prevId && Input::mouseLeftUp && leftHovered);
    if (Input::mouseLeftUp && Input::activeId == prevId) Input::activeId = 0;

    // Right button interaction
    bool rightHovered = !props.disabled && Input::IsHovered(rightBtnRc);
    if (rightHovered) Input::hotId = nextId;
    if (rightHovered && Input::mouseLeftDown) Input::activeId = nextId;
    bool rightClicked = (Input::activeId == nextId && Input::mouseLeftUp && rightHovered);
    if (Input::mouseLeftUp && Input::activeId == nextId) Input::activeId = 0;

    // Mouse wheel cycling over box
    bool wheelPrev = boxHovered && Input::mouseWheel < 0;
    bool wheelNext = boxHovered && Input::mouseWheel > 0;

    // Gamepad / Keyboard navigation left/right when focused
    bool navP = isNavFocused && Input::navLeft;
    bool navN = isNavFocused && Input::navRight;

    bool prevTrigger = leftClicked || wheelPrev || navP;
    bool nextTrigger = rightClicked || wheelNext || navN;

    if (!props.disabled) {
        if (prevTrigger) {
            if (*selectedIndex > 0) {
                (*selectedIndex)--;
            } else if (props.wrap) {
                *selectedIndex = itemCount - 1;
            }
        }
        if (nextTrigger) {
            if (*selectedIndex < itemCount - 1) {
                (*selectedIndex)++;
            } else if (props.wrap) {
                *selectedIndex = 0;
            }
        }
    }

    // Render left chevron button
    if (leftHovered && !props.disabled) {
        Renderer::DrawRoundedRect(leftBtnRc, t.accent, t.radiusSm);
    }
    bool canPrev = props.wrap || *selectedIndex > 0;
    CRGBA leftFg = props.disabled || !canPrev ? t.muted : (leftHovered ? t.foreground : t.mutedForeground);
    FontManager::Draw("", "<", leftBtnRc, leftFg, ALIGN_CENTER | ALIGN_VCENTER);

    // Render right chevron button
    if (rightHovered && !props.disabled) {
        Renderer::DrawRoundedRect(rightBtnRc, t.accent, t.radiusSm);
    }
    bool canNext = props.wrap || *selectedIndex < itemCount - 1;
    CRGBA rightFg = props.disabled || !canNext ? t.muted : (rightHovered ? t.foreground : t.mutedForeground);
    FontManager::Draw("", ">", rightBtnRc, rightFg, ALIGN_CENTER | ALIGN_VCENTER);

    // Render selected item text in center
    const char* currentText = items[*selectedIndex];
    FontManager::Draw("", currentText ? currentText : "", centerRc, props.disabled ? t.mutedForeground : t.foreground, ALIGN_CENTER | ALIGN_VCENTER);

    return *selectedIndex != oldIdx;
}

inline bool IndexSelect(const char* label, int* selectedIndex, const std::vector<std::string>& items, CRect rect, const IndexSelectProps& props = {}) {
    std::vector<const char*> ptrs;
    ptrs.reserve(items.size());
    for (const auto& s : items) ptrs.push_back(s.c_str());
    return IndexSelect(label, selectedIndex, ptrs.data(), static_cast<int>(ptrs.size()), rect, props);
}

inline bool IndexSelect(const char* label, int* selectedIndex, std::initializer_list<const char*> items, CRect rect, const IndexSelectProps& props = {}) {
    std::vector<const char*> ptrs(items);
    return IndexSelect(label, selectedIndex, ptrs.data(), static_cast<int>(ptrs.size()), rect, props);
}

inline bool Stepper(const char* label, int* selectedIndex, const char* const* items, int itemCount, CRect rect, const IndexSelectProps& props = {}) {
    return IndexSelect(label, selectedIndex, items, itemCount, rect, props);
}

inline bool Stepper(const char* label, int* selectedIndex, const std::vector<std::string>& items, CRect rect, const IndexSelectProps& props = {}) {
    return IndexSelect(label, selectedIndex, items, rect, props);
}

inline bool Stepper(const char* label, int* selectedIndex, std::initializer_list<const char*> items, CRect rect, const IndexSelectProps& props = {}) {
    return IndexSelect(label, selectedIndex, items, rect, props);
}

inline bool Stepper(const char* label, int* value, int minVal, int maxVal, CRect rect, int step = 1, bool wrap = false) {
    if (!value) return false;
    char buf[32];
    if (maxVal < minVal) std::swap(minVal, maxVal);
    if (*value < minVal) *value = minVal;
    if (*value > maxVal) *value = maxVal;
    int oldVal = *value;

    const bool hasLabel = (label && label[0] != '\0');
    float totalW = rect.right - rect.left;
    float boxW = hasLabel ? (std::min)(totalW * 0.55f, 240.0f) : totalW;

    if (hasLabel) {
        CRect labelRc(rect.left, rect.top, rect.right - boxW - 8.0f, rect.bottom);
        FontManager::Draw("", label, labelRc, GetTheme().foreground, ALIGN_LEFT | ALIGN_VCENTER);
    }

    CRect boxRc(rect.right - boxW, rect.top, rect.right, rect.bottom);
    const Theme& t = GetTheme();
    bool boxHovered = Input::IsHovered(boxRc);

    Renderer::DrawRoundedRect(boxRc, t.card, t.radiusMd);
    Renderer::DrawRoundedBorder(boxRc, boxHovered ? t.mutedForeground : t.border, t.radiusMd, 1.0f);

    float btnW = boxRc.bottom - boxRc.top;
    CRect leftBtnRc(boxRc.left, boxRc.top, boxRc.left + btnW, boxRc.bottom);
    CRect rightBtnRc(boxRc.right - btnW, boxRc.top, boxRc.right, boxRc.bottom);
    CRect centerRc(boxRc.left + btnW, boxRc.top, boxRc.right - btnW, boxRc.bottom);

    unsigned int id = HashId(label ? label : "Stepper");
    unsigned int prevId = id ^ 0x55AA55AA;
    unsigned int nextId = id ^ 0xAA55AA55;

    bool leftHovered = Input::IsHovered(leftBtnRc);
    if (leftHovered) Input::hotId = prevId;
    if (leftHovered && Input::mouseLeftDown) Input::activeId = prevId;
    bool leftClicked = (Input::activeId == prevId && Input::mouseLeftUp && leftHovered);
    if (Input::mouseLeftUp && Input::activeId == prevId) Input::activeId = 0;

    bool rightHovered = Input::IsHovered(rightBtnRc);
    if (rightHovered) Input::hotId = nextId;
    if (rightHovered && Input::mouseLeftDown) Input::activeId = nextId;
    bool rightClicked = (Input::activeId == nextId && Input::mouseLeftUp && rightHovered);
    if (Input::mouseLeftUp && Input::activeId == nextId) Input::activeId = 0;

    bool wheelPrev = boxHovered && Input::mouseWheel < 0;
    bool wheelNext = boxHovered && Input::mouseWheel > 0;

    if (leftClicked || wheelPrev) {
        if (*value - step >= minVal) *value -= step;
        else if (wrap) *value = maxVal;
    }
    if (rightClicked || wheelNext) {
        if (*value + step <= maxVal) *value += step;
        else if (wrap) *value = minVal;
    }

    if (leftHovered) Renderer::DrawRoundedRect(leftBtnRc, t.accent, t.radiusSm);
    FontManager::Draw("", "<", leftBtnRc, leftHovered ? t.foreground : t.mutedForeground, ALIGN_CENTER | ALIGN_VCENTER);

    if (rightHovered) Renderer::DrawRoundedRect(rightBtnRc, t.accent, t.radiusSm);
    FontManager::Draw("", ">", rightBtnRc, rightHovered ? t.foreground : t.mutedForeground, ALIGN_CENTER | ALIGN_VCENTER);

    snprintf(buf, sizeof(buf), "%d", *value);
    FontManager::Draw("", buf, centerRc, t.foreground, ALIGN_CENTER | ALIGN_VCENTER);

    return *value != oldVal;
}

// -------------------------------------------------------------
// 15. MENU ITEM (Full-width row, flex justify-between)
// -------------------------------------------------------------
struct MenuItemProps {
    float width{0.0f};              // 0.0f = fill container width
    float height{36.0f};            // Standard menu row height
    float controlWidth{0.0f};       // 0.0f = auto
    bool disabled{false};
    bool wrap{false};               // false = marquee scroll after 500ms; true = wrap
    bool showDivider{false};
    const char* valueText{nullptr}; // optional right value text (e.g. "ON", ">", "100%")
    CRGBA valueColor{0, 0, 0, 0};   // 0 alpha = auto mutedForeground
};

struct MenuProps {
    CRect rect{0.0f, 0.0f, 0.0f, 0.0f};
    CardPosition position{CardPosition::TopLeft};
    float marginX{40.0f};
    float marginY{40.0f};
    float width{380.0f};
    int maxVisible{8};              // Number of rows visible in viewport
    float itemHeight{36.0f};        // Height per row
    float padding{10.0f};
    const char* title{"TRAINER"};
    const char* subtitle{nullptr};
    CardBannerProps banner{};
    bool showFooter{true};
};

struct MenuContextState {
    unsigned int menuId{0};
    int totalItems{0};
    int currentItemIndex{0};
    int selectedIndex{0};
    int scrollOffset{0};
    int maxVisible{8};
    CRect contentRect{0.0f, 0.0f, 0.0f, 0.0f};
    float itemHeight{36.0f};
};

inline MenuContextState* g_activeMenu = nullptr;

struct MenuState {
    int selectedIndex{0};
    int scrollOffset{0};
    int totalItems{0};
};
inline std::unordered_map<unsigned int, MenuState> g_menuStates;

inline bool MenuItem(const char* label, CRect rect, const MenuItemProps& props = {}, const std::function<void(CRect rightRect)>& control = nullptr) {
    const Theme& t = GetTheme();
    unsigned int id = HashId(label ? label : "MenuItem");

    if (g_activeMenu) {
        int idx = g_activeMenu->currentItemIndex++;
        if (idx < g_activeMenu->scrollOffset || idx >= g_activeMenu->scrollOffset + g_activeMenu->maxVisible) {
            return false;
        }
        int visibleRow = idx - g_activeMenu->scrollOffset;
        float itemY = g_activeMenu->contentRect.top + visibleRow * g_activeMenu->itemHeight;
        rect = CRect(g_activeMenu->contentRect.left, itemY, g_activeMenu->contentRect.right, itemY + g_activeMenu->itemHeight);
        if (idx == g_activeMenu->selectedIndex) {
            Input::navFocusId = id;
        }
    }

    Input::navList.push_back(id);

    bool isNavFocused = (Input::navFocusId == id);
    bool isInteractive = !props.disabled;

    bool hovered = isInteractive && Input::IsHovered(rect);
    if (hovered) Input::hotId = id;
    if (hovered && Input::mouseLeftDown) Input::activeId = id;

    bool mouseClicked = (Input::activeId == id && Input::mouseLeftUp && hovered);
    if (Input::mouseLeftUp && Input::activeId == id) Input::activeId = 0;

    bool navClicked = isInteractive && isNavFocused && Input::navConfirm;
    bool clicked = (mouseClicked || navClicked);

    // Row background (hover / focus styling)
    if (isNavFocused) {
        Renderer::DrawRoundedRect(rect, t.accent, t.radiusMd);
        Renderer::DrawRoundedBorder(rect, t.ring, t.radiusMd, 1.5f);
    } else if (hovered) {
        Renderer::DrawRoundedRect(rect, t.accent, t.radiusMd);
    }

    if (props.showDivider) {
        Renderer::DrawRect(CRect(rect.left + 8.0f, rect.bottom - 1.0f, rect.right - 8.0f, rect.bottom), t.border);
    }

    const float padX = 12.0f;
    const float totalW = rect.right - rect.left;
    const float totalH = rect.bottom - rect.top;

    // Right control slot layout (aligned right)
    float rightW = props.controlWidth > 0.0f ? props.controlWidth : (control ? (std::min)(totalW * 0.45f, 150.0f) : (props.valueText ? 100.0f : 0.0f));
    CRect rightRect(rect.right - padX - rightW, rect.top + 2.0f, rect.right - padX, rect.bottom - 2.0f);

    if (control) {
        control(rightRect);
    } else if (props.valueText && props.valueText[0] != '\0') {
        CRGBA valCol = props.valueColor.a > 0 ? props.valueColor : (props.disabled ? t.mutedForeground : (hovered || isNavFocused ? t.foreground : t.mutedForeground));
        FontManager::DrawGameFont(GameFontStyle::Menu, props.valueText, rightRect, valCol, 0.32f, 0.88f, ALIGN_RIGHT);
    }

    // Left label layout (font MENU, align left)
    float labelLeft = rect.left + padX;
    float labelRight = rightW > 0.0f ? (rightRect.left - 8.0f) : (rect.right - padX);
    if (labelRight < labelLeft) labelRight = labelLeft;
    CRect labelClipRect(labelLeft, rect.top, labelRight, rect.bottom);

    if (label && label[0] != '\0') {
        CRGBA labelCol = props.disabled ? t.mutedForeground : (hovered || isNavFocused ? t.foreground : t.foreground);
        float textY = rect.top + (totalH - 16.0f) * 0.5f;

        // Measure text width using CFont MENU
        CFont::SetScale(0.35f, 0.95f);
        CFont::SetProportional(true);
        CFont::SetFontStyle(FONT_MENU);
        float textW = CFont::GetStringWidth(label, true);
        float availW = labelRight - labelLeft;

        if (textW > availW && !props.wrap) {
            // Marquee mode (scrolling when hovered/focused for at least 500ms)
            static std::unordered_map<unsigned int, unsigned int> s_hoverStartMap;
            unsigned int now = GetTickCount();

            float scrollOffset = 0.0f;
            if ((hovered || isNavFocused) && !props.disabled) {
                auto it = s_hoverStartMap.find(id);
                if (it == s_hoverStartMap.end()) {
                    s_hoverStartMap[id] = now;
                }
                unsigned int elapsed = now - s_hoverStartMap[id];
                if (elapsed >= 500) {
                    float scrollPixels = static_cast<float>(elapsed - 500) * 0.05f; // 50px/sec
                    float maxScroll = textW - availW + 24.0f;
                    scrollOffset = std::fmod(scrollPixels, maxScroll + 30.0f);
                    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
                }
            } else {
                s_hoverStartMap.erase(id);
            }

            Renderer::PushClipRect(labelClipRect);
            CRect scrollRc(labelLeft - scrollOffset, textY, labelLeft - scrollOffset + textW + 10.0f, rect.bottom);
            FontManager::DrawGameFont(GameFontStyle::Menu, label, scrollRc, labelCol, 0.35f, 0.95f, ALIGN_LEFT);
            CFont::RenderFontBuffer(); // Flush CFont buffer under active D3D9 scissor rect
            Renderer::PopClipRect();
        } else if (textW > availW && props.wrap) {
            CFont::SetWrapx(labelRight);
            CRect wrapRc(labelLeft, textY, labelRight, rect.bottom);
            FontManager::DrawGameFont(GameFontStyle::Menu, label, wrapRc, labelCol, 0.35f, 0.95f, ALIGN_LEFT);
        } else {
            CRect normRc(labelLeft, textY, labelRight, rect.bottom);
            FontManager::DrawGameFont(GameFontStyle::Menu, label, normRc, labelCol, 0.35f, 0.95f, ALIGN_LEFT);
        }
    }

    return clicked;
}

inline bool MenuItem(const char* label, const char* valueText, CRect rect, MenuItemProps props = {}) {
    props.valueText = valueText;
    return MenuItem(label, rect, props, nullptr);
}

// -------------------------------------------------------------
// 16. MENU (Trainer-style vertical list manager)
// -------------------------------------------------------------
inline void Menu(const MenuProps& props, const std::function<void()>& content) {
    if (!content) return;
    const Theme& t = GetTheme();
    unsigned int id = HashId(props.title ? props.title : "Menu");

    MenuState& state = g_menuStates[id];
    if (state.totalItems > 0) {
        if (Input::navDown) {
            state.selectedIndex = (state.selectedIndex + 1) % state.totalItems;
        } else if (Input::navUp) {
            state.selectedIndex = (state.selectedIndex - 1 + state.totalItems) % state.totalItems;
        }
    }

    if (state.selectedIndex < state.scrollOffset) {
        state.scrollOffset = state.selectedIndex;
    } else if (state.selectedIndex >= state.scrollOffset + props.maxVisible) {
        state.scrollOffset = state.selectedIndex - props.maxVisible + 1;
    }

    float bannerH = (props.banner.height > 0.0f && (props.banner.imageId || props.banner.color.a > 0)) ? props.banner.height : 58.0f;
    float subH = (props.subtitle && props.subtitle[0] != '\0') ? 26.0f : 0.0f;
    float rowsH = props.itemHeight * static_cast<float>(props.maxVisible);
    float footerH = props.showFooter ? 28.0f : 0.0f;
    float totalH = bannerH + subH + props.padding * 2.0f + rowsH + footerH;

    CRect menuRc = props.rect;
    if (props.position != CardPosition::Manual) {
        menuRc = ResolveCardRect(props.position, props.width, totalH, props.marginX, props.marginY);
    } else if (!(menuRc.right > menuRc.left && menuRc.bottom > menuRc.top)) {
        menuRc = CRect(props.marginX, props.marginY, props.marginX + props.width, props.marginY + totalH);
    }

    // Outer surface
    Renderer::DrawShadow(menuRc, t.radiusLg, 8.0f, CRGBA{0, 0, 0, 90});
    Renderer::DrawRoundedRect(menuRc, t.card, t.radiusLg);
    Renderer::DrawRoundedBorder(menuRc, t.border, t.radiusLg, 1.0f);

    float curY = menuRc.top;

    // Header banner
    CRect bannerRc(menuRc.left, curY, menuRc.right, curY + bannerH);
    if (props.banner.color.a > 0) {
        Renderer::DrawRoundedRect(bannerRc, props.banner.color, t.radiusLg);
    } else {
        Renderer::DrawRoundedRect(bannerRc, t.primary, t.radiusLg);
    }
    if (props.banner.imageId && props.banner.imageId[0]) {
        Image(props.banner.imageId, bannerRc, CRGBA{255, 255, 255, 255}, ImageFit::Cover);
    }
    Renderer::DrawRect(CRect(bannerRc.left, bannerRc.bottom - 1.0f, bannerRc.right, bannerRc.bottom), t.border);

    if (props.title && props.title[0] != '\0') {
        DWORD fontAlign = (props.banner.align == CardHeaderAlign::Center) ? ALIGN_CENTER : (props.banner.align == CardHeaderAlign::Right) ? ALIGN_RIGHT : ALIGN_LEFT;
        CRect titleRc(bannerRc.left + 16.0f, bannerRc.top + (bannerH - 24.0f) * 0.5f, bannerRc.right - 16.0f, bannerRc.bottom);
        FontManager::DrawGameFont(t.fontHeader, props.title, titleRc, CRGBA{255, 255, 255, 255}, 0.65f, 1.55f, fontAlign);
    }
    curY += bannerH;

    // Subtitle bar (Menyoo / NativeUI style)
    if (props.subtitle && props.subtitle[0] != '\0') {
        CRect subRc(menuRc.left, curY, menuRc.right, curY + subH);
        Renderer::DrawRect(subRc, t.secondary);
        Renderer::DrawRect(CRect(subRc.left, subRc.bottom - 1.0f, subRc.right, subRc.bottom), t.border);
        CRect subTextRc(subRc.left + 14.0f, subRc.top + 4.0f, subRc.right - 14.0f, subRc.bottom);
        FontManager::DrawGameFont(t.fontSubtitle, props.subtitle, subTextRc, t.foreground, 0.28f, 0.80f, ALIGN_LEFT);

        if (!props.showFooter && state.totalItems > 0) {
            char cntBuf[32];
            snprintf(cntBuf, sizeof(cntBuf), "%d / %d", state.selectedIndex + 1, state.totalItems);
            FontManager::DrawGameFont(t.fontSubtitle, cntBuf, subTextRc, t.mutedForeground, 0.28f, 0.80f, ALIGN_RIGHT);
        }
        curY += subH;
    }

    // Content rows slot
    curY += props.padding;
    CRect contentRc(menuRc.left + props.padding, curY, menuRc.right - props.padding, curY + rowsH);

    MenuContextState ctx;
    ctx.menuId = id;
    ctx.maxVisible = props.maxVisible;
    ctx.selectedIndex = state.selectedIndex;
    ctx.scrollOffset = state.scrollOffset;
    ctx.totalItems = state.totalItems;
    ctx.contentRect = contentRc;
    ctx.itemHeight = props.itemHeight;

    g_activeMenu = &ctx;
    content();
    state.totalItems = ctx.currentItemIndex;
    g_activeMenu = nullptr;

    // Footer bar
    if (props.showFooter) {
        CRect footerRc(menuRc.left, menuRc.top + totalH - footerH, menuRc.right, menuRc.top + totalH);
        Renderer::DrawRect(CRect(footerRc.left, footerRc.top, footerRc.right, footerRc.top + 1.0f), t.border);
        Renderer::DrawRoundedRect(footerRc, t.secondary, t.radiusLg);

        // Scroll indicator chevrons
        CRect indRc(footerRc.left + 14.0f, footerRc.top + 4.0f, footerRc.left + 50.0f, footerRc.bottom);
        const char* ind = (state.scrollOffset > 0 && state.scrollOffset + props.maxVisible < state.totalItems) ? "^ v" :
                          (state.scrollOffset > 0) ? "^" :
                          (state.scrollOffset + props.maxVisible < state.totalItems) ? "v" : "";
        FontManager::DrawGameFont(GameFontStyle::Menu, ind, indRc, t.mutedForeground, 0.30f, 0.85f, ALIGN_LEFT);

        // Counter: "X / Y"
        if (state.totalItems > 0) {
            char cntBuf[32];
            snprintf(cntBuf, sizeof(cntBuf), "%d / %d", state.selectedIndex + 1, state.totalItems);
            CRect cntRc(footerRc.left + 14.0f, footerRc.top + 4.0f, footerRc.right - 14.0f, footerRc.bottom);
            FontManager::DrawGameFont(GameFontStyle::Menu, cntBuf, cntRc, t.mutedForeground, 0.30f, 0.85f, ALIGN_RIGHT);
        }
    }
}

} // namespace sa_ui
