#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Packet.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "util/Joaat.hpp"
#include <network/rlScPeerConnection.hpp>
#include <network/CNetworkScSessionPlayer.hpp>
#include <network/CNetworkScSession.hpp>
#include <chrono>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>

namespace YimMenu::Features
{
    FORCEINLINE constexpr void _FORCE_LIGHTNING_FLASH_AT_COORDS(float x, float y, float z, float p3)
    {
        return YimMenu::NativeInvoker::Invoke<2412, void, false>(x, y, z, p3);
    }

    class BeatNKick : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        static std::vector<int> spawnedPeds;
        static std::mutex pedsMutex;

        static Hash beatdownGroupHash;
        static bool beatdownGroupInitialized;

    public:
        BeatNKick() : PlayerCommand("beatnkick", "Beat-N-Kick", "Spawns two invincible peds to attack the player for 30 seconds, then triggers a firework and lightning if the target is alive, followed by a population reset kick")
        {
        }

        virtual void OnCall(Player player) override
        {
            if (player.GetId() == Self::GetPlayer().GetId())
            {
                Notifications::Show("BeatNKick", "Cannot use on yourself!", NotificationType::Error);
                return;
            }

            int playerId = player.GetId();

            if (!Pointers.ScSession || !(*Pointers.ScSession) || !(*Pointers.ScSession)->m_SessionMultiplayer)
            {
                Notifications::Show("BeatNKick", "Invalid session!", NotificationType::Error);
                return;
            }

            auto sessionPlayer = (*Pointers.ScSession)->m_SessionMultiplayer->GetPlayerByIndex(playerId);
            if (!sessionPlayer || (sessionPlayer->m_SessionPeer && sessionPlayer->m_SessionPeer->m_IsHost))
            {
                Notifications::Show("BeatNKick", "Cannot target host or invalid player!", NotificationType::Error);
                return;
            }

            auto targetPed = player.GetPed();
            if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
            {
                Notifications::Show("BeatNKick", "Player ped not found!", NotificationType::Error);
                return;
            }

            int targetHandle = targetPed.GetHandle();
            auto targetPos = targetPed.GetPosition();

            Hash pedModel = Joaat("CS_oddfellowspinhead");
            if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel))
            {
                Notifications::Show("BeatNKick", "Invalid ped model!", NotificationType::Error);
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
                Notifications::Show("BeatNKick", "Failed to load ped model!", NotificationType::Error);
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
                Notifications::Show("BeatNKick", "Failed to spawn peds!", NotificationType::Error);
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
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, beatdownGroupHash, beatdownGroupHash);
                    PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, beatdownGroupHash, PED::GET_PED_RELATIONSHIP_GROUP_HASH(targetHandle));
                    beatdownGroupInitialized = true;
                }
                else
                {
                    Notifications::Show("BeatNKick", "Failed to create relationship group!", NotificationType::Error);
                    ped1.Delete();
                    ped2.Delete();
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
                    Notifications::Show("BeatNKick", "Ped invalid during config!", NotificationType::Warning);
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

            Notifications::Show("BeatNKick", "Peds spawned!", NotificationType::Success);

            std::atomic<bool> beatDownComplete{false};

            YimMenu::FiberPool::Push([this, targetHandle, ped1Handle, ped2Handle, &beatDownComplete]() {
                Notifications::Show("BeatNKick", "Starting beatdown timer!", NotificationType::Info);
                auto startTime = std::chrono::steady_clock::now();

                while (true)
                {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - startTime).count();

                    if (elapsed >= 30)
                    {
                        Notifications::Show("BeatNKick", "30 seconds elapsed!", NotificationType::Info);
                        if (ENTITY::DOES_ENTITY_EXIST(targetHandle) && 
                            !ENTITY::IS_ENTITY_DEAD(targetHandle))
                        {
                            Notifications::Show("BeatNKick", "Triggering effects!", NotificationType::Info);
                            auto coords = ENTITY::GET_ENTITY_COORDS(targetHandle, false, true);
                            FIRE::ADD_EXPLOSION(coords.x, coords.y, coords.z,
                                               (int)ExplosionTypes::FIREWORK,
                                               10.0f, true, false, 5.0f);
                            _FORCE_LIGHTNING_FLASH_AT_COORDS(coords.x, coords.y, coords.z, 1.0f);
                            Notifications::Show("BeatNKick", "Firework and lightning triggered!", NotificationType::Success);
                        }
                        else
                        {
                            Notifications::Show("BeatNKick", "Target invalid or dead!", NotificationType::Warning);
                        }
                        break;
                    }

                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(50));
                }

                Notifications::Show("BeatNKick", "Cleaning up peds!", NotificationType::Info);
                for (int handle : {ped1Handle, ped2Handle})
                {
                    if (ENTITY::DOES_ENTITY_EXIST(handle))
                    {
                        TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, false, true);
                        int tempHandle = handle;
                        ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                        ENTITY::DELETE_ENTITY(&tempHandle);
                    }
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

                Notifications::Show("BeatNKick", "Peds cleaned up!", NotificationType::Info);
                beatDownComplete = true;
            });

            YimMenu::FiberPool::Push([this, playerId, player, &beatDownComplete]() {
                while (!beatDownComplete)
                {
                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(50));
                }

                Notifications::Show("BeatNKick", "Pausing before kick!", NotificationType::Info);
                YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(2000));

                Notifications::Show("BeatNKick", "Starting kick packets!", NotificationType::Info);

                if (!Pointers.ScSession || !(*Pointers.ScSession) || !(*Pointers.ScSession)->m_SessionMultiplayer)
                {
                    Notifications::Show("BeatNKick", "Invalid session, cannot kick!", NotificationType::Error);
                    return;
                }

                auto sessionPlayer = (*Pointers.ScSession)->m_SessionMultiplayer->GetPlayerByIndex(playerId);
                if (!sessionPlayer || (sessionPlayer->m_SessionPeer && sessionPlayer->m_SessionPeer->m_IsHost))
                {
                    Notifications::Show("BeatNKick", "Cannot kick host or invalid player!", NotificationType::Error);
                    return;
                }

                for (int i = 0; i < 5; i++)
                {
                    Notifications::Show("BeatNKick", "Preparing packet " + std::to_string(i + 1), NotificationType::Info);
                    Packet sync;
                    sync.WriteMessageHeader(NetMessageType::RESET_POPULATION);
                    sync.GetBuffer().Write<bool>(false, 1);
                    sync.GetBuffer().Write<bool>(false, 1);
                    sync.Send(player, 13, true, true);
                    Notifications::Show("BeatNKick", "Sent packet " + std::to_string(i + 1), NotificationType::Success);

                    YimMenu::ScriptMgr::Yield(std::chrono::milliseconds(100 + (rand() % 400)));
                }

                Notifications::Show("BeatNKick", "Kick completed!", NotificationType::Success);
            });
        }
    };

    std::vector<int> BeatNKick::spawnedPeds;
    std::mutex BeatNKick::pedsMutex;
    Hash BeatNKick::beatdownGroupHash = 0;
    bool BeatNKick::beatdownGroupInitialized = false;

    static BeatNKick _BeatNKick{};
} 