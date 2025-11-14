#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class ExplodePlayerSmallerExplosionWithMetallicTrainSound : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        virtual void OnCall(Player player) override
        {
            auto playerCoords = player.GetPed().GetPosition();
            FIRE::ADD_EXPLOSION(playerCoords.x, playerCoords.y, playerCoords.z, (int)ExplosionTypes::SMALLER_EXPLOSION_WITH_METALLIC_TRAIN_SOUND, 10.0f, true, false, 5.0f);
        }
    };

    static ExplodePlayerSmallerExplosionWithMetallicTrainSound _ExplodePlayerSmallerExplosionWithMetallicTrainSound{"smaller_explosion_with_metallic_train_sound", "", "SMALLER_EXPLOSION_WITH_METALLIC_TRAIN_SOUND"};
}