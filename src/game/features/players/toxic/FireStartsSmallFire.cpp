#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class ExplodePlayerFireStartsSmallFire : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        virtual void OnCall(Player player) override
        {
            auto playerCoords = player.GetPed().GetPosition();
            FIRE::ADD_EXPLOSION(playerCoords.x, playerCoords.y, playerCoords.z, (int)ExplosionTypes::FIRE_STARTS_SMALL_FIRE, 10.0f, true, false, 5.0f);
        }
    };

    static ExplodePlayerFireStartsSmallFire _ExplodePlayerFireStartsSmallFire{"fire_starts_small_fire", "", "FIRE_STARTS_SMALL_FIRE"};
}