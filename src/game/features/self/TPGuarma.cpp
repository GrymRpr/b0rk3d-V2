#include "core/commands/Command.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "util/teleport.hpp"
namespace YimMenu::Features
{
    class TpToGuarma : public Command // Using Command, not PlayerCommand, to match TpToWaypoint
    {
    public:
        TpToGuarma() : Command("tptoguarma", "Teleport to Guarma", "Teleports you to Guarma. WARNING: You will have to reset the map") {}

        virtual void OnCall() override
        {
            rage::fvector3 coords = {1423.2081298828125f, -7320.37353515625f, 81.36766815185547f};
            float heading = 170.2138671875f;
            int pedHandle = Self::GetPed().GetHandle();
            YimMenu::Teleport::TeleportEntity(pedHandle, coords, true);
            ENTITY::SET_ENTITY_HEADING(pedHandle, heading);
        }
    };

    static TpToGuarma _TpToGuarma;
}