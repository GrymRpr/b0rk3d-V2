#include "GUI.hpp"
#include "Menu.hpp"
#include "ESP.hpp"
#include "ContextMenu.hpp"
#include "Overlay.hpp"
#include "core/commands/Commands.hpp" // Required to access the command system
#include "core/commands/BoolCommand.hpp" // Required to use BoolCommand
#include "core/renderer/Renderer.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/frontend/ChatDisplay.hpp"

namespace YimMenu
{
	GUI::GUI() :
	    m_IsOpen(false)
	{
		Menu::SetupFonts();
		Menu::SetupStyle();
		Menu::Init();

		Renderer::AddWindowProcedureCallback([this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			GUI::WndProc(hwnd, msg, wparam, lparam);
		});

		Renderer::AddRendererCallBack(
		    [&] {
			    Notifications::Draw();
		    },
		    -2);
		Renderer::AddRendererCallBack(
		    [&] {
			    ESP::Draw();
		    },
		    -3);

		Renderer::AddRendererCallBack(
		    [&] {
			    ContextMenu::DrawContextMenu();
		    },
		    -4);
		Renderer::AddRendererCallBack(
		    [&] {
			    ChatDisplay::Draw();
		    },
		    -5);
		Renderer::AddRendererCallBack(
		    [&] {
			    Overlay::Draw();
		    },
		    -6);
	}

	GUI::~GUI()
	{
	}

	void GUI::ToggleMouse()
	{
		auto& io           = ImGui::GetIO();
		io.MouseDrawCursor = GUI::IsOpen();
		GUI::IsOpen() ? io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse : io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	void GUI::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
        // THIS IS THE FIX: The logic is now inverted to match your request.
        // Get the state of the toggle command.
        const auto use_insert_key = Commands::GetCommand<BoolCommand>("togglemenukey"_J)->GetState();
        // If ticked (true), use Insert. If unticked (false), use F5.
        const auto key_to_check = use_insert_key ? VK_INSERT : VK_F5;

		if (msg == WM_KEYUP && wparam == key_to_check)
		{
			// Persist and restore the cursor position between menu instances
			static POINT CursorCoords{};
			if (m_IsOpen)
			{
				GetCursorPos(&CursorCoords);
			}
			else if (CursorCoords.x + CursorCoords.y)
			{
				SetCursorPos(CursorCoords.x, CursorCoords.y);
			}
			Toggle();
			ToggleMouse();
		}
	}
}