#pragma once
#include "core/frontend/manager/UIManager.hpp"
#include "game/backend/AnimationDict.hpp"
#include "game/backend/MusicDict.hpp"

namespace YimMenu::Submenus
{
	class Self : public Submenu
	{
	public:
		Self();
	};

	void RenderAnimationsCategory();

	void LoadMusicHistory();
	void SaveMusicHistory();
}