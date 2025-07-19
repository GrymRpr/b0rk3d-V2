#pragma once
#include "game/Commands/PlayerCommand.hpp"
#include "game/rdr/Player.hpp"

namespace YimMenu::Features {
    class SpankDatAss : public PlayerCommand {
    public:
        SpankDatAss();
        void OnCall(Player target) override;
    };
}