#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "util/Joaat.hpp"
#include <chrono>
#include <vector>
#include <mutex>
#include <algorithm>

namespace YimMenu::Features
{
    FORCEINLINE constexpr void _FORCE_LIGHTNING_FLASH_AT_COORDS(float x, float y, float z, float p3)
    {
        return YimMenu::NativeInvoker::Invoke<2412, void, false>(x, y, z, p3);
    }

    class BeatDown2 : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        static std::vector<int> spawnedPeds;
        static std::mutex pedsMutex;

        static Hash beatdownGroupHash;
        static bool beatdownGroupInitialized;

    public:
        BeatDown2() : PlayerCommand("beatdown2", "Beat Down 2", "Spawns two invincible peds to attack the player for 30 seconds, then triggers a firework and lightning if alive")
        {
        }

        virtual void OnCall(Player player) override
        {
            if (player.GetId() == Self::GetPlayer().GetId())
            {
                Notifications::Show("BeatDown2", "You cannot use this on yourself!", NotificationType::Error);
                return;
            }

            auto targetPed = player.GetPed();
            if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
            {
                Notifications::Show("BeatDown2", "Player ped not found!", NotificationType::Error);
                return;
            }

            int targetHandle = targetPed.GetHandle();
            auto targetPos = targetPed.GetPosition();

            Hash pedModel = Joaat("CS_oddfellowspinhead");
            if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel))
            {
                Notifications::Show("BeatDown2", "Ped model invalid!", NotificationType::Error);
                return;
            }
            STREAMING::REQUEST_MODEL(pedModel, false);
            int maxAttempts = 100;
            while (!STREAMING::HAS_MODEL_LOADED(pedModel) && maxAttempts-- > 0)
            {
                YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(10));
            }
            if (!STREAMING::HAS_MODEL_LOADED(pedModel))
            {
                Notifications::Show("BeatDown2", "Failed to load ped model!", NotificationType::Error);
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);
                return;
            }

            auto spawnPos1 = targetPos + rage::fvector3(2.0f, 2.0f, 0.0f);
            auto spawnPos2 = targetPos + rage::fvector3(-2.0f, -2.0f, 0.0f);

            auto ped1 = Ped::Create(pedModel, spawnPos1, targetPed.GetRotation().z);
            auto ped2 = Ped::Create(pedModel, spawnPos2, targetPed.GetRotation().z);

            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

            if (!ped1 || !ped1.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped1.GetHandle()) ||
                !ped2 || !ped2.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped2.GetHandle()))
            {
                Notifications::Show("BeatDown2", "Failed to spawn peds!", NotificationType::Error);
                if (ped1 && ped1.IsValid()) ped1.Delete();
                if (ped2 && ped2.IsValid()) ped2.Delete();
                return;
            }

            int ped1Handle = ped1.GetHandle();
            int ped2Handle = ped2.GetHandle();

            if (!beatdownGroupInitialized)
            {
                if (PED::ADD_RELATIONSHIP_GROUP("BEATDOWN2_GROUP", &beatdownGroupHash))
                {
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, beatdownGroupHash, beatdownGroupHash); // Friendly to itself
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, beatdownGroupHash, PED::GET_PED_RELATIONSHIP_GROUP_HASH(targetHandle)); // Hate target
                    beatdownGroupInitialized = true;
                }
                else
                {
                    Notifications::Show("BeatDown2", "Failed to create relationship group!", NotificationType::Error);
                    if (ped1.IsValid()) ped1.Delete();
                    if (ped2.IsValid()) ped2.Delete();
                    return;
                }
            }

            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped1Handle, true, true);
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped2Handle, true, true);

            ped1.SetInvincible(true);
            ped2.SetInvincible(true);

            for (auto ped : {ped1, ped2})
            {
                int handle = ped.GetHandle();
                if (!ENTITY::DOES_ENTITY_EXIST(handle))
                {
                    Notifications::Show("BeatDown2", "Ped invalidated during config!", NotificationType::Warning);
                    continue;
                }

                PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, beatdownGroupHash);

                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS, false);
                PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_DISABLE_IN_MP, true);

                PED::SET_PED_COMBAT_ABILITY(handle, 3);
                PED::SET_PED_COMBAT_MOVEMENT(handle, 2);
                PED::SET_PED_COMBAT_RANGE(handle, 2);
                PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true);
                PED::SET_PED_FLEE_ATTRIBUTES(handle, 0, false);
                PED::SET_PED_SEEING_RANGE(handle, 50.0f);
                PED::SET_PED_HEARING_RANGE(handle, 50.0f);

                if (ENTITY::DOES_ENTITY_EXIST(targetHandle))
                {
                    TASK::TASK_COMBAT_PED(handle, targetHandle, 0, 16);
                }
            }

            {
                std::lock_guard<std::mutex> lock(pedsMutex);
                spawnedPeds.push_back(ped1Handle);
                spawnedPeds.push_back(ped2Handle);
            }

            YimMenu::FiberPool::Push([targetHandle, ped1Handle, ped2Handle]() {
                auto startTime = std::chrono::steady_clock::now();

                while (true)
                {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - startTime).count();

                    if (elapsed >= 30)
                    {
                        if (ENTITY::DOES_ENTITY_EXIST(targetHandle) && !ENTITY::IS_ENTITY_DEAD(targetHandle))
                        {
                            rage::fvector3 targetCoords = ENTITY::GET_ENTITY_COORDS(targetHandle, false, true);
                            FIRE::ADD_EXPLOSION(targetCoords.x, targetCoords.y, targetCoords.z, 
                                               (int)ExplosionTypes::FIREWORK, 
                                               10.0f, 
                                               true, 
                                               false, 
                                               5.0f);
                            _FORCE_LIGHTNING_FLASH_AT_COORDS(targetCoords.x, targetCoords.y, targetCoords.z, 1.0f);
                            Notifications::Show("BeatDown2", "Target survived! Firework and lightning triggered!", NotificationType::Success);
                        }
                        break;
                    }

                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(50));
                }

                if (ENTITY::DOES_ENTITY_EXIST(ped1Handle))
                {
                    TASK::CLEAR_PED_TASKS_IMMEDIATELY(ped1Handle, false, true);
                    int tempHandle = ped1Handle;
                    ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                    ENTITY::DELETE_ENTITY(&tempHandle);
                }
                if (ENTITY::DOES_ENTITY_EXIST(ped2Handle))
                {
                    TASK::CLEAR_PED_TASKS_IMMEDIATELY(ped2Handle, false, true);
                    int tempHandle = ped2Handle;
                    ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                    ENTITY::DELETE_ENTITY(&tempHandle);
                }

                {
                    std::lock_guard<std::mutex> lock(pedsMutex);
                    spawnedPeds.erase(
                        std::remove_if(spawnedPeds.begin(), spawnedPeds.end(),
                            [ped1Handle, ped2Handle](int handle) {
                                return handle == ped1Handle || handle == ped2Handle;
                            }),
                        spawnedPeds.end());
                }

                Notifications::Show("BeatDown2", "Peds cleaned up after 30 seconds!", NotificationType::Info);
            });

            Notifications::Show("BeatDown2", "Peds spawned to beat down the player!", NotificationType::Success);
        }
    };

    std::vector<int> BeatDown2::spawnedPeds;
    std::mutex BeatDown2::pedsMutex;
    Hash BeatDown2::beatdownGroupHash = 0;
    bool BeatDown2::beatdownGroupInitialized = false;

    static BeatDown2 _BeatDown2{};
} 