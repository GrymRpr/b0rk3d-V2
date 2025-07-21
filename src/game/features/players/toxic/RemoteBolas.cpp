#include "core/frontend/Notifications.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/ListCommand.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/Entity.hpp"
#include "util/Math.hpp"

namespace YimMenu::Features
{
    enum class BolasType
    {
        REGULAR     = 0,
        IRON_SPIKED = 1,
        HAWKMOTH    = 2,
        INTERTWINED = 3
    };

    class RemoteBolasShowStriker : public BoolCommand
    {
        using BoolCommand::BoolCommand;
        virtual void OnCall() override {}
    };

    class RemoteBolasType : public ListCommand
    {
        using ListCommand::ListCommand;
        virtual void OnCall() override {}
    };

    static RemoteBolasShowStriker _RemoteBolasShowStriker{
        "remotebolas_show_striker", "Show Striker Ped", "Show the striker ped when firing bolas", false};

    static RemoteBolasType _RemoteBolasType{
        "remotebolas_type", "Bolas Type", "Type of bolas to use", 
        std::vector<std::pair<int, const char*>>{
            { (int)BolasType::REGULAR, "Regular" },
            { (int)BolasType::IRON_SPIKED, "Iron Spiked" },
            { (int)BolasType::HAWKMOTH, "Hawkmoth" },
            { (int)BolasType::INTERTWINED, "Intertwined" }
        }, 1};

    class RemoteBolas : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

    private:
        Hash GetBolasWeaponHash(BolasType type)
        {
            switch (type)
            {
                case BolasType::REGULAR: return "weapon_thrown_bolas"_J;
                case BolasType::IRON_SPIKED: return "weapon_thrown_bolas_ironspiked"_J;
                case BolasType::HAWKMOTH: return "weapon_thrown_bolas_hawkmoth"_J;
                case BolasType::INTERTWINED: return "weapon_thrown_bolas_intertwined"_J;
                default: return "weapon_thrown_bolas_ironspiked"_J;
            }
        }

    public:
        virtual void OnCall(Player player) override
        {
            if (!player.IsValid())
            {
                Notifications::Show("Remote Bolas", "Invalid player target!", NotificationType::Error);
                return;
            }

            // OG protection logic: Only allow if can be lassoed and not protected in MP
            bool canUseBolas = PED::_GET_PED_LASSO_HOGTIE_FLAG(player.GetPed().GetHandle(), (int)LassoFlags::LHF_CAN_BE_LASSOED)
                && !PED::_GET_PED_LASSO_HOGTIE_FLAG(player.GetPed().GetHandle(), (int)LassoFlags::LHF_DISABLE_IN_MP);

            if (!canUseBolas)
            {
                Notifications::Show("Remote Bolas", "This player has bolas protections enabled!", NotificationType::Error);
                return;
            }

            bool showStriker      = _RemoteBolasShowStriker.GetState();
            BolasType bolasType   = static_cast<BolasType>(_RemoteBolasType.GetState());

            auto targetPed = player.GetPed();
            auto targetPos = targetPed.GetPosition();
            auto targetRot = targetPed.GetRotation();

            // Fix 1: Always spawn striker at targetPos - Vector3(5, 0, 0) with target's rotation.z
            Vector3 strikerPos = targetPos - Vector3(5, 0, 0);
            auto striker = Ped::Create("cs_brendacrawley"_J, strikerPos, targetRot.z);

            if (!striker.IsValid())
            {
                Notifications::Show("Remote Bolas", "Failed to create striker!", NotificationType::Error);
                return;
            }

            ENTITY::SET_ENTITY_VISIBLE(striker.GetHandle(), showStriker);
            ENTITY::SET_ENTITY_INVINCIBLE(striker.GetHandle(), true);
            PED::SET_PED_CAN_RAGDOLL(striker.GetHandle(), false);

            const auto strikerPosActual = striker.GetPosition();
            const auto targetPosActual = targetPed.GetPosition();

            Hash weaponHash = GetBolasWeaponHash(bolasType);

            // Fix 2: Use the same projectile params as OG
            MISC::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(
                strikerPosActual.x,
                strikerPosActual.y,
                strikerPosActual.z,
                targetPosActual.x,
                targetPosActual.y,
                targetPosActual.z,
                25,                       // damage
                true,                     // unknown, OG: true
                weaponHash,               // weapon hash (can be selected)
                striker.GetHandle(),      // owner
                true,                     // high accuracy
                false,                    // not visible
                500.f,                    // speed
                true);                    // unknown, OG: true

            // Fix 3: Immediately delete the striker after firing
            striker.ForceControl();
            striker.Delete();

            Notifications::Show("Remote Bolas", std::format("Bolas fired at {}", player.GetName()), NotificationType::Success);
        }
    };

    static RemoteBolas _RemoteBolas{"remotebolas", "Remote Bolas", "Remotely strike players with configurable bolas"};
}