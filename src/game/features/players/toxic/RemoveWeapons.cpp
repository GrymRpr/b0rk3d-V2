#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class RemoveWeapons : public PlayerCommand
    {
    public:
        RemoveWeapons() : PlayerCommand("removeweapons", "Remove Weapons", "Removes all weapons from the targeted player") {}

        virtual void OnCall(Player player) override
        {
            Ped ped = player.GetPed();
            if (ENTITY::DOES_ENTITY_EXIST(ped.GetHandle())) // Use GetHandle() instead of implicit conversion
            {
                WEAPON::REMOVE_ALL_PED_WEAPONS(ped.GetHandle(), true, true); // Use GetHandle() here too
            }
        }
    };

    static RemoveWeapons _RemoveWeapons;
}