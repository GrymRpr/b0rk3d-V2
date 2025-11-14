#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "util/Joaat.hpp"
#include <chrono>
#include <vector>
#include <mutex>

namespace YimMenu::Features
{
    class BeatDown : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        static std::vector<YimMenu::Ped> spawnedPeds;
        static std::mutex pedsMutex;
        static Hash beatdownGroupHash;
        static bool beatdownGroupInitialized;
        static constexpr size_t maxSpawnedPeds = 10;

    public:
        BeatDown() : PlayerCommand("beatdown", "Beat Down", "Spawns two invincible peds to attack the player for 45 seconds")
        {
        }

        virtual void OnCall(Player player) override
        {
            if (player.GetId() == Self::GetPlayer().GetId())
            {
                Notifications::Show("BeatDown", "Cannot target self!", NotificationType::Error);
                return;
            }

            auto targetPed = player.GetPed();
            if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
            {
                Notifications::Show("BeatDown", "Player ped not found!", NotificationType::Error);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(pedsMutex);
                if (spawnedPeds.size() >= maxSpawnedPeds)
                {
                    Notifications::Show("BeatDown", "Too many peds!", NotificationType::Error);
                    return;
                }
            }

            int targetHandle = targetPed.GetHandle();
            auto targetPos = targetPed.GetPosition();

            Hash pedModel = Joaat("CS_oddfellowspinhead");
            if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel))
            {
                Notifications::Show("BeatDown", "Invalid ped model!", NotificationType::Error);
                return;
            }
            STREAMING::REQUEST_MODEL(pedModel, false);
            int maxAttempts = 100;
            while (!STREAMING::HAS_MODEL_LOADED(pedModel) && maxAttempts--)
            {
                YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(10));
            }
            if (!STREAMING::HAS_MODEL_LOADED(pedModel))
            {
                Notifications::Show("BeatDown", "Failed to load ped model!", NotificationType::Error);
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);
                return;
            }

            auto spawnPos1 = targetPos + Vector3(2.0f, 2.0f, 0.0f);
            auto spawnPos2 = targetPos + Vector3(-2.0f, -2.0f, 0.0f);

            auto ped1 = Ped::Create(pedModel, spawnPos1, targetPed.GetRotation().z);
            auto ped2 = Ped::Create(pedModel, spawnPos2, targetPed.GetRotation().z);

            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

            if (!ped1 || !ped1.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped1.GetHandle()) ||
                !ped2 || !ped2.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped2.GetHandle()))
            {
                Notifications::Show("BeatDown", "Failed to spawn peds!", NotificationType::Error);
                if (ped1 && ped1.IsValid()) ped1.Delete();
                if (ped2 && ped2.IsValid()) ped2.Delete();
                return;
            }

            if (!beatdownGroupInitialized)
            {
                if (PED::ADD_RELATIONSHIP_GROUP("BEATDOWN_GROUP", &beatdownGroupHash))
                {
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, beatdownGroupHash, beatdownGroupHash);
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, beatdownGroupHash, PED::GET_PED_RELATIONSHIP_GROUP_HASH(targetHandle));
                    beatdownGroupInitialized = true;
                }
                else
                {
                    Notifications::Show("BeatDown", "Failed to create relationship group!", NotificationType::Error);
                    if (ped1.IsValid()) ped1.Delete();
                    if (ped2.IsValid()) ped2.Delete();
                    return;
                }
            }

            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped1.GetHandle(), true, true);
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped2.GetHandle(), true, true);

            ped1.SetInvincible(true);
            ped2.SetInvincible(true);

            for (auto ped : {ped1, ped2})
            {
                int handle = ped.GetHandle();
                if (!ENTITY::DOES_ENTITY_EXIST(handle))
                {
                    Notifications::Show("BeatDown", "Ped invalidated!", NotificationType::Warning);
                    continue;
                }

                // High health and behavior tweaks
                ENTITY::SET_ENTITY_HEALTH(handle, 1000, 0); // High health
                PED::SET_PED_MOVE_RATE_OVERRIDE(handle, 2.0f); // Maximum movement rate (affects walk/run)
                PED::SET_PED_COMBAT_MOVEMENT(handle, 3); // Suicidal Offensive (most aggressive)
                PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, beatdownGroupHash);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_DISABLE_IN_MP, true);
                PED::SET_PED_COMBAT_ABILITY(handle, 3); // Elite
                // PED::SET_PED_COMBAT_MOVEMENT(handle, 2); // Aggressive; replaced by suicidal above
                PED::SET_PED_COMBAT_RANGE(handle, 2); // Far
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, true); // Always fight
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 0, true); // Use cover
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 1, true); // Use vehicles
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true); // Fight armed when unarmed
                PED::SET_PED_FLEE_ATTRIBUTES(handle, 0, false); // Never flee
                PED::SET_PED_SEEING_RANGE(handle, 75.0f);
                PED::SET_PED_HEARING_RANGE(handle, 75.0f);
                ped.SetConfigFlag(PedConfigFlag::CanAttackFriendly, true);
                ped.SetConfigFlag(PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
                ped.SetConfigFlag(PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);

                if (ENTITY::DOES_ENTITY_EXIST(targetHandle))
                {
                    TASK::TASK_COMBAT_PED(handle, targetHandle, 0, 16);
                }
            }

            {
                std::lock_guard<std::mutex> lock(pedsMutex);
                spawnedPeds.push_back(ped1);
                spawnedPeds.push_back(ped2);
            }

            int ped1Handle = ped1.GetHandle();
            int ped2Handle = ped2.GetHandle();

            YimMenu::FiberPool::Push([ped1Handle, ped2Handle]() {
                auto start = std::chrono::steady_clock::now();
                while (std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start).count() < 45)
                {
                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(50));
                }

                {
                    std::lock_guard<std::mutex> lock(pedsMutex);
                    for (auto it = spawnedPeds.begin(); it != spawnedPeds.end();)
                    {
                        if (it->GetHandle() == ped1Handle || it->GetHandle() == ped2Handle)
                        {
                            if (it->IsValid() && ENTITY::DOES_ENTITY_EXIST(it->GetHandle()))
                            {
                                it->ForceControl();
                                auto handle = it->GetHandle();
                                ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, false, false);
                                TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, false, true);
                                it->Delete();
                            }
                            it = spawnedPeds.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    if (spawnedPeds.empty() && beatdownGroupInitialized)
                    {
                        PED::REMOVE_RELATIONSHIP_GROUP(beatdownGroupHash);
                        beatdownGroupInitialized = false;
                        beatdownGroupHash = 0;
                    }
                }

                Notifications::Show("BeatDown", "Peds cleaned up!", NotificationType::Info);
            });

            Notifications::Show("BeatDown", "Peds spawned!", NotificationType::Success);
        }
    };

    std::vector<YimMenu::Ped> BeatDown::spawnedPeds;
    std::mutex BeatDown::pedsMutex;
    Hash BeatDown::beatdownGroupHash = 0;
    bool BeatDown::beatdownGroupInitialized = false;

    static BeatDown _BeatDown{};
}