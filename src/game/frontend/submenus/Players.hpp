#pragma once
#include "core/frontend/manager/UIManager.hpp"

namespace YimMenu::Submenus
{
    class Players : public Submenu
    {
    public:
        Players();
    };
    
    // Forward declarations for submenu builders
    std::shared_ptr<Category> BuildInfoMenu();
    std::shared_ptr<Category> BuildHelpfulMenu();
    std::shared_ptr<Category> BuildTrollingMenu();
    std::shared_ptr<Category> BuildToxicMenu();
    std::shared_ptr<Category> BuildKickMenu();
    std::shared_ptr<Category> BuildBoomPlusMenu();
    // No need to declare Zombies here since it's included via BoomPlus
}