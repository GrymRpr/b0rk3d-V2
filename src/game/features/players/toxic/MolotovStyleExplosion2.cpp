#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class ExplodePlayerMolotovStyleExplosion2 : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        virtual void OnCall(Player player) override
        {
            auto playerCoords = player.GetPed().GetPosition();
            FIRE::ADD_EXPLOSION(playerCoords.x, playerCoords.y, playerCoords.z, (int)ExplosionTypes::MOLOTOV_STYLE_EXPLOSION2, 10.0f, true, false, 5.0f);
        }
    };

    static ExplodePlayerMolotovStyleExplosion2 _ExplodePlayerMolotovStyleExplosion2{"molotov_style_explosion2", "", "MOLOTOV_STYLE_EXPLOSION2"};
}