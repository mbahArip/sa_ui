#include <plugin.h>
#include <game_sa/CStreaming.h>
#include <game_sa/CWorld.h>
#include <game_sa/CAutomobile.h>
#include <game_sa/CPlayerPed.h>
#include <game_sa/CPad.h>
#include "sa_ui/sa_ui.h"

using namespace plugin;

class SaUiDemoPlugin {
    bool m_showMenu = false;
    bool m_showTrainer = false;
    bool m_godMode = false;
    bool m_infiniteAmmo = true;
    int  m_selectedTab = 0;
    int  m_selectedVehicle = 0;
    bool m_vehicleSelectOpen = false;
    bool m_dialogOpen = false;
    bool m_collapsibleOpen = true;
    float m_playerSpeed = 1.0f;
    float m_radarOpacity = 0.85f;
    float m_cameraPitch = 0.0f;
    int  m_weatherPreset = 0;
    char m_playerName[64] = "Carl Johnson";
    char m_notes[256] = "Welcome to San Andreas UI!";

public:
    SaUiDemoPlugin() {
        // Initialize sa_ui on startup
        Events::initRwEvent += [] {
            sa_ui::Init(sa_ui::Theme::ZincDark());

            // Load custom TrueType / OpenType font if available, or system font
            sa_ui::FontManager::LoadSystem("default", "Segoe UI", 16, FW_SEMIBOLD);
        };

        // Key toggle: F4 for Dashboard Card, F5 for Trainer Menu
        Events::gameProcessEvent += [this] {
            if (KeyPressed(VK_F4)) {
                m_showMenu = !m_showMenu;
            }
            if (KeyPressed(VK_F5)) {
                m_showTrainer = !m_showTrainer;
            }
        };

        // Render UI in GTA SA draw pass
        Events::drawHudEvent += [this] {
            sa_ui::BeginFrame(m_showMenu || m_showTrainer);

            if (m_showMenu) {
                // Main Container Card
                CRect mainPanel(80.0f, 60.0f, 520.0f, 560.0f);
                sa_ui::BeginCard(mainPanel, "San Andreas UI", "Modern shadcn-style native component library");

                // Badge in header
                sa_ui::Badge("v1.0.0", CRect(440.0f, 74.0f, 500.0f, 94.0f), sa_ui::BadgeVariant::Secondary);

                // Tabs
                std::vector<std::string> tabs = { "Player", "Vehicles", "Items", "Settings" };
                m_selectedTab = sa_ui::Tabs(tabs, m_selectedTab, CRect(100.0f, 120.0f, 500.0f, 155.0f));

                float curY = 175.0f;

                if (m_selectedTab == 0) {
                    // TAB 1: PLAYER
                    sa_ui::InputText("Player Name", m_playerName, sizeof(m_playerName), CRect(100.0f, curY, 500.0f, curY + 36.0f), "Enter player name...");
                    curY += 46.0f;

                    sa_ui::Checkbox("God Mode", &m_godMode, {100.0f, curY});
                    curY += 28.0f;

                    sa_ui::Checkbox("Infinite Ammo", &m_infiniteAmmo, {100.0f, curY});
                    curY += 34.0f;

                    sa_ui::FontManager::Draw("", "Move Speed", CRect(100.0f, curY, 500.0f, curY + 16.0f), sa_ui::GetTheme().foreground, sa_ui::ALIGN_LEFT);
                    curY += 20.0f;
                    sa_ui::Slider("Speed", &m_playerSpeed, 0.5f, 3.0f, CRect(100.0f, curY, 500.0f, curY + 24.0f));
                    curY += 36.0f;

                    if (sa_ui::Button("Heal Player", CRect(100.0f, curY, 290.0f, curY + 36.0f), sa_ui::ButtonVariant::Primary)) {
                        CPlayerPed* player = FindPlayerPed(0);
                        if (player) player->m_fHealth = 1000.0f;
                    }

                    if (sa_ui::Button("Open Modal", CRect(310.0f, curY, 500.0f, curY + 36.0f), sa_ui::ButtonVariant::Outline)) {
                        m_dialogOpen = true;
                    }
                }
                else if (m_selectedTab == 1) {
                    // TAB 2: VEHICLES
                    std::vector<std::string> vehicleList = { "Infernus", "Turismo", "Bullet", "NRG-500", "Hydra" };
                    sa_ui::Select("Vehicle", vehicleList, &m_selectedVehicle, &m_vehicleSelectOpen, CRect(100.0f, curY, 500.0f, curY + 36.0f));
                    curY += 50.0f;

                    if (sa_ui::Button("Spawn Selected Vehicle", CRect(100.0f, curY, 500.0f, curY + 38.0f), sa_ui::ButtonVariant::Primary)) {
                        // Spawn logic example
                    }
                    curY += 48.0f;

                    sa_ui::Collapsible("Advanced Options", &m_collapsibleOpen, CRect(100.0f, curY, 500.0f, curY + 34.0f));
                    if (m_collapsibleOpen) {
                        curY += 42.0f;
                        if (sa_ui::Button("Repair Current Vehicle", CRect(100.0f, curY, 500.0f, curY + 34.0f), sa_ui::ButtonVariant::Secondary)) {
                            CVehicle* veh = FindPlayerVehicle(0, false);
                            if (veh) veh->m_fHealth = 1000.0f;
                        }
                    }
                }
                else if (m_selectedTab == 2) {
                    // TAB 3: MENU ITEMS (Flex justify-between rows)
                    sa_ui::MenuItem("Quick Travel Destination", "LS Airport", CRect(100.0f, curY, 500.0f, curY + 38.0f), { .showDivider = true });
                    curY += 42.0f;

                    // Marquee scroll test on long label
                    sa_ui::MenuItem("Super Ultra Extreme High Density Realistic Population Traffic Mode", "ACTIVE", CRect(100.0f, curY, 500.0f, curY + 38.0f), { .showDivider = true });
                    curY += 42.0f;

                    // Item with inline control callback
                    sa_ui::MenuItem("Infinite Ammo Mode", CRect(100.0f, curY, 500.0f, curY + 38.0f), { .showDivider = true }, [&](CRect rightRc) {
                        sa_ui::Checkbox("", &m_infiniteAmmo, { rightRc.right - 20.0f, rightRc.top + (rightRc.bottom - rightRc.top - 18.0f) * 0.5f });
                    });
                    curY += 42.0f;

                    // Disabled item
                    sa_ui::MenuItem("Restricted Admin Debug Tools", "LOCKED", CRect(100.0f, curY, 500.0f, curY + 38.0f), { .disabled = true, .showDivider = true });
                    curY += 42.0f;
                }
                else if (m_selectedTab == 3) {
                    // TAB 4: SETTINGS
                    sa_ui::IndexSelect("Weather", &m_weatherPreset, { "Sunny", "Extra Sunny", "Cloudy", "Rainy", "Foggy" }, CRect(100.0f, curY, 500.0f, curY + 34.0f));
                    curY += 44.0f;

                    sa_ui::Slider("Radar Opacity", &m_radarOpacity, { .type = sa_ui::SliderType::Percent }, CRect(100.0f, curY, 500.0f, curY + 36.0f));
                    curY += 46.0f;

                    sa_ui::Slider("Camera Pitch", &m_cameraPitch, { .type = sa_ui::SliderType::Number, .min = -45.0f, .max = 45.0f, .step = 1.0f }, CRect(100.0f, curY, 500.0f, curY + 36.0f));
                    curY += 46.0f;

                    if (sa_ui::Button("Switch to Light Theme", CRect(100.0f, curY, 290.0f, curY + 36.0f), sa_ui::ButtonVariant::Outline)) {
                        sa_ui::SetTheme(sa_ui::Theme::ZincLight());
                        sa_ui::Toast::Show("Theme Changed", "Zinc Light preset applied", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::Button("Switch to Dark Theme", CRect(310.0f, curY, 500.0f, curY + 36.0f), sa_ui::ButtonVariant::Outline)) {
                        sa_ui::SetTheme(sa_ui::Theme::ZincDark());
                        sa_ui::Toast::Show("Theme Changed", "Zinc Dark preset applied", sa_ui::ToastVariant::Success);
                    }
                }

                // Modal Dialog Component
                sa_ui::Dialog("Confirm Action", &m_dialogOpen, [this](CRect area) {
                    sa_ui::FontManager::Draw("", "Are you sure you want to perform this operation?", CRect(area.left, area.top + 10.0f, area.right, area.top + 40.0f), sa_ui::GetTheme().foreground, sa_ui::ALIGN_LEFT);

                    if (sa_ui::Button("Confirm", CRect(area.left, area.bottom - 40.0f, area.left + 140.0f, area.bottom), sa_ui::ButtonVariant::Primary)) {
                        m_dialogOpen = false;
                    }
                    if (sa_ui::Button("Cancel", CRect(area.left + 150.0f, area.bottom - 40.0f, area.left + 270.0f, area.bottom), sa_ui::ButtonVariant::Ghost)) {
                        m_dialogOpen = false;
                    }
                });
            }

            if (m_showTrainer) {
                // Trainer Menu Stack with Windowed Item Scrolling & Footer Counter
                sa_ui::Menu({
                    .position = sa_ui::CardPosition::TopRight,
                    .marginX = 40.0f,
                    .marginY = 40.0f,
                    .width = 380.0f,
                    .maxVisible = 8,
                    .title = "TRAINER",
                    .subtitle = "TELEPORT DESTINATIONS",
                    .banner = { .color = CRGBA{20, 110, 70, 255} }
                }, [&] {
                    if (sa_ui::MenuItem("Los Santos International Airport", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to LS Airport", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Grove Street Cul-de-Sac", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Grove Street", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Mount Chiliad Peak Viewpoint", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Mount Chiliad", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Las Venturas The Strip Casino", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to LV Strip", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("San Fierro Pier 69 Esplanade", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to SF Pier 69", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Bayside Marina Lighthouse", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Bayside", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Area 69 Restricted Military Research Base", "LOCKED", { .disabled = true })) {
                    }
                    if (sa_ui::MenuItem("Verdant Meadows Aircraft Graveyard", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Airstrip", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Chilliad Mountain State Wilderness Park", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Wilderness", sa_ui::ToastVariant::Default);
                    }
                    if (sa_ui::MenuItem("Santa Maria Beach Boardwalk Pier", ">")) {
                        sa_ui::Toast::Show("Teleport", "Traveled to Santa Maria", sa_ui::ToastVariant::Default);
                    }
                });
            }

            sa_ui::EndFrame(m_showMenu || m_showTrainer);
        };
    }
} saUiDemoPlugin;
