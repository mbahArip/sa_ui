#pragma once

#include "Components.h"
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace sa_ui {

enum class FlexDirection { Row, Column };
enum class FlexJustify { Start, Center, End, SpaceBetween, SpaceAround };
enum class FlexAlign { Start, Center, End, Stretch };

struct FlexProps {
    CRect rect{0.0f, 0.0f, 0.0f, 0.0f};
    float width{0.0f};
    float height{0.0f};
    float gap{0.0f};
    float padding{0.0f};
    FlexDirection direction{FlexDirection::Column};
    FlexJustify justify{FlexJustify::Start};
    FlexAlign align{FlexAlign::Stretch};
};

struct GridProps {
    CRect rect{0.0f, 0.0f, 0.0f, 0.0f};
    float width{0.0f};
    float height{0.0f};
    int columns{1};
    float gap{0.0f};
    float padding{0.0f};
    FlexAlign align{FlexAlign::Stretch};
};

struct TextProps {
    std::string font{};
    bool useGameFont{true};
    GameFontStyle gameFont{GameFontStyle::Menu};
    int size{0};
    CRGBA color{250, 250, 250, 255};
    DWORD weight{FW_NORMAL};
    DWORD align{ALIGN_LEFT};
    float width{0.0f};
    float height{0.0f};
};

struct ButtonProps {
    ButtonVariant variant{ButtonVariant::Default};
    ButtonSize size{ButtonSize::Md};
    bool disabled{false};
    bool loading{false};
    float width{0.0f};
    float height{0.0f};
};

struct CardProps {
    CRect rect{0.0f, 0.0f, 0.0f, 0.0f};
    CardPosition position{CardPosition::Manual};
    float marginX{40.0f};
    float marginY{40.0f};
    float width{0.0f};
    float height{0.0f};
    float padding{20.0f};
    const char* title{nullptr};
    const char* description{nullptr};
    CardBannerProps banner{};
};

class LayoutContext {
private:
    struct Node {
        CRect rect;
        FlexProps flex{};
        GridProps grid{};
        bool isGrid{false};
        int index{0};
    };

    static inline std::vector<Node> s_nodes;

    static CRect ResolveRect(const CRect& rect, float width, float height) {
        if (rect.right > rect.left && rect.bottom > rect.top) return rect;
        const float screenWidth = static_cast<float>(RsGlobal.maximumWidth);
        const float screenHeight = static_cast<float>(RsGlobal.maximumHeight);
        const float resolvedWidth = width > 0.0f ? width : screenWidth;
        const float resolvedHeight = height > 0.0f ? height : screenHeight;
        return CRect(0.0f, 0.0f, resolvedWidth, resolvedHeight);
    }

public:
    static CRect BeginFlex(const FlexProps& props) {
        CRect rect = ResolveRect(props.rect, props.width, props.height);
        s_nodes.push_back({rect, props, {}, false, 0});
        return rect;
    }

    static CRect BeginGrid(const GridProps& props) {
        CRect rect = ResolveRect(props.rect, props.width, props.height);
        s_nodes.push_back({rect, {}, props, true, 0});
        return rect;
    }

    static void End() {
        if (!s_nodes.empty()) s_nodes.pop_back();
    }

    static CRect Next(float width = 0.0f, float height = 0.0f) {
        if (s_nodes.empty()) return CRect(0.0f, 0.0f, width, height);
        Node& node = s_nodes.back();
        const float left = node.rect.left + (node.isGrid ? node.grid.padding : node.flex.padding);
        const float top = node.rect.top + (node.isGrid ? node.grid.padding : node.flex.padding);
        const float right = node.rect.right - (node.isGrid ? node.grid.padding : node.flex.padding);
        const float bottom = node.rect.bottom - (node.isGrid ? node.grid.padding : node.flex.padding);
        const float availableWidth = (std::max)(0.0f, right - left);
        const float availableHeight = (std::max)(0.0f, bottom - top);

        if (node.isGrid) {
            const int columns = (std::max)(1, node.grid.columns);
            const int column = node.index % columns;
            const int row = node.index / columns;
            const float cellWidth = (availableWidth - node.grid.gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
            const float cellHeight = height > 0.0f ? height : 36.0f;
            node.index++;
            const float x = left + column * (cellWidth + node.grid.gap);
            const float y = top + row * (cellHeight + node.grid.gap);
            return CRect(x, y, x + (width > 0.0f ? width : cellWidth), y + cellHeight);
        }

        const bool row = node.flex.direction == FlexDirection::Row;
        const float gap = node.flex.gap;
        const float itemWidth = width > 0.0f ? width : (row ? availableWidth : availableWidth);
        const float itemHeight = height > 0.0f ? height : (row ? availableHeight : 36.0f);
        float x = left;
        float y = top;
        if (row) {
            x += node.index * (itemWidth + gap);
        } else {
            y += node.index * (itemHeight + gap);
        }
        node.index++;
        return CRect(x, y, x + itemWidth, y + itemHeight);
    }
};

inline void Flex(const FlexProps& props, const std::function<void()>& content) {
    LayoutContext::BeginFlex(props);
    if (content) content();
    LayoutContext::End();
}

inline void Grid(const GridProps& props, const std::function<void()>& content) {
    LayoutContext::BeginGrid(props);
    if (content) content();
    LayoutContext::End();
}

inline void Text(const char* content, TextProps props = {}) {
    if (!content) return;
    const Theme& theme = GetTheme();
    if (props.size <= 0) props.size = theme.textBodySize;
    if (props.color.a == 0) props.color = theme.foreground;
    if (props.font.empty()) props.font = theme.fontDefault;
    CRect rect = LayoutContext::Next(props.width, props.height > 0.0f ? props.height : static_cast<float>(props.size + 6));
    if (props.useGameFont) {
        FontManager::DrawGameFont(props.gameFont, content, rect, props.color, props.size / 42.0f, props.size / 16.0f, props.align);
    } else {
        FontManager::Draw(props.font, content, rect, props.color, props.align | DT_NOCLIP);
    }
}

inline bool Button(const char* label, ButtonProps props = {}) {
    const Theme& theme = GetTheme();
    float textWidth = 0.0f;
    FontManager::Measure(theme.fontDefault, label, &textWidth, nullptr);
    const float width = props.width > 0.0f ? props.width : textWidth + 32.0f;
    const float height = props.height > 0.0f ? props.height : (props.size == ButtonSize::Lg ? 42.0f : props.size == ButtonSize::Sm ? 30.0f : 36.0f);
    return sa_ui::Button(label, LayoutContext::Next(width, height), props.variant, props.size, props.disabled, props.loading);
}

inline bool Checkbox(const char* label, bool* value) {
    float textWidth = 0.0f;
    FontManager::Measure(GetTheme().fontDefault, label, &textWidth, nullptr);
    CRect rect = LayoutContext::Next(textWidth + 26.0f, 18.0f);
    return sa_ui::Checkbox(label, value, {rect.left, rect.top});
}

inline void Card(const CardProps& props, const std::function<void()>& content) {
    CRect rect = props.rect;
    if (props.position != CardPosition::Manual) {
        rect = ResolveCardRect(props.position, props.width, props.height, props.marginX, props.marginY);
    } else if (!(rect.right > rect.left && rect.bottom > rect.top)) {
        rect = LayoutContext::Next(props.width, props.height);
    }
    const Theme& theme = GetTheme();
    Renderer::DrawShadow(rect, theme.radiusLg, 6.0f, CRGBA{0, 0, 0, 80});
    Renderer::DrawRoundedRect(rect, theme.card, theme.radiusLg);
    Renderer::DrawRoundedBorder(rect, theme.border, theme.radiusLg, 1.0f);

    float bannerOffset = 0.0f;
    if (props.banner.height > 0.0f && (props.banner.imageId || props.banner.color.a > 0)) {
        CRect bannerRc(rect.left, rect.top, rect.right, rect.top + props.banner.height);
        if (props.banner.color.a > 0) {
            Renderer::DrawRoundedRect(bannerRc, props.banner.color, theme.radiusLg);
        }
        if (props.banner.imageId && props.banner.imageId[0]) {
            Image(props.banner.imageId, bannerRc, CRGBA{255, 255, 255, 255}, ImageFit::Cover);
        }
        Renderer::DrawRect(CRect(bannerRc.left, bannerRc.bottom - 1.0f, bannerRc.right, bannerRc.bottom), theme.border);
        if (props.title && props.title[0]) {
            DWORD fontAlign = (props.banner.align == CardHeaderAlign::Center) ? ALIGN_CENTER : (props.banner.align == CardHeaderAlign::Right) ? ALIGN_RIGHT : ALIGN_LEFT;
            CRect titleRc(bannerRc.left + 16.0f, bannerRc.top, bannerRc.right - 16.0f, bannerRc.bottom);
            FontManager::DrawGameFont(theme.fontHeader, props.title, titleRc, CRGBA{255, 255, 255, 255}, 0.55f, 1.4f, fontAlign);
        }
        bannerOffset = props.banner.height;
    }

    FlexProps inner;
    inner.rect = CRect(rect.left + props.padding, rect.top + props.padding + bannerOffset, rect.right - props.padding, rect.bottom - props.padding);
    inner.direction = FlexDirection::Column;
    inner.gap = 10.0f;
    Flex(inner, content);
}

inline void CardHeader(const char* title, const char* description = nullptr) {
    const Theme& theme = GetTheme();
    TextProps heading;
    heading.font = theme.fontHeading;
    heading.useGameFont = true;
    heading.gameFont = theme.fontStyledHeader;
    heading.size = theme.textHeadingSize;
    heading.weight = FW_BOLD;
    heading.color = theme.cardForeground;
    Text(title, heading);
    if (description && description[0]) {
        TextProps body;
        body.font = theme.fontDefault;
        body.useGameFont = true;
        body.gameFont = theme.fontSubtitle;
        body.size = theme.textSmallSize;
        body.color = theme.mutedForeground;
        Text(description, body);
    }
}

inline void Separator() {
    CRect rect = LayoutContext::Next(0.0f, 1.0f);
    Renderer::DrawRect(rect, GetTheme().border);
}

inline bool Slider(const char* label, float* value, SliderProps props = {}) {
    const float height = props.height > 0.0f ? props.height : 36.0f;
    CRect rect = LayoutContext::Next(props.width, height);
    return sa_ui::Slider(label, value, props, rect);
}

inline bool IndexSelect(const char* label, int* selectedIndex, const std::vector<std::string>& items, IndexSelectProps props = {}) {
    const float height = props.height > 0.0f ? props.height : 34.0f;
    CRect rect = LayoutContext::Next(props.width, height);
    return sa_ui::IndexSelect(label, selectedIndex, items, rect, props);
}

inline bool IndexSelect(const char* label, int* selectedIndex, std::initializer_list<const char*> items, IndexSelectProps props = {}) {
    const float height = props.height > 0.0f ? props.height : 34.0f;
    CRect rect = LayoutContext::Next(props.width, height);
    return sa_ui::IndexSelect(label, selectedIndex, items, rect, props);
}

inline bool Stepper(const char* label, int* selectedIndex, const std::vector<std::string>& items, IndexSelectProps props = {}) {
    return IndexSelect(label, selectedIndex, items, props);
}

inline bool Stepper(const char* label, int* selectedIndex, std::initializer_list<const char*> items, IndexSelectProps props = {}) {
    return IndexSelect(label, selectedIndex, items, props);
}

inline bool Stepper(const char* label, int* value, int minVal, int maxVal, int step = 1, bool wrap = false, float width = 0.0f, float height = 34.0f) {
    CRect rect = LayoutContext::Next(width, height);
    return sa_ui::Stepper(label, value, minVal, maxVal, rect, step, wrap);
}

inline bool MenuItem(const char* label, MenuItemProps props = {}, const std::function<void(CRect rightRect)>& control = nullptr) {
    CRect rect{};
    if (!g_activeMenu) {
        const float height = props.height > 0.0f ? props.height : 36.0f;
        rect = LayoutContext::Next(props.width, height);
    }
    return sa_ui::MenuItem(label, rect, props, control);
}

inline bool MenuItem(const char* label, const char* valueText, MenuItemProps props = {}) {
    props.valueText = valueText;
    return MenuItem(label, props, nullptr);
}

} // namespace sa_ui
