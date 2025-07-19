#include "Settings.hpp"

#include "core/commands/Commands.hpp"
#include "core/commands/HotkeySystem.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "core/commands/ColorCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/features/Features.hpp"
#include "game/frontend/items/Items.hpp"
#include "MenuPreSets.hpp"
#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace YimMenu::Submenus
{

    int Settings::GetWindowWidth() const { return m_WindowWidth; }
    int Settings::GetWindowHeight() const { return m_WindowHeight; }
    int Settings::GetWindowPosX() const { return m_WindowPosX; }
    int Settings::GetWindowPosY() const { return m_WindowPosY; }
    bool Settings::IsWindowMaximized() const { return m_WindowMaximized; }

    void Settings::SetWindowWidth(int w) { m_WindowWidth = w; }
    void Settings::SetWindowHeight(int h) { m_WindowHeight = h; }
    void Settings::SetWindowPosX(int x) { m_WindowPosX = x; }
    void Settings::SetWindowPosY(int y) { m_WindowPosY = y; }
    void Settings::SetWindowMaximized(bool maximized) { m_WindowMaximized = maximized; }

    std::string Settings::GetSettingsFilePath() const {
        char* appdata = std::getenv("appdata");
        if (!appdata) return "";
        std::filesystem::path path = std::filesystem::path(appdata) / "Terminus-b0rk3d" / "settings.json";
        return path.string();
    }

    void Settings::LoadWindowSettings(const nlohmann::json& j) {
        int defW = 800, defH = 600, defX = 100, defY = 100;
        bool defMax = false;
        if (j.contains("window") && j["window"].is_object()) {
            const auto& win = j["window"];
            m_WindowWidth = win.value("width", defW);
            m_WindowHeight = win.value("height", defH);
            m_WindowPosX = win.value("pos_x", defX);
            m_WindowPosY = win.value("pos_y", defY);
            m_WindowMaximized = win.value("is_maximized", defMax);
        } else {
            m_WindowWidth = defW; m_WindowHeight = defH; m_WindowPosX = defX; m_WindowPosY = defY; m_WindowMaximized = defMax;
        }

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (m_WindowWidth < 300 || m_WindowWidth > screenW) m_WindowWidth = defW;
        if (m_WindowHeight < 200 || m_WindowHeight > screenH) m_WindowHeight = defH;
        if (m_WindowPosX < 0 || m_WindowPosX > screenW - 50) m_WindowPosX = defX;
        if (m_WindowPosY < 0 || m_WindowPosY > screenH - 50) m_WindowPosY = defY;

        m_LastSavedWidth = m_WindowWidth;
        m_LastSavedHeight = m_WindowHeight;
        m_LastSavedPosX = m_WindowPosX;
        m_LastSavedPosY = m_WindowPosY;
        m_LastSavedMaximized = m_WindowMaximized;
    }

    void Settings::SaveWindowSettings(nlohmann::json& j) const {
        nlohmann::json win;
        win["width"] = m_WindowWidth;
        win["height"] = m_WindowHeight;
        win["pos_x"] = m_WindowPosX;
        win["pos_y"] = m_WindowPosY;
        win["is_maximized"] = m_WindowMaximized;
        j["window"] = win;
    }

    void Settings::LoadSettings() {
        std::string path = GetSettingsFilePath();
        std::ifstream in(path);
        if (in) {
            try {
                nlohmann::json j;
                in >> j;
                LoadWindowSettings(j);
            } catch (...) {
                m_WindowWidth = 800; m_WindowHeight = 600; m_WindowPosX = 100; m_WindowPosY = 100; m_WindowMaximized = false;
            }
        } else {
            m_WindowWidth = 800; m_WindowHeight = 600; m_WindowPosX = 100; m_WindowPosY = 100; m_WindowMaximized = false;
        }
        m_LastSavedWidth = m_WindowWidth;
        m_LastSavedHeight = m_WindowHeight;
        m_LastSavedPosX = m_WindowPosX;
        m_LastSavedPosY = m_WindowPosY;
        m_LastSavedMaximized = m_WindowMaximized;
    }

    void Settings::SaveSettings() {
        std::string path = GetSettingsFilePath();
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        nlohmann::json j;
        std::ifstream in(path);
        if (in) {
            try { in >> j; } catch (...) { j = nlohmann::json::object(); }
        }
        SaveWindowSettings(j);
        std::ofstream out(path);
        if (out) out << j.dump(4);
    }

    void Settings::SaveIfWindowChanged(int width, int height, int posX, int posY, bool maximized) {
        if (width != m_LastSavedWidth || height != m_LastSavedHeight ||
            posX != m_LastSavedPosX || posY != m_LastSavedPosY ||
            maximized != m_LastSavedMaximized) {

            m_WindowWidth = width;
            m_WindowHeight = height;
            m_WindowPosX = posX;
            m_WindowPosY = posY;
            m_WindowMaximized = maximized;

            SaveSettings();

            m_LastSavedWidth = width;
            m_LastSavedHeight = height;
            m_LastSavedPosX = posX;
            m_LastSavedPosY = posY;
            m_LastSavedMaximized = maximized;
        }
    }


    class MenuColorCommand : public ColorCommand
    {
    public:
        using ColorCommand::ColorCommand;
    protected:
        void OnChange() override {}
    };

    static BoolCommand g_UseInsertForMenuToggle("togglemenukey", "Menu Toggle Key", "Un-Ticked = F5, Ticked = Insert", false);

    // region Color Commands
    static MenuColorCommand _MenuWindowBg("menu_window_bg", "Window Background", "", {0.07f, 0.10f, 0.12f, 0.0f});
    static MenuColorCommand _MenuChildBg("menu_child_bg", "Child Background", "", {0.10f, 0.14f, 0.17f, 0.70f});
    static MenuColorCommand _MenuPopupBg("menu_popup_bg", "Popup Background", "", {0.12f, 0.16f, 0.19f, 0.95f});
    static MenuColorCommand _MenuTextColor("menu_text_color", "Text", "", {1.0f, 0.89f, 0.71f, 1.00f});
    static MenuColorCommand _MenuTextDisabled("menu_text_disabled", "Text Disabled", "", {0.85f, 0.85f, 0.85f, 0.5f});
    static MenuColorCommand _MenuTextSelectedBg("menu_text_selected_bg", "Text Selected BG", "", {1.0f, 1.0f, 0.0f, 0.5f});
    static MenuColorCommand _MenuFrameBg("menu_frame_bg", "Frame Background", "", {0.27f, 0.51f, 0.71f, 0.3f});
    static MenuColorCommand _MenuFrameBgHovered("menu_frame_bg_hovered", "Frame Background Hovered", "", {0.00f, 0.48f, 1.00f, 0.60f});
    static MenuColorCommand _MenuFrameBgActive("menu_frame_bg_active", "Frame Background Active", "", {0.86f, 0.08f, 0.24f, 0.6f});
    static MenuColorCommand _MenuBorderColor("menu_border_color", "Border", "", {0.28f, 0.28f, 0.28f, 0.25f});
    static MenuColorCommand _MenuButton("menu_button", "Button", "", {0.27f, 0.51f, 0.71f, 0.6f});
    static MenuColorCommand _MenuButtonHovered("menu_button_hovered", "Button Hovered", "", {0.72f, 0.53f, 0.04f, 0.8f});
    static MenuColorCommand _MenuButtonActive("menu_button_active", "Button Active", "", {0.86f, 0.08f, 0.24f, 0.6f});
    static MenuColorCommand _MenuTitleBg("menu_title_bg", "Title Background", "", {0.00f, 0.00f, 0.00f, 0.60f});
    static MenuColorCommand _MenuTitleBgActive("menu_title_bg_active", "Title Background Active", "", {0.10f, 0.10f, 0.10f, 0.60f});
    static MenuColorCommand _MenuTitleBgCollapsed("menu_title_bg_collapsed", "Title Background Collapsed", "", {0.00f, 0.00f, 0.00f, 0.60f});
    static MenuColorCommand _MenuTab("menu_tab", "Tab", "", {0.13f, 0.13f, 0.13f, 0.60f});
    static MenuColorCommand _MenuTabHovered("menu_tab_hovered", "Tab Hovered", "", {0.00f, 0.48f, 1.00f, 0.60f});
    static MenuColorCommand _MenuTabActive("menu_tab_active", "Tab Active", "", {0.13f, 0.13f, 0.13f, 0.80f});
    static MenuColorCommand _MenuHeader("menu_header", "Header", "", {0.13f, 0.13f, 0.13f, 0.60f});
    static MenuColorCommand _MenuHeaderHovered("menu_header_hovered", "Header Hovered", "", {0.00f, 0.48f, 1.00f, 0.80f});
    static MenuColorCommand _MenuHeaderActive("menu_header_active", "Header Active", "", {0.00f, 0.48f, 1.00f, 0.90f});
    static MenuColorCommand _MenuPlotLines("menu_plot_lines", "Plot Lines", "", {0.80f, 0.80f, 0.00f, 0.00f});
    static MenuColorCommand _MenuPlotLinesHovered("menu_plot_lines_hovered", "Plot Lines Hovered", "", {1.00f, 0.85f, 0.00f, 1.00f});
    static MenuColorCommand _MenuPlotHistogram("menu_plot_histogram", "Plot Histogram", "", {0.90f, 0.70f, 0.20f, 1.00f});
    static MenuColorCommand _MenuPlotHistogramHovered("menu_plot_histogram_hovered", "Plot Histogram Hovered", "", {1.00f, 0.80f, 0.40f, 1.00f});
    static MenuColorCommand _MenuTabUnfocused("menu_tab_unfocused", "Tab Unfocused", "", {0.13f, 0.13f, 0.13f, 0.60f});
    static MenuColorCommand _MenuTabUnfocusedActive("menu_tab_unfocused_active", "Tab Unfocused Active", "", {0.13f, 0.13f, 0.13f, 0.80f});
    static MenuColorCommand _MenuMenuBarBg("menu_menubar_bg", "MenuBar Background", "", {0.10f, 0.10f, 0.10f, 0.60f});
    static MenuColorCommand _MenuScrollbarBg("menu_scrollbar_bg", "Scrollbar Background", "", {0.0f, 0.0f, 0.0f, 0.0f});
    static MenuColorCommand _MenuScrollbarGrab("menu_scrollbar_grab", "Scrollbar Grab", "", {0.27f, 0.51f, 0.71f, 0.6f});
    static MenuColorCommand _MenuScrollbarGrabHovered("menu_scrollbar_grab_hovered", "Scrollbar Grab Hovered", "", {0.72f, 0.53f, 0.04f, 0.8f});
    static MenuColorCommand _MenuScrollbarGrabActive("menu_scrollbar_grab_active", "Scrollbar Grab Active", "", {0.86f, 0.08f, 0.24f, 0.6f});
    static MenuColorCommand _MenuCheckMark("menu_checkmark", "CheckMark", "", {1.0f, 0.89f, 0.71f, 1.00f});
    static MenuColorCommand _MenuSliderGrab("menu_slider_grab", "Slider Grab", "", {0.27f, 0.51f, 0.71f, 0.6f});
    static MenuColorCommand _MenuSliderGrabActive("menu_slider_grab_active", "Slider Grab Active", "", {0.86f, 0.08f, 0.24f, 0.6f});
    static MenuColorCommand _MenuSeparator("menu_separator", "Separator", "", {0.28f, 0.28f, 0.28f, 0.25f});
    static MenuColorCommand _MenuResizeGrip("menu_resize_grip", "Resize Grip", "", {0.27f, 0.51f, 0.71f, 0.6f});
    static MenuColorCommand _MenuResizeGripHovered("menu_resize_grip_hovered", "Resize Grip Hovered", "", {0.72f, 0.53f, 0.04f, 0.8f});
    static MenuColorCommand _MenuResizeGripActive("menu_resize_grip_active", "Resize Grip Active", "", {0.86f, 0.08f, 0.24f, 0.6f});
    // endregion

    void ApplyAggressivePadding()
    {
        ImGuiStyle baseStyle; 
        auto& style = ImGui::GetStyle();

        style.FramePadding     = ImVec2(baseStyle.FramePadding.x * 0.5f, baseStyle.FramePadding.y * 0.4f);
        style.CellPadding      = ImVec2(baseStyle.CellPadding.x * 0.6f, baseStyle.CellPadding.y * 0.2f);
        style.ItemSpacing      = ImVec2(baseStyle.ItemSpacing.x * 0.5f, baseStyle.ItemSpacing.y * 0.3f);
        style.ItemInnerSpacing = ImVec2(baseStyle.ItemInnerSpacing.x * 0.5f, baseStyle.ItemInnerSpacing.y * 0.5f);
        style.WindowPadding    = ImVec2(baseStyle.WindowPadding.x * 0.5f, baseStyle.WindowPadding.y * 0.4f);
    }

    void ApplyMenuColors()
    {
        auto& style = ImGui::GetStyle();
        for (const auto& [col, name] : Themes::ImGuiCol_to_CommandName)
        {
            if (auto cmd = Commands::GetCommand<ColorCommand>(name))
            {
                style.Colors[col] = cmd->GetState();
            }
        }
    }
    
    void ApplyTheme(const Themes::ThemePreset& theme)
    {
        for (const auto& [col, vec] : theme.m_Colors)
        {
            if (auto it = Themes::ImGuiCol_to_CommandName.find(col); it != Themes::ImGuiCol_to_CommandName.end())
            {
                if (auto cmd = Commands::GetCommand<ColorCommand>(it->second))
                {
                    cmd->SetState(vec);
                }
            }
        }
        ApplyMenuColors();
    }

    void ResetToDefaultColors()
    {
        for (const auto& theme : Themes::GetThemePresets())
        {
            if (theme.m_Name == "Clear Blue")
            {
                ApplyTheme(theme);
                ApplyAggressivePadding();
                return;
            }
        }
    }

    void DrawColorSettings()
    {
        ImGui::TextUnformatted("Preset Themes");
        ImGui::Separator();
        const auto& presets = Themes::GetThemePresets();
        int buttons_per_row = 6; 

        for (int i = 0; i < presets.size(); ++i)
        {
            const auto& preset = presets[i];
            ImGui::PushID(preset.m_Name.c_str());
            ImGui::PushStyleColor(ImGuiCol_Button, preset.m_PreviewColor);

            if (ImGui::Button("  ", {40, 20}))
            {
                ApplyTheme(preset);
                ApplyAggressivePadding(); 
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", preset.m_Name.c_str());
            }

            if ((i + 1) % buttons_per_row != 0 && (i + 1) < presets.size())
            {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }

        ImGui::Spacing();
        if (ImGui::Button("Reset to default theme"))
        {
            ResetToDefaultColors();
        }
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Custom Color Editor");
        ImGui::Separator();

        auto DrawColorGroup = [](const char* title, std::initializer_list<joaat_t> hashes) {
            ImGui::TextUnformatted(title);
            ImGui::Separator();
            ImGui::BeginGroup();
            constexpr int columns = 4;
            int count = 0;
            for (const auto& hash : hashes)
            {
                if (auto cmd = Commands::GetCommand<ColorCommand>(hash))
                {
                    ImGui::PushID(cmd->GetHash());
                    
                    ImVec4 color = cmd->GetState();
                    if (ImGui::ColorEdit4("##picker", &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
                    {
                        cmd->SetState(color);
                        ApplyMenuColors();
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    {
                        ImGui::SetTooltip("%s", cmd->GetLabel().c_str());
                    }

                    ImGui::PopID();
                    
                    if ((count + 1) % columns != 0 && (count + 1) < hashes.size())
                    {
                         ImGui::SameLine();
                    }
                }
                count++;
            }
            ImGui::EndGroup();
            ImGui::Spacing();
        };

        ImGui::BeginGroup();
        DrawColorGroup("Backgrounds", {"menu_window_bg"_J, "menu_child_bg"_J, "menu_popup_bg"_J, "menu_menubar_bg"_J});
        DrawColorGroup("Frames & Borders", {"menu_frame_bg"_J, "menu_frame_bg_hovered"_J, "menu_frame_bg_active"_J, "menu_border_color"_J});
        DrawColorGroup("Title Bar", {"menu_title_bg"_J, "menu_title_bg_active"_J, "menu_title_bg_collapsed"_J});
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 15.0f);

        ImGui::BeginGroup();
        DrawColorGroup("Text", {"menu_text_color"_J, "menu_text_disabled"_J, "menu_text_selected_bg"_J, "menu_checkmark"_J});
        DrawColorGroup("Tabs", {"menu_tab"_J, "menu_tab_hovered"_J, "menu_tab_active"_J, "menu_tab_unfocused"_J});
        DrawColorGroup("Sliders & Grips", {"menu_slider_grab"_J, "menu_slider_grab_active"_J, "menu_resize_grip"_J, "menu_resize_grip_hovered"_J});
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 15.0f);
        
        ImGui::BeginGroup();
        DrawColorGroup("Plots", {"menu_plot_lines"_J, "menu_plot_lines_hovered"_J, "menu_plot_histogram"_J, "menu_plot_histogram_hovered"_J});
        DrawColorGroup("Scrollbar", {"menu_scrollbar_bg"_J, "menu_scrollbar_grab"_J, "menu_scrollbar_grab_hovered"_J, "menu_scrollbar_grab_active"_J});
        DrawColorGroup("Buttons", {"menu_button"_J, "menu_button_hovered"_J, "menu_button_active"_J});
        ImGui::EndGroup();
    }

    static void DrawHotkeySettings()
    {
        ImGui::BulletText("Hold the command name clicked to change its hotkey");
        ImGui::BulletText("Press any registered key to remove");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (auto& [Hash, Command] : Commands::GetCommands())
        {
            ImGui::PushID(Hash);

            if (g_HotkeySystem.m_CommandHotkeys.find(Hash) != g_HotkeySystem.m_CommandHotkeys.end())
                HotkeySetter(Hash).Draw();

            ImGui::Spacing();
            ImGui::PopID();
        }
    }

    Settings::Settings() :
        Submenu::Submenu("Settings")
    {
        auto hotkeys           = std::make_shared<Category>("Hotkeys");
        auto gui               = std::make_shared<Category>("GUI");
        auto protections       = std::make_shared<Category>("Protection");
        auto syncGroup         = std::make_shared<Group>("Sync");
        auto networkEventGroup = std::make_shared<Group>("Network Events");
        auto scriptEventGroup  = std::make_shared<Group>("Script Events");
        auto playerEsp         = std::make_shared<Group>("Player ESP", 10);
        auto pedEsp            = std::make_shared<Group>("Ped ESP", 10);
        auto overlay           = std::make_shared<Group>("Overlay");
        auto context           = std::make_shared<Group>("Context Menu");
        auto misc              = std::make_shared<Group>("Misc");
        auto menuTheme         = std::make_shared<Group>("Menu Theme");

        hotkeys->AddItem(std::make_shared<ImGuiItem>(DrawHotkeySettings));

        playerEsp->AddItem(std::make_shared<BoolCommandItem>("espdrawplayers"_J));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espdrawdeadplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espnameplayers"_J, "Player Name")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("namecolorplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espdistanceplayers"_J, "Player Distance")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("distancecolorplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espskeletonplayers"_J, "Player Skeleton")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("skeletoncolorplayers"_J)));

        pedEsp->AddItem(std::make_shared<BoolCommandItem>("espdrawpeds"_J));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espdrawdeadpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espmodelspeds"_J, "Ped Hashes")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("hashcolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espnetinfopeds"_J, "Ped Net Info")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espscriptinfopeds"_J, "Ped Script Info")));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espdistancepeds"_J, "Ped Distance")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("distancecolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espskeletonpeds"_J, "Ped Skeleton")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("skeletoncolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espskeletonhorse"_J, "Horse Skeleton")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("skeletoncolorhorse"_J)));

        overlay->AddItem(std::make_shared<BoolCommandItem>("overlay"_J));
        overlay->AddItem(std::make_shared<ConditionalItem>("overlay"_J, std::make_shared<BoolCommandItem>("overlayfps"_J)));

        context->AddItem(std::make_shared<BoolCommandItem>("ctxmenu"_J));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuplayers"_J, "Players")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenupeds"_J, "Peds")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuvehicles"_J, "Vehicles")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuobjects"_J, "Objects")));

        menuTheme->AddItem(std::make_shared<ImGuiItem>(DrawColorSettings));

        misc->AddItem(std::make_shared<BoolCommandItem>("togglemenukey"_J));

        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockspectate"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockspectatesession"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockattach"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockvehflood"_J));

        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockexplosions"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockptfx"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockclearpedtasks"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockscriptcommand"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("userelaycxns"_J));

        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockhonormanipulation"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockdefensive"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockoffensive"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockpresscharges"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockstartparlay"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockendparlay"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blocktickerspam"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockstableevents"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockkickfrommissionlobby"_J));

        gui->AddItem(playerEsp);
        gui->AddItem(pedEsp);
        gui->AddItem(overlay);
        gui->AddItem(context);
        gui->AddItem(misc);
        gui->AddItem(menuTheme);

        protections->AddItem(syncGroup);
        protections->AddItem(networkEventGroup);
        protections->AddItem(scriptEventGroup);

        AddCategory(std::move(hotkeys));
        AddCategory(std::move(gui));
        AddCategory(std::move(protections));
    }
}