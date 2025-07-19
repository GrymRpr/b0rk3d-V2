#include "RideemCowboy.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/backend/Players.hpp"
#include "game/rdr/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "util/Joaat.hpp"
#include "game/backend/FiberPool.hpp"

namespace YimMenu::Features
{
    RideemCowboy::RideemCowboy() : PlayerCommand("rideemcowboy", "Rideem Cowboy", "Spawns a naked swimmer ped on the player's shoulders")
    {
    }

    void RideemCowboy::OnCall(Player player)
    {
        Notifications::Show("RideemCowboy", 
            "Player ID: " + std::to_string(player.GetId()) + 
            ", Self ID: " + std::to_string(Self::GetPlayer().GetId()), 
            NotificationType::Info);

        if (player.GetId() == Self::GetPlayer().GetId())
        {
            Notifications::Show("RideemCowboy", "Cannot use on yourself!", NotificationType::Error);
            return;
        }

        auto targetPed = player.GetPed();
        if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
        {
            Notifications::Show("RideemCowboy", "Player ped not found!", NotificationType::Error);
            return;
        }
        int targetPedHandle = targetPed.GetHandle();

        Hash pedModel = Joaat("RE_NAKEDSWIMMER_MALES_01");
        if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel))
        {
            Notifications::Show("RideemCowboy", "Invalid ped model!", NotificationType::Error);
            return;
        }
        STREAMING::REQUEST_MODEL(pedModel, false);
        int maxAttempts = 100;
        while (!STREAMING::HAS_MODEL_LOADED(pedModel) && maxAttempts-- > 0)
        {
            ScriptMgr::Yield(std::chrono::milliseconds(10));
        }
        if (!STREAMING::HAS_MODEL_LOADED(pedModel))
        {
            Notifications::Show("RideemCowboy", "Failed to load ped model!", NotificationType::Error);
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);
            return;
        }

        auto spawnPos = targetPed.GetPosition() + Vector3(0.0f, 0.0f, 0.7f);
        auto ped = Ped::Create(pedModel, spawnPos, targetPed.GetRotation().z);
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

        if (!ped || !ped.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
        {
            Notifications::Show("RideemCowboy", "Failed to spawn ped!", NotificationType::Error);
            if (ped && ped.IsValid()) ped.Delete();
            return;
        }

        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped.GetHandle(), true, true);
        ENTITY::SET_ENTITY_INVINCIBLE(ped.GetHandle(), true);
        ENTITY::SET_ENTITY_VISIBLE(ped.GetHandle(), true);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped.GetHandle(), true);
        PED::SET_PED_FLEE_ATTRIBUTES(ped.GetHandle(), 0, false); // Disable fleeing
        PED::SET_PED_CONFIG_FLAG(ped.GetHandle(), 297, true); // PCF_DisableFearReactions (RDR2-specific flag)

        ENTITY::FREEZE_ENTITY_POSITION(ped.GetHandle(), false);
        ENTITY::SET_ENTITY_COLLISION(ped.GetHandle(), false, false);

        ENTITY::ATTACH_ENTITY_TO_ENTITY(
            ped.GetHandle(),
            targetPedHandle,
            0,
            0.0f, 0.0f, 0.7f, // Same position as RideOnShoulders
            0.0f, 0.0f, 10.0f, // Same rotation as RideOnShoulders
            false, false, false, true, 2, true, false, false
        );

        const char* animDict = "script_story@gry1@ig@ig_15_archibald_sit_reading";
        const char* animName = "held_1hand_archibald";
        STREAMING::REQUEST_ANIM_DICT(animDict);
        maxAttempts = 100;
        while (!STREAMING::HAS_ANIM_DICT_LOADED(animDict) && maxAttempts-- > 0) {
            ScriptMgr::Yield(std::chrono::milliseconds(10));
        }
        if (STREAMING::HAS_ANIM_DICT_LOADED(animDict)) {
            TASK::TASK_PLAY_ANIM(
                ped.GetHandle(),
                animDict,
                animName,
                8.0f,    // Speed
                -8.0f,   // Speed multiplier
                -1,      // Duration
                1 | 16,  // FLAG: 1=repeat, 16=stay in anim when done
                0.0f,
                false, false, false, "", false
            );
        } else {
            Notifications::Show("RideemCowboy", "Failed to load animation!", NotificationType::Warning);
        }

        FiberPool::Push([pedHandle = ped.GetHandle(), targetId = player.GetId(), targetPedHandle, animDict, animName] {
            while (true) {
                if (!Players::GetPlayers().contains(targetId) ||
                    !ENTITY::DOES_ENTITY_EXIST(targetPedHandle) ||
                    ENTITY::IS_ENTITY_DEAD(targetPedHandle) ||
                    !ENTITY::DOES_ENTITY_EXIST(pedHandle)) {
                    break;
                }

                if (!ENTITY::IS_ENTITY_PLAYING_ANIM(pedHandle, animDict, animName, 3)) {
                    if (STREAMING::HAS_ANIM_DICT_LOADED(animDict)) {
                        TASK::TASK_PLAY_ANIM(
                            pedHandle,
                            animDict,
                            animName,
                            8.0f,
                            -8.0f,
                            -1,
                            1 | 16,
                            0.0f,
                            false, false, false, "", false
                        );
                    }
                }

                if (!ENTITY::IS_ENTITY_ATTACHED_TO_ENTITY(pedHandle, targetPedHandle)) {
                    ENTITY::ATTACH_ENTITY_TO_ENTITY(
                        pedHandle,
                        targetPedHandle,
                        0,
                        0.0f, 0.0f, 0.7f,
                        0.0f, 0.0f, 10.0f,
                        false, false, false, true, 2, true, false, false
                    );
                }

                ScriptMgr::Yield(std::chrono::milliseconds(1000));
            }
            if (ENTITY::DOES_ENTITY_EXIST(pedHandle)) {
                TASK::CLEAR_PED_TASKS_IMMEDIATELY(pedHandle, false, true);
                int tempHandle = pedHandle;
                ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                ENTITY::DELETE_ENTITY(&tempHandle);
            }
            Notifications::Show("RideemCowboy", "Ped cleaned up!", NotificationType::Info);
        });

        Notifications::Show("RideemCowboy", "Ped spawned on player's shoulders!", NotificationType::Success);
    }

    static RideemCowboy _RideemCowboy{};
}