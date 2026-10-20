#pragma once
#include <plugin.h>
#include "CRGBA.h"
#include <string>

namespace sa_ui {

enum class GameFontStyle {
    Gothic,
    Subtitle,
    Menu,
    Pricedown
};

struct Theme {
    // Background & Surfaces
    CRGBA background{9, 9, 11, 240};        // zinc-950 (overlay / window bg)
    CRGBA foreground{250, 250, 250, 255};    // zinc-50  (default text)
    CRGBA card{24, 24, 27, 255};             // zinc-900 (cards / panels)
    CRGBA cardForeground{250, 250, 250, 255};
    CRGBA popover{24, 24, 27, 255};          // dropdown / dialog
    CRGBA popoverForeground{250, 250, 250, 255};

    // Brand / Action
    CRGBA primary{250, 250, 250, 255};       // zinc-50 (primary button bg)
    CRGBA primaryForeground{9, 9, 11, 255};  // zinc-950 (primary button text)
    CRGBA secondary{39, 39, 42, 255};        // zinc-800
    CRGBA secondaryForeground{244, 244, 245, 255}; // zinc-100

    // Utility & States
    CRGBA muted{39, 39, 42, 255};            // zinc-800
    CRGBA mutedForeground{161, 161, 170, 255}; // zinc-400
    CRGBA accent{39, 39, 42, 255};           // hover highlights
    CRGBA accentForeground{250, 250, 250, 255};
    CRGBA destructive{239, 68, 68, 255};     // red-500
    CRGBA destructiveForeground{250, 250, 250, 255};

    // Borders & Rings
    CRGBA border{39, 39, 42, 255};           // zinc-800
    CRGBA input{39, 39, 42, 255};            // input borders
    CRGBA ring{59, 130, 246, 255};           // blue-500 (focus indicator)

    // Geometry tokens
    float radiusSm{4.0f};
    float radiusMd{6.0f};
    float radiusLg{8.0f};

    // Typography slots. These IDs are resolved by FontManager at draw time.
    std::string fontDefault{"default"};
    std::string fontHeading{"default"};
    std::string fontMono{"default"};
    GameFontStyle fontHeader{GameFontStyle::Pricedown};
    GameFontStyle fontLabel{GameFontStyle::Menu};
    GameFontStyle fontSubtitle{GameFontStyle::Subtitle};
    GameFontStyle fontStyledHeader{GameFontStyle::Gothic};
    int textBodySize{15};
    int textSmallSize{12};
    int textHeadingSize{19};

    // Behavior & Effects
    bool slowTimeWhenOpen{false};
    float timeScaleOnOpen{0.0f};          // 0.0f = full pause, 0.1f = slow-mo
    bool showBackdropOverlay{true};
    CRGBA backdropOverlayCol{0, 0, 0, 26}; // 10% black dim overlay
    bool useGameDefaultCursor{true};       // uses models/fronten_pc.txd mouse sprite

    // Pre-built presets
    static Theme ZincDark() {
        return Theme{};
    }

    static Theme ZincLight() {
        Theme t;
        t.background          = CRGBA{255, 255, 255, 240};
        t.foreground          = CRGBA{9, 9, 11, 255};
        t.card                = CRGBA{255, 255, 255, 255};
        t.cardForeground      = CRGBA{9, 9, 11, 255};
        t.popover             = CRGBA{255, 255, 255, 255};
        t.popoverForeground   = CRGBA{9, 9, 11, 255};
        t.primary             = CRGBA{24, 24, 27, 255};
        t.primaryForeground   = CRGBA{250, 250, 250, 255};
        t.secondary           = CRGBA{244, 244, 245, 255};
        t.secondaryForeground = CRGBA{24, 24, 27, 255};
        t.muted               = CRGBA{244, 244, 245, 255};
        t.mutedForeground     = CRGBA{113, 113, 122, 255};
        t.accent              = CRGBA{244, 244, 245, 255};
        t.accentForeground    = CRGBA{24, 24, 27, 255};
        t.destructive         = CRGBA{239, 68, 68, 255};
        t.destructiveForeground = CRGBA{250, 250, 250, 255};
        t.border              = CRGBA{228, 228, 231, 255};
        t.input               = CRGBA{228, 228, 231, 255};
        t.ring                = CRGBA{59, 130, 246, 255};
        return t;
    }

    static Theme SlateDark() {
        Theme t;
        t.background          = CRGBA{2, 6, 23, 240};   // slate-950
        t.foreground          = CRGBA{248, 250, 252, 255};
        t.card                = CRGBA{15, 23, 42, 255}; // slate-900
        t.cardForeground      = CRGBA{248, 250, 252, 255};
        t.popover             = CRGBA{15, 23, 42, 255};
        t.popoverForeground   = CRGBA{248, 250, 252, 255};
        t.primary             = CRGBA{248, 250, 252, 255};
        t.primaryForeground   = CRGBA{15, 23, 42, 255};
        t.secondary           = CRGBA{30, 41, 59, 255};  // slate-800
        t.secondaryForeground = CRGBA{248, 250, 252, 255};
        t.muted               = CRGBA{30, 41, 59, 255};
        t.mutedForeground     = CRGBA{148, 163, 184, 255};
        t.border              = CRGBA{30, 41, 59, 255};
        t.input               = CRGBA{30, 41, 59, 255};
        t.ring                = CRGBA{56, 189, 248, 255}; // sky-400
        return t;
    }

    static Theme Cyberpunk() {
        Theme t;
        t.background          = CRGBA{10, 10, 20, 245};
        t.foreground          = CRGBA{254, 240, 138, 255}; // yellow-200
        t.card                = CRGBA{18, 18, 36, 255};
        t.cardForeground      = CRGBA{254, 240, 138, 255};
        t.popover             = CRGBA{18, 18, 36, 255};
        t.popoverForeground   = CRGBA{254, 240, 138, 255};
        t.primary             = CRGBA{254, 226, 0, 255};   // cyber yellow
        t.primaryForeground   = CRGBA{0, 0, 0, 255};
        t.secondary           = CRGBA{0, 240, 255, 255};   // cyber cyan
        t.secondaryForeground = CRGBA{0, 0, 0, 255};
        t.muted               = CRGBA{35, 35, 60, 255};
        t.mutedForeground     = CRGBA{160, 160, 190, 255};
        t.border              = CRGBA{254, 226, 0, 180};
        t.input               = CRGBA{35, 35, 60, 255};
        t.ring                = CRGBA{0, 240, 255, 255};
        t.radiusSm = 0.0f;
        t.radiusMd = 0.0f;
        t.radiusLg = 0.0f;
        return t;
    }
};

inline Theme currentTheme = Theme::ZincDark();

inline void SetTheme(const Theme& theme) {
    currentTheme = theme;
}

inline const Theme& GetTheme() {
    return currentTheme;
}

} // namespace sa_ui
