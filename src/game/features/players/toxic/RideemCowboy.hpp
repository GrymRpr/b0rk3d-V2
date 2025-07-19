#pragma once
#include "core/frontend/manager/Category.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/commands/PlayerCommand.hpp" // Added for PlayerCommand
#include "game/rdr/Player.hpp" // Added for Player

namespace YimMenu::Features
{
    class RideemCowboy : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

    public:
        RideemCowboy();
        virtual void OnCall(Player player) override;
    };
}