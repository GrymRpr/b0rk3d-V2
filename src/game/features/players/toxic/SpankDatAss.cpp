#include "SpankDatAss.hpp"
#include "game/rdr/Natives.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "util/Joaat.hpp"

namespace YimMenu::Features {
    SpankDatAss::SpankDatAss() : PlayerCommand("spankdatass", "Spank Dat Ass", "Spawns a ped to spank the player") {}

    void SpankDatAss::OnCall(Player player) {
        // Debug: Log player and self IDs
        Notifications::Show("SpankDatAss", 
            "Player ID: " + std::to_string(player.GetId()) + 
            ", Self ID: " + std::to_string(Self::GetPlayer().GetId()), 
            NotificationType::Info);

        // Prevent self-targeting
        if (player.GetId() == Self::GetPlayer().GetId()) {
            Notifications::Show("SpankDatAss", "Cannot spank yourself!", NotificationType::Error);
            return;
        }

        // Validate target ped
        auto targetPed = player.GetPed();
        if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle())) {
            Notifications::Show("SpankDatAss", "Player ped not found!", NotificationType::Error);
            return;
        }
        int targetPedHandle = targetPed.GetHandle();

        // Load ped model
        Hash pedModel = Joaat("RE_NAKEDSWIMMER_MALES_01");
        if (!STREAMING::IS_MODEL_IN_CDIMAGE(pedModel) || !STREAMING::IS_MODEL_VALID(pedModel)) {
            Notifications::Show("SpankDatAss", "Invalid ped model!", NotificationType::Error);
            return;
        }
        STREAMING::REQUEST_MODEL(pedModel, false);
        int maxAttempts = 100;
        while (!STREAMING::HAS_MODEL_LOADED(pedModel) && maxAttempts-- > 0) {
            ScriptMgr::Yield(std::chrono::milliseconds(10));
        }
        if (!STREAMING::HAS_MODEL_LOADED(pedModel)) {
            Notifications::Show("SpankDatAss", "Failed to load ped model!", NotificationType::Error);
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);
            return;
        }

        // Spawn ped
        Vector3 position = targetPed.GetPosition();
        position.y -= 0.8f; // Position behind target
        auto ped = Ped::Create(pedModel, position, targetPed.GetRotation().z);
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedModel);

        // Validate spawned ped
        if (!ped || !ped.IsValid() || !ENTITY::DOES_ENTITY_EXIST(ped.GetHandle())) {
            Notifications::Show("SpankDatAss", "Failed to spawn ped!", NotificationType::Error);
            if (ped && ped.IsValid()) ped.Delete();
            return;
        }

        // Debug: Log ped handle and visibility
        Notifications::Show("SpankDatAss", 
            "Ped Handle: " + std::to_string(ped.GetHandle()) + 
            ", Visible: " + std::to_string(ENTITY::IS_ENTITY_VISIBLE(ped.GetHandle())), 
            NotificationType::Info);

        // Make ped mission entity, invincible, and visible
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped.GetHandle(), true, true);
        ENTITY::SET_ENTITY_INVINCIBLE(ped.GetHandle(), true);
        ENTITY::SET_ENTITY_VISIBLE(ped.GetHandle(), true);

        // Attach ped to target (use Attachments.cpp parameters)
        ENTITY::ATTACH_ENTITY_TO_ENTITY(
            ped.GetHandle(), 
            targetPedHandle, 
            0, // No specific bone index, as in Attachments.cpp
            0.0f, -0.8f, -0.5f, // Position from Attachments.cpp
            0.0f, 15.0f, -30.0f, // Rotation from Attachments.cpp
            false, false, false, true, 0, true, false, false
        );

        // Load and play animation (use Attachments.cpp parameters)
        const char* animDict = "script_re@peep_tom@spanking_cowboy";
        const char* animName = "spanking_idle_female";
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
                1.0f, 1.0f, -1, 1, 0.0f, // Parameters from Attachments.cpp
                false, false, false, "", false
            );
        } else {
            Notifications::Show("SpankDatAss", "Failed to load animation!", NotificationType::Warning);
        }

        // Set up cleanup fiber (persist until target is dead)
        FiberPool::Push([pedHandle = ped.GetHandle(), targetPedHandle, targetId = player.GetId()] {
            while (true) {
                if (!Players::GetPlayers().contains(targetId) || 
                    !ENTITY::DOES_ENTITY_EXIST(targetPedHandle) || 
                    ENTITY::IS_ENTITY_DEAD(targetPedHandle) || 
                    !ENTITY::DOES_ENTITY_EXIST(pedHandle)) {
                    break;
                }
                ScriptMgr::Yield(std::chrono::milliseconds(500));
            }
            if (ENTITY::DOES_ENTITY_EXIST(pedHandle)) {
                ENTITY::DETACH_ENTITY(pedHandle, true, true);
                TASK::CLEAR_PED_TASKS_IMMEDIATELY(pedHandle, false, true);
                int tempHandle = pedHandle;
                ENTITY::DELETE_ENTITY(&tempHandle);
            }
            Notifications::Show("SpankDatAss", "Ped cleaned up!", NotificationType::Info);
        });

        Notifications::Show("SpankDatAss", "Ped spawned to spank player!", NotificationType::Success);
    }

    // Register the command
    static SpankDatAss g_SpankDatAss;
}