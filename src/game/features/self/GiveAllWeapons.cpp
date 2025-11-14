#include "core/commands/Command.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/data/WeaponTypes.hpp"
#include "game/rdr/Enums.hpp"

namespace YimMenu::Features
{
    class GiveAllWeapons : public Command
    {
    public:
        using Command::Command;

        virtual void OnCall() override
        {
            FiberPool::Push([=] {
                auto ped = Self::GetPed();
                
                if (!ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
                {
                    LOG(WARNING) << "Invalid ped handle in GiveAllWeapons";
                    return;
                }

                for (const auto& weapon : Data::g_WeaponTypes)
                {
                    Hash weaponHash = Joaat(weapon);
                    
                    if (weaponHash == 0)
                    {
                        LOG(WARNING) << "Invalid weapon hash for: " << weapon;
                        continue;
                    }

                    WEAPON::GIVE_WEAPON_TO_PED(
                        ped.GetHandle(), 
                        weaponHash, 
                        9999,     // Ammo count
                        true,     // Equip immediately
                        false,    // Not as drop
                        0,        // Group
                        true,     // Equip to left slot if two-handed
                        0.5f,     // Blend duration
                        1.0f,     // Unk
                        0x2CD419DC, // Component flags
                        true,     // Skip equipping if already owned
                        0.0f,     // Unk
                        false     // Not a gift
                    );
                    ScriptMgr::Yield(20ms);
                }

                LOG(INFO) << "All weapons added successfully";
            });
        }
    };

    static GiveAllWeapons _GiveAllWeapons{"giveallweapons", "Give All Weapons", "Gives you all of the weapons"};

    void TriggerGiveAllWeapons()
    {
        _GiveAllWeapons.OnCall();
    }
}