#pragma once
#include "core/frontend/manager/UIManager.hpp"
#include "core/commands/ColorCommand.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace YimMenu::Submenus
{
	class Settings : public Submenu
	{
	public:
		Settings();

		int GetWindowWidth() const;
		int GetWindowHeight() const;
		int GetWindowPosX() const;
		int GetWindowPosY() const;
		bool IsWindowMaximized() const;

		void SetWindowWidth(int w);
		void SetWindowHeight(int h);
		void SetWindowPosX(int x);
		void SetWindowPosY(int y);
		void SetWindowMaximized(bool maximized);

		void LoadSettings();
		void SaveSettings();
		void SaveIfWindowChanged(int width, int height, int posX, int posY, bool maximized);

	private:
		int m_WindowWidth = 800;
		int m_WindowHeight = 600;
		int m_WindowPosX = 100;
		int m_WindowPosY = 100;
		bool m_WindowMaximized = false;

		int m_LastSavedWidth = 800;
		int m_LastSavedHeight = 600;
		int m_LastSavedPosX = 100;
		int m_LastSavedPosY = 100;
		bool m_LastSavedMaximized = false;

		void LoadWindowSettings(const nlohmann::json& j);
		void SaveWindowSettings(nlohmann::json& j) const;
		std::string GetSettingsFilePath() const;
	};

	void ApplyMenuColors();
	void ResetToDefaultColors();
	void DrawColorSettings();
}