# sa_ui — Modern Native UI Library for GTA San Andreas

> [!WARNING]
> **100% AI-Coded & Work-In-Progress (WIP)**  
> The entire library was generated and iterated via LLM agents. It is battle-tested against a live game session, but the API surface is still moving. Expect breaking changes, rough edges, and the occasional crash.

A header-only, shadcn-inspired native UI component library for **Grand Theft Auto: San Andreas (1.0 US)** built on RenderWare 2D (`RwIm2D`) & Direct3D 9.

This is **not** an ImGui/ReShade overlay. Everything renders inside the game's own draw pipeline, so it composites correctly with the world, radar, and HUD.

---

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Component Reference](#component-reference)
- [Declarative Layout](#declarative-layout)
- [Theming](#theming)
- [Typography & Fonts](#typography--fonts)
- [Images](#images)
- [Toasts](#toasts)
- [Input & Gamepad](#input--gamepad)
- [Demo Project](#demo-project)
- [Repo Layout](#repo-layout)
- [Credits](#credits)

---

## Features

| Area | Capabilities |
|---|---|
| **Containers** | `Card` (freeform dashboard), `Menu` (trainer-style vertical list with auto-scroll), `Tabs`, `Collapsible`, `Dialog` (modal) |
| **Inputs** | `InputText`, `Textarea`, `Checkbox`, `RadioButton`, `Select` (dropdown), `Slider` (number/percent with fine-tune & default snap), `IndexSelect` / `Stepper` (enum stepper) |
| **Actions** | `Button` (7 variants, 3 sizes, loading & disabled states), `MenuItem` (full-width row with marquee scroll) |
| **Layout** | Immediate-mode `Flex` / `Grid` containers, explicit `CRect` placement, `Text`, `Separator`, `Badge` |
| **Theming** | Token-based `Theme` struct (Tailwind/shadcn color scale), 4 built-in presets, full custom theme support |
| **Typography** | TrueType/OpenType loader (`ID3DXFont`), system fonts, native GTA `CFont` roles (Pricedown / Menu / Subtitle / Gothic) |
| **Images** | RenderWare TXD textures with `Stretch` / `Contain` / `Cover` fit modes |
| **Feedback** | Floating `Toast` notifications, backdrop dim overlay, optional time-scale pause while open |

---

## Requirements

- **Game**: Grand Theft Auto: San Andreas 1.0 US (`gta_sa.exe`)
- **SDK**: [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa) — the extended Plugin-SDK fork with reverse-engineered game structures
- **Toolchain**: Visual Studio 2022/2026, **Win32 (x86)**, C++20 (`/std:c++latest`)
- **Runtime**: ASI Loader (e.g. [Silent's ASI Loader](https://gtaforums.com/topic/523431-silents-asi-loader/)) for the demo build

---

## Installation

`sa_ui` is a **header-only** library. You do not ship a separate `.asi` with your mod — you just add the include path and include the header.

### 1. Clone

```bash
git clone https://github.com/mbahArip/sa_ui.git
```

### 2. Add the include path to your `.vcxproj`

```xml
<AdditionalIncludeDirectories>$(YourSaUiPath)\include;$(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
```

> Replace `$(YourSaUiPath)` with your local clone path, e.g. `C:\Users\me\dev\sa_project\sa_ui`.

### 3. Link the required libraries

```xml
<AdditionalDependencies>gdi32.lib;user32.lib;$(SdkRoot)\shared\dxsdk\d3dx9.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

`$(SdkRoot)` must point at your `plugin-sdk-sa` clone. The demo project resolves it from the `SA_SDK_ROOT` environment variable, falling back to a local path:

```powershell
$env:SA_SDK_ROOT = "D:\path\to\plugin-sdk-sa"
```

You can also override it per build:

```powershell
MSBuild sa_ui.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:SdkRoot="D:\path\to\plugin-sdk-sa"
```

### 4. Include and use

```cpp
#include <plugin.h>
#include <sa_ui/sa_ui.h>
```

---

## Quick Start

```cpp
#include <plugin.h>
#include <sa_ui/sa_ui.h>

using namespace plugin;

class MyModPlugin {
    bool showMenu = false;
    bool godMode = false;
    int  selectedTab = 0;

public:
    MyModPlugin() {
        // 1. Initialize the library once RenderWare is up
        Events::initRwEvent += [] {
            sa_ui::Init(sa_ui::Theme::ZincDark());
            sa_ui::FontManager::LoadSystem("default", "Segoe UI", 15, FW_SEMIBOLD);
        };

        // 2. Toggle the menu
        Events::gameProcessEvent += [this] {
            if (KeyPressed(VK_F4)) showMenu = !showMenu;
        };

        // 3. Draw inside the HUD pass
        Events::drawHudEvent += [this] {
            sa_ui::BeginFrame(showMenu);

            if (showMenu) {
                CRect panel(100.0f, 100.0f, 480.0f, 460.0f);
                sa_ui::BeginCard(panel, "Trainer", "Manage your player options");

                selectedTab = sa_ui::Tabs({"Player", "Vehicles"}, selectedTab,
                                          CRect(116.0f, 160.0f, 464.0f, 195.0f));

                if (selectedTab == 0) {
                    sa_ui::Checkbox("God Mode", &godMode, { 116.0f, 215.0f });

                    if (sa_ui::Button("Heal CJ",
                                      CRect(116.0f, 260.0f, 250.0f, 296.0f),
                                      sa_ui::ButtonVariant::Primary)) {
                        if (CPlayerPed* p = FindPlayerPed(0)) p->m_fHealth = 1000.0f;
                    }
                }
            }

            sa_ui::EndFrame(showMenu);
        };

        // 4. Release resources on shutdown / hot-reload
        Events::shutdownRwEvent += [] {
            sa_ui::Shutdown();
        };
    }
} myModPlugin;
```

### Lifecycle Contract

| Phase | Call | Notes |
|---|---|---|
| `initRwEvent` | `sa_ui::Init(theme)` | Creates font manager + input state. Safe to call once. |
| `drawHudEvent` | `sa_ui::BeginFrame(isOpen)` → widgets → `sa_ui::EndFrame(isOpen)` | **Every widget must be called between these two.** |
| `shutdownRwEvent` | `sa_ui::Shutdown()` | Frees fonts, images, and overlay state. |

`BeginFrame` handles the time-scale pause, backdrop dim overlay, and input state update. `EndFrame` flushes stacked overlays (dropdown popovers, dialogs), draws the cursor, and renders pending toasts.

---

## Component Reference

All widgets are immediate-mode: call them directly inside the draw pass.

### Card — freeform dashboard

```cpp
CRect cardRect(100.0f, 100.0f, 480.0f, 520.0f);
sa_ui::BeginCard(cardRect, "Vehicle Spawner", "Select and customize vehicles");
```

Or the compound form with position presets and a banner:

```cpp
sa_ui::Card({
    .position = sa_ui::CardPosition::TopLeft,
    .marginX = 50.0f,
    .marginY = 50.0f,
    .width = 460.0f,
    .height = 560.0f,
    .banner = {
        .color = CRGBA{20, 90, 60, 255},
        .height = 80.0f,
        .align = sa_ui::CardHeaderAlign::Center
    },
    .title = "SAN ANDREAS"
}, [&] {
    // Tabs, Grid, Sliders, Checkboxes...
});
```

Positions: `Manual`, `Center`, `TopLeft`, `TopRight`, `BottomLeft`, `BottomRight`, `LeftCenter`, `RightCenter`.

### Button

```cpp
if (sa_ui::Button("Spawn Bullet", CRect(120.0f, 200.0f, 280.0f, 236.0f), sa_ui::ButtonVariant::Primary)) { /* ... */ }
if (sa_ui::Button("Delete Vehicle", CRect(290.0f, 200.0f, 450.0f, 236.0f), sa_ui::ButtonVariant::Destructive)) { /* ... */ }
```

Variants: `Primary`, `Secondary`, `Outline`, `Ghost`, `Destructive`, `Link`. Sizes: `Sm`, `Md`, `Lg`.

### InputText / Textarea

```cpp
static char playerName[64] = "Carl Johnson";
sa_ui::InputText("NameInput", playerName, sizeof(playerName),
                 CRect(120.0f, 160.0f, 460.0f, 196.0f), "Enter name...");

static char notes[256] = "Default mod notes...";
sa_ui::Textarea("NotesInput", notes, sizeof(notes),
                CRect(120.0f, 210.0f, 460.0f, 320.0f), "Enter notes here...");
```

### Checkbox / RadioButton

```cpp
static bool godMode = false;
if (sa_ui::Checkbox("God Mode", &godMode, CVector2D{ 120.0f, 340.0f })) { /* toggled */ }

static int drivingStyle = 0;
sa_ui::RadioButton("Normal", &drivingStyle, 0, CVector2D{ 120.0f, 370.0f });
sa_ui::RadioButton("Aggressive", &drivingStyle, 1, CVector2D{ 220.0f, 370.0f });
```

### Tabs

```cpp
static int activeTab = 0;
std::vector<std::string> tabLabels = { "Player", "Weapons", "Vehicles", "Config" };
activeTab = sa_ui::Tabs(tabLabels, activeTab, CRect(120.0f, 150.0f, 460.0f, 185.0f));
```

### Collapsible

```cpp
static bool isEngineOpen = false;
sa_ui::Collapsible("Engine Tuning", &isEngineOpen, CRect(120.0f, 200.0f, 460.0f, 234.0f));
if (isEngineOpen) { /* draw sub-items with an offset */ }
```

### Select (dropdown)

```cpp
static int selectedCar = 0;
static bool isDropdownOpen = false;
std::vector<std::string> carList = { "Infernus", "Turismo", "Bullet", "Cheetah" };

sa_ui::Select("CarSelect", carList, &selectedCar, &isDropdownOpen,
              CRect(120.0f, 250.0f, 460.0f, 286.0f), "Choose car...");
```

### Dialog (modal)

```cpp
static bool isDialogOpen = false;

sa_ui::Dialog("Confirm Replacement", &isDialogOpen, [&](CRect contentArea) {
    sa_ui::FontManager::Draw("", "Replace your current vehicle?",
        CRect(contentArea.left, contentArea.top + 10.0f, contentArea.right, contentArea.top + 40.0f),
        sa_ui::GetTheme().foreground);

    if (sa_ui::Button("Confirm", CRect(contentArea.left, contentArea.bottom - 36.0f,
                                       contentArea.left + 120.0f, contentArea.bottom),
                       sa_ui::ButtonVariant::Primary)) isDialogOpen = false;
    if (sa_ui::Button("Cancel", CRect(contentArea.left + 130.0f, contentArea.bottom - 36.0f,
                                      contentArea.left + 230.0f, contentArea.bottom),
                       sa_ui::ButtonVariant::Ghost)) isDialogOpen = false;
}, 440.0f, 220.0f);
```

### Slider

- **Percent**: value stored as `0.0`–`1.0`, default step `0.05`, displayed as `0%`–`100%`.
- **Number**: range may span negative to positive, default step `1.0`.
- **Fine-tune**: click the value pill to type an exact value, or press Up/Down.
- **Default snap**: double-click the thumb resets to `defaultValue`.

```cpp
static float opacity = 0.85f;
sa_ui::Slider("Opacity", &opacity, {
    .type = sa_ui::SliderType::Percent,
    .defaultValue = 0.85f,
    .hasDefault = true
}, CRect(120.0f, 300.0f, 460.0f, 336.0f));

static float pitchOffset = 0.0f;
sa_ui::Slider("Pitch Offset", &pitchOffset, {
    .type = sa_ui::SliderType::Number,
    .min = -45.0f, .max = 45.0f, .step = 1.0f
}, CRect(120.0f, 350.0f, 460.0f, 386.0f));
```

### IndexSelect / Stepper

Compact horizontal enum switcher — `[ < ] Option [ > ]` — preferred over dropdowns for enums.

```cpp
static int weatherIdx = 0;
sa_ui::IndexSelect("Weather", &weatherIdx, { "Sunny", "Extra Sunny", "Cloudy", "Rainy", "Foggy" },
                    CRect(120.0f, 430.0f, 460.0f, 464.0f));

static int stars = 3;
sa_ui::Stepper("Wanted Level", &stars, 0, 6,
               CRect(120.0f, 475.0f, 460.0f, 509.0f), /*step=*/1, /*wrap=*/true);
```

`wrap` defaults to `false` (clamps at the ends); set `true` to loop.

### MenuItem

Full-width trainer row: native `FONT_MENU` label on the left, value text or a child control on the right.

- Labels longer than the available width scroll smoothly after 500 ms of hover/nav focus (`wrap = false`), or wrap (`wrap = true`). Clipped with a D3D9 scissor rect.
- `disabled = true` greys the row and blocks interaction.
- `showDivider = true` draws a bottom separator line.

```cpp
// Inside a Menu or Card:
sa_ui::MenuItem("Teleport to Marker", ">", { .showDivider = true });

// With an inline right-hand control:
sa_ui::MenuItem("Infinite Ammo", {}, [&](CRect rightRc) {
    sa_ui::Checkbox("", &ammo, { rightRc.right - 20.0f, rightRc.top + 8.0f });
});
```

### Menu (trainer stack)

Auto-windowed scrolling (`maxVisible`, default 8), header banner, subtitle bar, index counter footer (`3 / 20`), scroll chevrons, and D-pad wraparound.

```cpp
sa_ui::Menu({
    .position = sa_ui::CardPosition::TopLeft,
    .marginX = 40.0f,
    .marginY = 40.0f,
    .width = 380.0f,
    .maxVisible = 8,
    .title = "TRAINER",
    .subtitle = "TELEPORT MENU",
    .banner = { .color = CRGBA{180, 20, 20, 255 } }
}, [&] {
    if (sa_ui::MenuItem("Los Santos Airport", ">")) { /* ... */ }
    if (sa_ui::MenuItem("Grove Street", ">"))        { /* ... */ }
    // 50+ items are handled transparently
});
```

### Badge / Separator

```cpp
sa_ui::Badge("PRO", CRect(400.0f, 110.0f, 450.0f, 130.0f), sa_ui::BadgeVariant::Secondary);
sa_ui::Separator(CVector2D{ 120.0f, 200.0f }, 340.0f);
```

---

## Declarative Layout

For HTML-like composition, use the stack-based helpers. Containers establish a layout scope; children acquire their next rectangle from that scope.

```cpp
using namespace sa_ui;

Flex({
    .rect = CRect(100.0f, 100.0f, 520.0f, 420.0f),
    .direction = FlexDirection::Column,
    .gap = 12.0f,
    .padding = 20.0f
}, [&] {
    CardHeader("Player Settings", "Tune the local player controls");
    Text("Movement", { .size = 12, .color = GetTheme().mutedForeground });
    Checkbox("God Mode", &godMode);
    Checkbox("Infinite Ammo", &infiniteAmmo);
    Button("Save Changes", { .variant = ButtonVariant::Primary, .width = 180.0f });
});

Grid({
    .rect = CRect(100.0f, 100.0f, 700.0f, 320.0f),
    .columns = 2,
    .gap = 12.0f,
    .padding = 16.0f
}, [&] {
    Checkbox("Show on Foot", &showOnFoot);
    Checkbox("Show in Vehicle", &showInVehicle);
    Checkbox("Lock Elevation", &lockElevation);
});
```

`FlexProps` supports `direction` (`Row` / `Column`), `gap`, `padding`, `rect`, `width`, `height`, plus `justify` and `align` tokens.

> Children with intrinsic sizing should pass an explicit `width` when used in a row. This keeps placement deterministic in an immediate-mode renderer.

---

## Theming

`Theme` is a token struct mirroring Tailwind / shadcn. Colors use RenderWare's `CRGBA{r, g, b, a}` (0–255).

```cpp
sa_ui::Theme theme = sa_ui::Theme::ZincDark();
theme.fontDefault = "default";
theme.fontHeading = "heading";
theme.fontMono = "mono";
theme.textBodySize = 15;
theme.textSmallSize = 12;
theme.textHeadingSize = 19;
sa_ui::SetTheme(theme);
```

### Built-in presets

| Preset | Call |
|---|---|
| Zinc Dark (default) | `sa_ui::Theme::ZincDark()` |
| Zinc Light | `sa_ui::Theme::ZincLight()` |
| Slate Dark | `sa_ui::Theme::SlateDark()` |
| Cyberpunk | `sa_ui::Theme::Cyberpunk()` |

### Token groups

| Group | Tokens |
|---|---|
| Surfaces | `background`, `foreground`, `card`, `cardForeground`, `popover`, `popoverForeground` |
| Action | `primary`, `primaryForeground`, `secondary`, `secondaryForeground` |
| Utility | `muted`, `mutedForeground`, `accent`, `accentForeground`, `destructive`, `destructiveForeground` |
| Borders | `border`, `input`, `ring` |
| Geometry | `radiusSm`, `radiusMd`, `radiusLg` |
| Behavior | `slowTimeWhenOpen`, `timeScaleOnOpen`, `showBackdropOverlay`, `backdropOverlayCol`, `useGameDefaultCursor` |

### Custom theme

```cpp
sa_ui::Theme emerald;
emerald.background        = CRGBA{6, 20, 15, 240};
emerald.card              = CRGBA{10, 30, 22, 255};
emerald.cardForeground    = CRGBA{236, 253, 245, 255};
emerald.primary           = CRGBA{16, 185, 129, 255};
emerald.primaryForeground = CRGBA{0, 0, 0, 255};
emerald.ring              = CRGBA{52, 211, 153, 255};
sa_ui::SetTheme(emerald);
```

---

## Typography & Fonts

`sa_ui` loads TrueType/OpenType fonts through Direct3D 9 (`ID3DXFont`) with private process registration (`AddFontResourceEx`), and falls back to the native `CFont` engine on failure.

### From file

```cpp
sa_ui::FontManager::LoadFromFile(
    "inter",                    // Font ID handle
    "fonts/Inter-Medium.ttf",   // File path
    "Inter",                    // Face name inside the TTF metadata
    16,                         // Pixel height
    FW_NORMAL                   // FW_LIGHT | FW_NORMAL | FW_MEDIUM | FW_SEMIBOLD | FW_BOLD
);
```

**Path resolution order** (no Windows font install required):
1. Plugin directory (`PLUGIN_PATH`) — e.g. `modloader/MyMod/fonts/Inter-Medium.ttf`
2. Game root (`GAME_PATH`) — e.g. `GTA San Andreas/fonts/Inter-Medium.ttf`
3. Absolute path — e.g. `C:/MyProject/fonts/Inter-Medium.ttf`

If the file is missing or corrupt, `LoadFromFile` returns `false` and every component falls back to the native game font.

### System fonts

```cpp
sa_ui::FontManager::LoadSystem("heading", "Segoe UI", 18, FW_BOLD);
```

### Native GTA font roles

```cpp
sa_ui::Theme theme = sa_ui::Theme::ZincDark();
theme.fontHeader        = sa_ui::GameFontStyle::Pricedown;
theme.fontLabel         = sa_ui::GameFontStyle::Menu;
theme.fontSubtitle      = sa_ui::GameFontStyle::Subtitle;
theme.fontStyledHeader  = sa_ui::GameFontStyle::Gothic;
sa_ui::SetTheme(theme);
```

| Role | `GameFontStyle` | Use |
|---|---|---|
| Display header | `Pricedown` | Major headings |
| Label | `Menu` | Labels and control text |
| Subtitle | `Subtitle` | Descriptions and captions |
| Styled header | `Gothic` | Emphasized headings |

### Measure & draw

```cpp
float w = 0.0f, h = 0.0f;
sa_ui::FontManager::Measure("inter", "Spawn Infernus", &w, &h);

sa_ui::FontManager::Draw(
    "inter", "Vehicle Options",
    CRect(100.0f, 100.0f, 300.0f, 120.0f),
    sa_ui::GetTheme().foreground,
    sa_ui::ALIGN_LEFT | DT_NOCLIP
);
```

Font textures survive D3D9 device loss and reset — `d3dLostEvent` / `d3dResetEvent` are handled internally.

---

## Images

Images render textures already loaded in the current RenderWare TXD:

```cpp
sa_ui::ImageManager::LoadFromCurrentTxd("logo", "my_logo");
sa_ui::Image("logo", CRect(100.0f, 100.0f, 260.0f, 160.0f),
             CRGBA{255, 255, 255, 255}, sa_ui::ImageFit::Contain);
```

`ImageFit` supports `Stretch`, `Contain`, and `Cover`. Texture ownership is managed by `ImageManager` and released on `sa_ui::Shutdown()`.

---

## Toasts

```cpp
sa_ui::Toast::Show("Saved", "Settings written to disk", sa_ui::ToastVariant::Success);
```

Toasts render in the top layer with a slide-in animation and auto-dismiss.

---

## Input & Gamepad

While the UI is open:

| Input | Action |
|---|---|
| Mouse | Move, hover, click, wheel, drag (sliders), text caret |
| D-Pad / Left Stick / Up / Down | Cycle the focused widget (focus ring highlight) |
| Enter / Cross / Circle | Activate a button, toggle a checkbox, enter widget edit mode |
| Left / Right | Adjust a slider, step an `IndexSelect`, move a `RadioButton` |
| L1 / R1 (LB / RB) | Switch the active tab |
| Textarea | `Shift+Enter` inserts a newline, `Enter` commits |

Game controls are frozen while the menu is open (shooting, moving, camera, and the pause menu are suppressed). When the menu closes, the cursor is re-centered to prevent mouse-look snapping.

---

## Demo Project

`src/Main.cpp` is a standalone demo `.asi` that exercises every component. It opens with **F4** (dashboard) and **F5** (trainer menu).

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" sa_ui.vcxproj /p:Configuration=Release /p:Platform=Win32
```

Copy `bin\Release\sa_ui.asi` into your GTA San Andreas root directory and launch the game.

---

## Repo Layout

```text
sa_ui/
├── include/sa_ui/       ; Header-only library (this is all you need to consume it)
│   ├── sa_ui.h          ; Init / BeginFrame / EndFrame / Shutdown
│   ├── Theme.h          ; Theme tokens & presets
│   ├── Font.h           ; TTF/OTF + system font manager
│   ├── Renderer.h       ; RwIm2D drawing primitives
│   ├── Input.h          ; Mouse / keyboard / gamepad state
│   ├── Image.h          ; TXD texture manager
│   ├── Components.h     ; All widgets
│   ├── Layout.h         ; Flex / Grid declarative layout
│   └── Toast.h          ; Notification toasts
├── src/Main.cpp         ; Demo plugin
└── sa_ui.vcxproj        ; Demo project file
```

---

## Credits

- **100% code generation**: LLM agents.
- **SDK**: [Dryxio/plugin-sdk-sa](https://github.com/Dryxio/plugin-sdk-sa) by Dryxio and the Plugin-SDK community.
- **Design language**: shadcn/ui and Tailwind CSS.
