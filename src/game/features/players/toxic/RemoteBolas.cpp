#include "core/frontend/Notifications.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/FloatCommand.hpp"
#include "core/commands/IntCommand.hpp"
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

    class RemoteBolasRandomDirection : public BoolCommand
    {
        using BoolCommand::BoolCommand;
        virtual void OnCall() override {}
    };

    class RemoteBolasStrikerDistance : public FloatCommand
    {
        using FloatCommand::FloatCommand;
        virtual void OnCall() override {}
    };

    class RemoteBolasDamage : public IntCommand
    {
        using IntCommand::IntCommand;
        virtual void OnCall() override {}
    };

    class RemoteBolasType : public ListCommand
    {
        using ListCommand::ListCommand;
        virtual void OnCall() override {}
    };

    static RemoteBolasShowStriker _RemoteBolasShowStriker{
        "remotebolas_show_striker", "Show Striker Ped", "Show the striker ped when firing bolas", false};

    static RemoteBolasRandomDirection _RemoteBolasRandomDirection{
        "remotebolas_random_direction", "Random Strike Direction", "Strike from random directions", true};

    static RemoteBolasStrikerDistance _RemoteBolasStrikerDistance{
        "remotebolas_striker_distance", "Striker Distance", "Distance of striker from target", 3.0f, 20.0f, 8.0f};

    static RemoteBolasDamage _RemoteBolasDamage{
        "remotebolas_damage", "Bolas Damage", "0 for no damage, 100 for full damage.", 0, 100, 100};

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

        Vector3 GetRandomOffsetPosition(const Vector3& center, float radius)
        {
            float angle = Math::DegToRad(MISC::GET_RANDOM_FLOAT_IN_RANGE(0.0f, 360.0f));
            float x     = center.x + (cos(angle) * radius);
            float y     = center.y + (sin(angle) * radius);
            return Vector3(x, y, center.z);
        }

        bool IsPlayerProtected(Player player)
        {
            auto ped = player.GetPed();

            bool canBeLassoed = PED::_GET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), (int)LassoFlags::LHF_CAN_BE_LASSOED);
            bool disabledInMP = PED::_GET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), (int)LassoFlags::LHF_DISABLE_IN_MP);
            bool isInVehicle  = PED::IS_PED_IN_ANY_VEHICLE(ped.GetHandle(), false);
            bool isDead       = PED::IS_PED_DEAD_OR_DYING(ped.GetHandle(), false);
            bool isRagdoll    = PED::IS_PED_RAGDOLL(ped.GetHandle());

            return !canBeLassoed || disabledInMP || isInVehicle || isDead || isRagdoll;
        }

        void CreateVisualEffects(const Vector3& strikerPos, const Vector3& targetPos)
        {
            GRAPHICS::USE_PARTICLE_FX_ASSET("scr_moonshine");
            GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("scr_moonshine_throw",
                strikerPos.x,
                strikerPos.y,
                strikerPos.z,
                0.0f,
                0.0f,
                0.0f,
                0.5f,
                false,
                false,
                false);
        }

    public:
        virtual void OnCall(Player player) override
        {
            if (!player.IsValid())
            {
                Notifications::Show("Remote Bolas", "Invalid player target!", NotificationType::Error);
                return;
            }

            if (IsPlayerProtected(player))
            {
                Notifications::Show("Remote Bolas", "Target is protected or unavailable!", NotificationType::Warning);
                return;
            }

            bool showStriker      = _RemoteBolasShowStriker.GetState();
            bool randomDirection  = _RemoteBolasRandomDirection.GetState();
            BolasType bolasType   = static_cast<BolasType>(_RemoteBolasType.GetState());

            float strikerDistance = _RemoteBolasStrikerDistance.GetState();
            int damage = _RemoteBolasDamage.GetState();
            
            auto targetPed = player.GetPed();
            auto targetPos = targetPed.GetPosition();

            Vector3 strikerPos;
            if (randomDirection)
            {
                strikerPos = GetRandomOffsetPosition(targetPos, strikerDistance);
            }
            else
            {
                strikerPos = targetPos - Vector3(strikerDistance, 0, 0);
            }

            float ground_z;
            if (MISC::GET_GROUND_Z_FOR_3D_COORD(strikerPos.x, strikerPos.y, targetPos.z, &ground_z, false))
            {
                strikerPos.z = ground_z;
            }
            else
            {
                strikerPos.z = targetPos.z;
            }

            auto striker = Ped::Create("cs_brendacrawley"_J, strikerPos, 0.0f);

            if (!striker.IsValid())
            {
                Notifications::Show("Remote Bolas", "Failed to create striker!", NotificationType::Error);
                return;
            }
            
            ENTITY::SET_ENTITY_VISIBLE(striker.GetHandle(), showStriker);
            ENTITY::SET_ENTITY_INVINCIBLE(striker.GetHandle(), true);
            PED::SET_PED_CAN_RAGDOLL(striker.GetHandle(), false);

            auto direction = targetPos - strikerPos;
            float heading  = atan2(direction.y, direction.x) * (180.0f / 3.14159265359f);
            PED::SET_PED_DESIRED_HEADING(striker.GetHandle(), heading);

            CreateVisualEffects(strikerPos, targetPos);

            Hash weaponHash = GetBolasWeaponHash(bolasType);

            MISC::SHOOT_SINGLE_BULLET_BETWEEN_COORDS(strikerPos.x,
                strikerPos.y,
                strikerPos.z + 1.0f,
                targetPos.x,
                targetPos.y,
                targetPos.z,
                damage,
                true,
                weaponHash,
                striker.GetHandle(),
                true,
                true,
                -1.0f,
                true);

            Notifications::Show("Remote Bolas", std::format("Bolas fired at {}", player.GetName()), NotificationType::Success);

            if (!showStriker)
            {
                striker.Delete();
            }
            else
            {
                ENTITY::SET_ENTITY_INVINCIBLE(striker.GetHandle(), false);
                ENTITY::SET_ENTITY_HEALTH(striker.GetHandle(), 0, 0);
            }
        }
    };

    static RemoteBolas _RemoteBolas{"remotebolas", "Remote Bolas", "Remotely strike players with configurable bolas"};
}