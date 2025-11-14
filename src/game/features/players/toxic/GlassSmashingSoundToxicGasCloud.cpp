#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class ExplodePlayerGlassSmashingSoundToxicGasCloud : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        virtual void OnCall(Player player) override
        {
            auto playerCoords = player.GetPed().GetPosition();
            FIRE::ADD_EXPLOSION(playerCoords.x, playerCoords.y, playerCoords.z, (int)ExplosionTypes::GLASS_SMASHING_SOUND_TOXIC_GAS_CLOUD, 10.0f, true, false, 5.0f);
        }
    };

    static ExplodePlayerGlassSmashingSoundToxicGasCloud _ExplodePlayerGlassSmashingSoundToxicGasCloud{"glass_smashing_sound_toxic_gas_cloud", "", "GLASS_SMASHING_SOUND_TOXIC_GAS_CLOUD"};
}