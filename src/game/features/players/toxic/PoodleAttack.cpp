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
    class PoodleAttack : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        static std::vector<YimMenu::Ped> spawnedPeds;
        static std::mutex pedsMutex;
        static Hash poodleGroupHash;
        static bool poodleGroupInitialized;
        static constexpr size_t maxSpawnedPeds = 3;

    public:
        PoodleAttack() : PlayerCommand("poodleattack", "Poodle Attack", "Spawns three invincible poodles to attack the player for 2 minutes")
        {
        }

        virtual void OnCall(Player player) override
        {
            if (player.GetId() == Self::GetPlayer().GetId())
            {
                Notifications::Show("PoodleAttack", "Cannot target self!", NotificationType::Error);
                return;
            }

            auto targetPed = player.GetPed();
            if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
            {
                Notifications::Show("PoodleAttack", "Player ped not found!", NotificationType::Error);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(pedsMutex);
                if (spawnedPeds.size() + maxSpawnedPeds > maxSpawnedPeds)
                {
                    Notifications::Show("PoodleAttack", "Too many poodles!", NotificationType::Error);
                    return;
                }
            }

            int targetHandle = targetPed.GetHandle();
            auto targetPos = targetPed.GetPosition();

            Hash pedModel = Joaat("a_c_dogpoodle_01");
            if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel))
            {
                Notifications::Show("PoodleAttack", "Invalid poodle model!", NotificationType::Error);
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
                Notifications::Show("PoodleAttack", "Failed to load poodle model!", NotificationType::Error);
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);
                return;
            }

            // Spawn positions: triangle around target
            auto spawnPos1 = targetPos + Vector3(2.0f, 2.0f, 0.0f);
            auto spawnPos2 = targetPos + Vector3(-2.0f, -2.0f, 0.0f);
            auto spawnPos3 = targetPos + Vector3(2.0f, -2.0f, 0.0f);

            auto ped1 = Ped::Create(pedModel, spawnPos1, targetPed.GetRotation().z);
            auto ped2 = Ped::Create(pedModel, spawnPos2, targetPed.GetRotation().z);
            auto ped3 = Ped::Create(pedModel, spawnPos3, targetPed.GetRotation().z);

            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

            if (!ped1 || !ped1.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped1.GetHandle()) ||
                !ped2 || !ped2.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped2.GetHandle()) ||
                !ped3 || !ped3.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped3.GetHandle()))
            {
                Notifications::Show("PoodleAttack", "Failed to spawn poodles!", NotificationType::Error);
                if (ped1 && ped1.IsValid()) ped1.Delete();
                if (ped2 && ped2.IsValid()) ped2.Delete();
                if (ped3 && ped3.IsValid()) ped3.Delete();
                return;
            }

            if (!poodleGroupInitialized)
            {
                if (PED::ADD_RELATIONSHIP_GROUP("POODLE_ATTACK_GROUP", &poodleGroupHash))
                {
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, poodleGroupHash, poodleGroupHash);
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, poodleGroupHash, PED::GET_PED_RELATIONSHIP_GROUP_HASH(targetHandle));
                    poodleGroupInitialized = true;
                }
                else
                {
                    Notifications::Show("PoodleAttack", "Failed to create relationship group!", NotificationType::Error);
                    if (ped1.IsValid()) ped1.Delete();
                    if (ped2.IsValid()) ped2.Delete();
                    if (ped3.IsValid()) ped3.Delete();
                    return;
                }
            }

            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped1.GetHandle(), true, true);
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped2.GetHandle(), true, true);
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped3.GetHandle(), true, true);

            ped1.SetInvincible(true);
            ped2.SetInvincible(true);
            ped3.SetInvincible(true);

            for (auto ped : {ped1, ped2, ped3})
            {
                int handle = ped.GetHandle();
                if (!ENTITY::DOES_ENTITY_EXIST(handle))
                {
                    Notifications::Show("PoodleAttack", "Poodle invalidated!", NotificationType::Warning);
                    continue;
                }

                ENTITY::SET_ENTITY_HEALTH(handle, 1000, 0); // High health
                PED::SET_PED_MOVE_RATE_OVERRIDE(handle, 2.0f); // Maximum movement rate
                PED::SET_PED_COMBAT_MOVEMENT(handle, 3); // Most aggressive
                PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, poodleGroupHash);

                // Remove blocking of non-temporary events so AI can always be retasked
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, false);

                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_DISABLE_IN_MP, true);

                // Set combat attributes for animals
                PED::SET_PED_COMBAT_ABILITY(handle, 3); // Elite pursuit
                PED::SET_PED_COMBAT_MOVEMENT(handle, 3); // Most aggressive
                PED::SET_PED_COMBAT_RANGE(handle, 2); // Far, to ensure pursuit
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, true); // Always fight
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 0, true); // Use cover (does not hurt animals)
                PED::SET_PED_FLEE_ATTRIBUTES(handle, 0, false); // Never flee

                // Increase perception ranges
                PED::SET_PED_SEEING_RANGE(handle, 200.0f);
                PED::SET_PED_HEARING_RANGE(handle, 200.0f);

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
                spawnedPeds.push_back(ped3);
            }

            int ped1Handle = ped1.GetHandle();
            int ped2Handle = ped2.GetHandle();
            int ped3Handle = ped3.GetHandle();

            YimMenu::FiberPool::Push([ped1Handle, ped2Handle, ped3Handle, targetHandle]() {
                auto start = std::chrono::steady_clock::now();
                bool targetAlive = true;
                while (std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start).count() < 120)
                {
                    // If target dies, break early
                    if (!ENTITY::DOES_ENTITY_EXIST(targetHandle) || ENTITY::IS_ENTITY_DEAD(targetHandle))
                    {
                        targetAlive = false;
                        break;
                    }

                    // Force immediate combat task re-assignment for all poodles while the target is alive
                    for (int h : {ped1Handle, ped2Handle, ped3Handle}) {
                        if (ENTITY::DOES_ENTITY_EXIST(h) && !ENTITY::IS_ENTITY_DEAD(h)
                            && ENTITY::DOES_ENTITY_EXIST(targetHandle) && !ENTITY::IS_ENTITY_DEAD(targetHandle)) {
                            TASK::TASK_COMBAT_PED(h, targetHandle, 0, 16);
                        }
                    }

                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(50));
                }

                {
                    std::lock_guard<std::mutex> lock(pedsMutex);
                    for (auto it = spawnedPeds.begin(); it != spawnedPeds.end();)
                    {
                        int h = it->GetHandle();
                        if (h == ped1Handle || h == ped2Handle || h == ped3Handle)
                        {
                            if (it->IsValid() && ENTITY::DOES_ENTITY_EXIST(h))
                            {
                                it->ForceControl();
                                ENTITY::SET_ENTITY_AS_MISSION_ENTITY(h, false, false);
                                TASK::CLEAR_PED_TASKS_IMMEDIATELY(h, false, true);
                                it->Delete();
                            }
                            it = spawnedPeds.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    if (spawnedPeds.empty() && poodleGroupInitialized)
                    {
                        PED::REMOVE_RELATIONSHIP_GROUP(poodleGroupHash);
                        poodleGroupInitialized = false;
                        poodleGroupHash = 0;
                    }
                }

                if (targetAlive)
                    Notifications::Show("PoodleAttack", "Poodles cleaned up (timeout)!", NotificationType::Info);
                else
                    Notifications::Show("PoodleAttack", "Poodles cleaned up (target dead)!", NotificationType::Info);
            });

            Notifications::Show("PoodleAttack", "Poodles spawned!", NotificationType::Success);
        }
    };

    std::vector<YimMenu::Ped> PoodleAttack::spawnedPeds;
    std::mutex PoodleAttack::pedsMutex;
    Hash PoodleAttack::poodleGroupHash = 0;
    bool PoodleAttack::poodleGroupInitialized = false;

    static PoodleAttack _PoodleAttack{};
}