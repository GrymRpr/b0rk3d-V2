#include "core/commands/Command.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/data/AmmoTypes.hpp"
#include "game/rdr/Enums.hpp"

namespace YimMenu::Features
{
    class GiveAllAmmo : public Command
    {
    public:
        using Command::Command;

        virtual void OnCall() override
        {
            FiberPool::Push([=] {
                auto ped = Self::GetPed();
                
                if (!ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
                {
                    LOG(WARNING) << "Invalid ped handle in GiveAllAmmo";
                    return;
                }

                for (const auto& ammo : Data::g_AmmoTypes)
                {
                    Hash ammoHash = Joaat(ammo);
                    
                    if (ammoHash == 0)
                    {
                        LOG(WARNING) << "Invalid ammo hash for: " << ammo;
                        continue;
                    }

                    WEAPON::_ADD_AMMO_TO_PED_BY_TYPE(ped.GetHandle(), ammoHash, 9999, 0x2CD419DC);
                    ScriptMgr::Yield(10ms);
                }

                LOG(INFO) << "All ammo types added successfully";
            });
        }
    };

    static GiveAllAmmo _GiveAllAmmo{"giveallammo", "Give All Ammo", "Gives you all the kinds of ammo"};

    void TriggerGiveAllAmmo()
    {
        _GiveAllAmmo.OnCall();
    }
}