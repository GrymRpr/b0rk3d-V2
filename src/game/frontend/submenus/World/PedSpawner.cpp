#include "PedSpawner.hpp"
#include "core/commands/HotkeySystem.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/NativeHooks.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/data/PedModels.hpp"
#include <chrono>
#include <algorithm>
#include <map>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <set>

namespace YimMenu::Submenus
{
    #define LOG(x) std::cout << "[PedSpawner] " << x << std::endl

    void GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(rage::scrNativeCallContext* ctx)
    {
        if (ctx->GetArg<int>(0) == "mp_intro"_J)
        {
            ctx->SetReturnValue<int>(1);
        }
        else
        {
            ctx->SetReturnValue<int>(SCRIPTS::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(ctx->GetArg<int>(0)));
        }
    }

    void _GET_META_PED_TYPE(rage::scrNativeCallContext* ctx)
    {
        ctx->SetReturnValue<int>(4);
    }

    static bool IsPedModelInList(const std::string& model)
    {
        return Data::g_PedModels.contains(Joaat(model));
    }

    static int PedSpawnerInputCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
        {
            std::string newText{};
            std::string inputLower = data->Buf;
            std::transform(inputLower.begin(), inputLower.end(), inputLower.begin(), ::tolower);
            for (const auto& [key, model] : Data::g_PedModels)
            {
                if (!model) continue;
                std::string pedModelLower = model;
                std::transform(pedModelLower.begin(), pedModelLower.end(), pedModelLower.begin(), ::tolower);
                if (pedModelLower.find(inputLower) != std::string::npos)
                {
                    newText = model;
                }
            }

            if (!newText.empty())
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, newText.c_str());
            }

            return 1;
        }
        return 0;
    }

    enum class LassoFlags : int
    {
        LHF_CAN_BE_LASSOED = 0,
        LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI = 2,
        LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS = 3,
        LHF_DISABLE_IN_MP = 5
    };

    static void ApplyCompanionSettings(YimMenu::Ped& ped)
    {
        static Hash companionGroupHash = 0;
        static bool companionGroupInitialized = false;

        if (!ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
        {
            return;
        }

        if (!companionGroupInitialized)
        {
            if (!PED::ADD_RELATIONSHIP_GROUP("PLAYER_FRIENDLY_COMPANION", &companionGroupHash))
            {
                return;
            }
            companionGroupInitialized = true;
        }

        int group = PED::GET_PED_GROUP_INDEX(YimMenu::Self::GetPed().GetHandle());
        if (!PED::DOES_GROUP_EXIST(group))
        {
            group = PED::CREATE_GROUP(0);
            PED::SET_PED_AS_GROUP_LEADER(YimMenu::Self::GetPed().GetHandle(), group, true);
        }

        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped.GetHandle(), false, false);
        PED::SET_PED_AS_GROUP_MEMBER(ped.GetHandle(), group);
        PED::SET_PED_CAN_BE_TARGETTED_BY_PLAYER(ped.GetHandle(), YimMenu::Self::GetPlayer().GetId(), false);
        PED::SET_PED_RELATIONSHIP_GROUP_HASH(ped.GetHandle(), companionGroupHash);

        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, companionGroupHash, Joaat("PLAYER"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, companionGroupHash, Joaat("REL_CIVMALE"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, companionGroupHash, Joaat("REL_CRIMINALS"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, companionGroupHash, Joaat("REL_GANG"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, companionGroupHash, Joaat("REL_COP"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, companionGroupHash, Joaat("REL_WILD_ANIMAL"));

        PED::SET_PED_COMBAT_ABILITY(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_MOVEMENT(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_RANGE(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 46, true);
        PED::SET_PED_FLEE_ATTRIBUTES(ped.GetHandle(), 0, false);
        TASK::TASK_COMBAT_HATED_TARGETS_AROUND_PED(ped.GetHandle(), 150.0f, 0, 0);

        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 5, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 0, true);
        PED::SET_PED_SEEING_RANGE(ped.GetHandle(), 75.0f);
        PED::SET_PED_HEARING_RANGE(ped.GetHandle(), 75.0f);

        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_DISABLE_IN_MP), true);

        ped.SetConfigFlag(PedConfigFlag::DisableEvasiveStep, false);
        ped.SetConfigFlag(PedConfigFlag::DisableExplosionReactions, false);
        ped.SetConfigFlag(PedConfigFlag::CanAttackFriendly, true);
        ped.SetConfigFlag(PedConfigFlag::_0x16A14D9A, false);
        ped.SetConfigFlag(PedConfigFlag::DisableShockingEvents, false);
        ped.SetConfigFlag(PedConfigFlag::DisableHorseGunshotFleeResponse, false);
        ped.SetConfigFlag(PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
        ped.SetConfigFlag(PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped.GetHandle(), false);

        if (ped.IsAnimal())
        {
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 104, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 105, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 10, 2.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 146, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 113, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 114, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 115, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 116, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 117, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 118, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 119, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 111, 0.0f);
            FLOCK::SET_ANIMAL_TUNING_FLOAT_PARAM(ped.GetHandle(), 107, 0.0f);
        }

        auto blip = MAP::BLIP_ADD_FOR_ENTITY(Joaat("BLIP_STYLE_COMPANION"), ped.GetHandle());
        MAP::BLIP_ADD_MODIFIER(blip, Joaat("BLIP_MODIFIER_COMPANION_DOG"));
    }

    static void ApplyKillEmAllSettings(YimMenu::Ped& ped)
    {
        static Hash killEmAllGroupHash = 0;
        static bool killEmAllGroupInitialized = false;

        static const std::unordered_set<Hash> blacklistedModels = {   //I AM A TEST
            static_cast<Hash>(Joaat("A_M_M_ShopKeep_01")),
            static_cast<Hash>(Joaat("CS_SHOPKEEP_ANNABELLE")),
            static_cast<Hash>(Joaat("RE_SHOPKEEP_01")),
            static_cast<Hash>(Joaat("RCSP_SHOPKEEP_01")),
            static_cast<Hash>(Joaat("U_M_M_ARMGENERALSTOREOWNER_01")),
            static_cast<Hash>(Joaat("U_M_M_BlWGeneralStoreOwner_01")),
            static_cast<Hash>(Joaat("U_M_M_NbxGeneralStoreOwner_01")),
        };

        if (!ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
        {
            return;
        }

        if (!killEmAllGroupInitialized)
        {
            if (!PED::ADD_RELATIONSHIP_GROUP("KILL_EM_ALL_TERMINATOR", &killEmAllGroupHash))
            {
                return;
            }
            killEmAllGroupInitialized = true;
        }

        int pedHandle = ped.GetHandle();
        int selfPedHandle = Self::GetPed().GetHandle();

        PED::SET_PED_RELATIONSHIP_GROUP_HASH(pedHandle, killEmAllGroupHash);

        if (ENTITY::DOES_ENTITY_EXIST(selfPedHandle))
        {
            Hash selfGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(selfPedHandle);
            PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, killEmAllGroupHash, selfGroup);
            PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, selfGroup, killEmAllGroupHash);
        }
        
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("PLAYER"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_CIVMALE"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_CIVFEMALE"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_CRIMINALS"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_GANG"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_COP"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_GANG_DUTCH"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_GANG_MICAH"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, killEmAllGroupHash, Joaat("REL_GUNSLINGERS"));
        
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, killEmAllGroupHash, Joaat("REL_WILD_ANIMAL"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, killEmAllGroupHash, Joaat("REL_DOMESTIC_ANIMAL"));

        PED::SET_PED_CAN_BE_TARGETTED_BY_PLAYER(pedHandle, YimMenu::Self::GetPlayer().GetId(), false);

        PED::SET_PED_COMBAT_ABILITY(pedHandle, 3);
        PED::SET_PED_COMBAT_MOVEMENT(pedHandle, 3);
        PED::SET_PED_COMBAT_RANGE(pedHandle, 3);
        PED::SET_PED_ACCURACY(pedHandle, 100);
        
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 0, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 1, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 2, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 3, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 5, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 13, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 20, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 21, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 22, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 27, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 28, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 38, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 39, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 42, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 46, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 50, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 58, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 60, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(pedHandle, 61, true);

        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 0, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 1, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 2, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 4, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 8, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 16, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 32, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 64, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 128, false);
        PED::SET_PED_FLEE_ATTRIBUTES(pedHandle, 256, false);

        PED::SET_PED_SEEING_RANGE(pedHandle, 500.0f);
        PED::SET_PED_HEARING_RANGE(pedHandle, 500.0f);
        PED::SET_PED_VISUAL_FIELD_PERIPHERAL_RANGE(pedHandle, 180.0f);
        PED::SET_PED_VISUAL_FIELD_CENTER_ANGLE(pedHandle, 90.0f);

        ped.SetStamina(100.0f);
        ped.SetHealth(1000.0f);
        PED::SET_PED_MOVE_RATE_OVERRIDE(pedHandle, 4.0f);
        
        if (!ped.IsAnimal())
        {
            WEAPON::GIVE_WEAPON_TO_PED(pedHandle, Joaat("WEAPON_MELEE_KNIFE"), 1, true, true, 0, false, 0.5f, 1.0f, 1.0f, false, 0, false);
            WEAPON::GIVE_WEAPON_TO_PED(pedHandle, Joaat("WEAPON_REVOLVER_LEMAT"), 500, true, true, 0, false, 0.5f, 1.0f, 1.0f, false, 0, false);
            WEAPON::GIVE_WEAPON_TO_PED(pedHandle, Joaat("WEAPON_REPEATER_WINCHESTER"), 500, true, true, 0, false, 0.5f, 1.0f, 1.0f, false, 0, false);
        }

        PED::SET_PED_LASSO_HOGTIE_FLAG(pedHandle, static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_DISABLE_IN_MP), true);
        PED::SET_PED_CONFIG_FLAG(pedHandle, 187, true);
        PED::SET_PED_CONFIG_FLAG(pedHandle, 185, true);

        ped.SetConfigFlag(PedConfigFlag::DisableEvasiveStep, false);
        ped.SetConfigFlag(PedConfigFlag::DisableExplosionReactions, true);
        ped.SetConfigFlag(PedConfigFlag::CanAttackFriendly, true);
        ped.SetConfigFlag(PedConfigFlag::_0x16A14D9A, false);
        ped.SetConfigFlag(PedConfigFlag::DisableShockingEvents, true);
        ped.SetConfigFlag(PedConfigFlag::DisableHorseGunshotFleeResponse, true);
        ped.SetConfigFlag(PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
        ped.SetConfigFlag(PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(pedHandle, false);

        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(pedHandle, true, true);

        Hash localGroupHash = killEmAllGroupHash;
        FiberPool::Push([pedHandle, localGroupHash, selfPedHandle] {
            std::chrono::steady_clock::time_point lastTargetScan = std::chrono::steady_clock::now();
            std::chrono::steady_clock::time_point lastTargetSwitch = std::chrono::steady_clock::now();
            int lastKnownTarget = 0;
            
            while (ENTITY::DOES_ENTITY_EXIST(pedHandle) && !ENTITY::IS_ENTITY_DEAD(pedHandle))
            {
                Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(pedHandle, true, false);
                auto now = std::chrono::steady_clock::now();
                
                if (lastKnownTarget != 0 && 
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTargetSwitch).count() >= 5000)
                {
                    lastKnownTarget = 0;
                    TASK::CLEAR_PED_TASKS(pedHandle, true, false);
                }

                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTargetScan).count() >= 300)
                {
                    lastTargetScan = now;
                    
                    int targetHandle = 0;
                    float closestDistance = 1000.0f;
                    
                    for (int i = 0; i < 32; i++)
                    {
                        if (NETWORK::NETWORK_IS_PLAYER_ACTIVE(i) && i != YimMenu::Self::GetPlayer().GetId())
                        {
                            int playerPed = PLAYER::GET_PLAYER_PED(i);
                            if (ENTITY::DOES_ENTITY_EXIST(playerPed) && !ENTITY::IS_ENTITY_DEAD(playerPed))
                            {
                                Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true, false);
                                float distance = MISC::GET_DISTANCE_BETWEEN_COORDS(pedPos.x, pedPos.y, pedPos.z, playerPos.x, playerPos.y, playerPos.z, true);
                                if (distance < closestDistance)
                                {
                                    closestDistance = distance;
                                    targetHandle = playerPed;
                                }
                            }
                        }
                    }
                    
                    if (targetHandle == 0)
                    {
                        int closestPed = 0;
                        if (PED::GET_CLOSEST_PED(pedPos.x, pedPos.y, pedPos.z, 200.0f, true, true, &closestPed, true, true, true, -1))
                        {
                            if (ENTITY::DOES_ENTITY_EXIST(closestPed) && closestPed != pedHandle && closestPed != selfPedHandle)
                            {
                                Hash targetGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(closestPed);
                                Hash targetModel = ENTITY::GET_ENTITY_MODEL(closestPed);
                                bool isEssential = PED::GET_PED_CONFIG_FLAG(closestPed, 223, true);
                                
                                bool isBlacklisted = blacklistedModels.find(targetModel) != blacklistedModels.end();
                                
                                if (targetGroup != Joaat("REL_WILD_ANIMAL") && 
                                    targetGroup != Joaat("REL_DOMESTIC_ANIMAL") &&
                                    targetGroup != localGroupHash &&
                                    !ENTITY::IS_ENTITY_DEAD(closestPed) &&
                                    !isEssential &&
                                    !isBlacklisted)
                                {
                                    Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(closestPed, true, false);
                                    float distance = MISC::GET_DISTANCE_BETWEEN_COORDS(pedPos.x, pedPos.y, pedPos.z, targetPos.x, targetPos.y, targetPos.z, true);
                                    if (distance < closestDistance)
                                    {
                                        closestDistance = distance;
                                        targetHandle = closestPed;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (targetHandle != 0 && (targetHandle != lastKnownTarget || 
                        !ENTITY::DOES_ENTITY_EXIST(lastKnownTarget) || 
                        ENTITY::IS_ENTITY_DEAD(lastKnownTarget)))
                    {
                        lastKnownTarget = targetHandle;
                        lastTargetSwitch = now;
                        TASK::CLEAR_PED_TASKS(pedHandle, true, false);
                        TASK::TASK_COMBAT_PED(pedHandle, targetHandle, 0, 16);
                        TASK::TASK_COMBAT_HATED_TARGETS_AROUND_PED(pedHandle, 200.0f, 0, 0);
                        
                        Hash currentWeapon;
                        if (WEAPON::GET_CURRENT_PED_WEAPON(pedHandle, &currentWeapon, true, 0, false))
                        {
                            if (currentWeapon == Joaat("WEAPON_UNARMED"))
                            {
                                WEAPON::SET_CURRENT_PED_WEAPON(pedHandle, Joaat("WEAPON_MELEE_HATCHET_MELEEONLY"), true, 0, false, false);
                            }
                        }
                    }
                }
                
                if (!PED::IS_PED_IN_COMBAT(pedHandle, 0))
                {
                    TASK::TASK_COMBAT_HATED_TARGETS_AROUND_PED(pedHandle, 200.0f, 0, 0);
                }
                
                if (ENTITY::GET_ENTITY_HEALTH(pedHandle) < 800)
                {
                    ENTITY::SET_ENTITY_HEALTH(pedHandle, 1000, 0);
                }
                
                ScriptMgr::Yield(std::chrono::milliseconds(100));
            }
            
            if (ENTITY::DOES_ENTITY_EXIST(pedHandle))
            {
                TASK::CLEAR_PED_TASKS_IMMEDIATELY(pedHandle, false, true);
                int tempHandle = pedHandle;
                ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                ENTITY::DELETE_ENTITY(&tempHandle);
            }
        });
        
        auto blip = MAP::BLIP_ADD_FOR_ENTITY(Joaat("BLIP_STYLE_ENEMY"), pedHandle);
        MAP::BLIP_ADD_MODIFIER(blip, Joaat("BLIP_MODIFIER_ENEMY_GANG"));
    }

    static void ApplyBodyGuardSettings(YimMenu::Ped& ped)
    {
        static Hash bodyGuardPosseGroupHash = 0;
        static bool bodyGuardPosseGroupInitialized = false;

        if (!ENTITY::DOES_ENTITY_EXIST(ped.GetHandle()))
        {
            return;
        }

        if (!bodyGuardPosseGroupInitialized)
        {
            if (!PED::ADD_RELATIONSHIP_GROUP("BODYGUARD_POSSE", &bodyGuardPosseGroupHash))
            {
                return;
            }
            bodyGuardPosseGroupInitialized = true;
        }

        PED::SET_PED_RELATIONSHIP_GROUP_HASH(ped.GetHandle(), bodyGuardPosseGroupHash);

        int selfPedHandle = Self::GetPed().GetHandle();
        if (ENTITY::DOES_ENTITY_EXIST(selfPedHandle))
        {
            PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, bodyGuardPosseGroupHash, PED::GET_PED_RELATIONSHIP_GROUP_HASH(selfPedHandle));
        }
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, bodyGuardPosseGroupHash, Joaat("REL_WILD_ANIMAL"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, bodyGuardPosseGroupHash, Joaat("REL_CIVMALE"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, bodyGuardPosseGroupHash, Joaat("REL_CRIMINALS"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, bodyGuardPosseGroupHash, Joaat("REL_GANG"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, bodyGuardPosseGroupHash, Joaat("REL_COP"));
        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, bodyGuardPosseGroupHash, Joaat("PLAYER"));

        PED::SET_PED_CAN_BE_TARGETTED_BY_PLAYER(ped.GetHandle(), YimMenu::Self::GetPlayer().GetId(), false);

        PED::SET_PED_COMBAT_ABILITY(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_MOVEMENT(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_RANGE(ped.GetHandle(), 3);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 46, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 5, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(ped.GetHandle(), 0, true);
        PED::SET_PED_FLEE_ATTRIBUTES(ped.GetHandle(), 0, false);
        PED::SET_PED_SEEING_RANGE(ped.GetHandle(), 500.0f);
        PED::SET_PED_HEARING_RANGE(ped.GetHandle(), 500.0f);

        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_AI), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS), false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(ped.GetHandle(), static_cast<int>(LassoFlags::LHF_DISABLE_IN_MP), true);

        ped.SetConfigFlag(PedConfigFlag::DisableEvasiveStep, false);
        ped.SetConfigFlag(PedConfigFlag::DisableExplosionReactions, false);
        ped.SetConfigFlag(PedConfigFlag::CanAttackFriendly, true);
        ped.SetConfigFlag(PedConfigFlag::_0x16A14D9A, false);
        ped.SetConfigFlag(PedConfigFlag::DisableShockingEvents, false);
        ped.SetConfigFlag(PedConfigFlag::DisableHorseGunshotFleeResponse, false);
        ped.SetConfigFlag(PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
        ped.SetConfigFlag(PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped.GetHandle(), false);

        int group = PED::GET_PED_GROUP_INDEX(YimMenu::Self::GetPed().GetHandle());
        if (!PED::DOES_GROUP_EXIST(group))
        {
            group = PED::CREATE_GROUP(0);
            PED::SET_PED_AS_GROUP_LEADER(YimMenu::Self::GetPed().GetHandle(), group, true);
        }
        PED::SET_PED_AS_GROUP_MEMBER(ped.GetHandle(), group);

        Hash localGroupHash = bodyGuardPosseGroupHash;
        FiberPool::Push([pedHandle = ped.GetHandle(), localGroupHash] {
            bool vehicleEntryPending = false;
            int playerOutOfVehicleTicks = 0;
            while (ENTITY::DOES_ENTITY_EXIST(pedHandle))
            {
                YimMenu::Ped playerPed = Self::GetPed();
                if (!playerPed.IsValid() || ENTITY::IS_ENTITY_DEAD(pedHandle))
                    break;

                int nearestAttackerHandle = 0;
                Vector3 playerPos = playerPed.GetPosition();

                if (PED::GET_CLOSEST_PED(playerPos.x, playerPos.y, playerPos.z, 50.0f, 0, 1, &nearestAttackerHandle, 1, 0, 0, -1) &&
                    nearestAttackerHandle != 0)
                {
                    YimMenu::Ped nearestAttacker(nearestAttackerHandle);
                    if (nearestAttackerHandle != pedHandle && nearestAttackerHandle != playerPed.GetHandle() &&
                        ENTITY::DOES_ENTITY_EXIST(nearestAttackerHandle))
                    {
                        if (!PED::IS_PED_IN_COMBAT(nearestAttacker.GetHandle(), playerPed.GetHandle()))
                        {
                            nearestAttackerHandle = 0;
                        }
                    }
                    else
                    {
                        nearestAttackerHandle = 0;
                    }
                }

                if (nearestAttackerHandle != 0)
                {
                    Hash attackerGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(nearestAttackerHandle);
                    if (attackerGroup == Joaat("REL_COP"))
                    {
                        PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, localGroupHash, Joaat("REL_COP"));
                    }
                    if (PED::IS_PED_IN_ANY_VEHICLE(pedHandle, false))
                    {
                        int pedVehicle = PED::GET_VEHICLE_PED_IS_USING(pedHandle);
                        if (ENTITY::DOES_ENTITY_EXIST(pedVehicle))
                        {
                            TASK::TASK_LEAVE_VEHICLE(pedHandle, pedVehicle, 0, 0);
                        }
                    }
                    TASK::TASK_COMBAT_PED(pedHandle, nearestAttackerHandle, 0, 16);
                    vehicleEntryPending = false;
                    playerOutOfVehicleTicks = 0;
                }
                else
                {
                    float minDistance = 50.0f;
                    int wantedTargetHandle = 0;
                    for (int i = 0; i < 32; i++)
                    {
                        if (NETWORK::NETWORK_IS_PLAYER_ACTIVE(i) && i != YimMenu::Self::GetPlayer().GetId())
                        {
                            int wantedLevel = PLAYER::GET_PLAYER_WANTED_LEVEL(i);
                            if (wantedLevel > 0)
                            {
                                int otherPlayerHandle = PLAYER::GET_PLAYER_PED(i);
                                if (ENTITY::DOES_ENTITY_EXIST(otherPlayerHandle))
                                {
                                    YimMenu::Ped otherPlayerPed(otherPlayerHandle);
                                    if (otherPlayerPed.IsValid() && !otherPlayerPed.IsDead())
                                    {
                                        Vector3 otherPos = otherPlayerPed.GetPosition();
                                        float distance = MISC::GET_DISTANCE_BETWEEN_COORDS(
                                            playerPos.x, playerPos.y, playerPos.z,
                                            otherPos.x, otherPos.y, otherPos.z, true);
                                        if (distance < minDistance)
                                        {
                                            minDistance = distance;
                                            wantedTargetHandle = otherPlayerHandle;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (wantedTargetHandle != 0)
                    {
                        if (PED::IS_PED_IN_ANY_VEHICLE(pedHandle, false))
                        {
                            int pedVehicle = PED::GET_VEHICLE_PED_IS_USING(pedHandle);
                            if (ENTITY::DOES_ENTITY_EXIST(pedVehicle))
                            {
                                TASK::TASK_LEAVE_VEHICLE(pedHandle, pedVehicle, 0, 0);
                            }
                        }
                        TASK::TASK_COMBAT_PED(pedHandle, wantedTargetHandle, 0, 16);
                        vehicleEntryPending = false;
                        playerOutOfVehicleTicks = 0;
                    }
                    else
                    {
                        bool playerInVehicle = PED::IS_PED_IN_ANY_VEHICLE(playerPed.GetHandle(), false);
                        bool pedInVehicle = PED::IS_PED_IN_ANY_VEHICLE(pedHandle, false);

                        if (vehicleEntryPending && TASK::GET_SCRIPT_TASK_STATUS(pedHandle, Joaat("SCRIPT_TASK_ENTER_VEHICLE"), true) == 1)
                        {
                            ScriptMgr::Yield(std::chrono::milliseconds(500));
                            continue;
                        }

                        if (playerInVehicle && !pedInVehicle && !vehicleEntryPending)
                        {
                            int vehicle = PED::GET_VEHICLE_PED_IS_USING(playerPed.GetHandle());
                            if (ENTITY::DOES_ENTITY_EXIST(vehicle))
                            {
                                if (PED::IS_PED_ON_MOUNT(playerPed.GetHandle()))
                                {
                                    int mount = PED::GET_MOUNT(playerPed.GetHandle());
                                    if (ENTITY::DOES_ENTITY_EXIST(mount))
                                    {
                                        int closestPed = 0;
                                        if (PED::GET_CLOSEST_PED(playerPos.x, playerPos.y, playerPos.z, 50.0f, true, false, &closestPed, true, true, true, 4))
                                        {
                                            if (ENTITY::DOES_ENTITY_EXIST(closestPed) && ENTITY::IS_ENTITY_A_PED(closestPed))
                                            {
                                                YimMenu::Ped closest(closestPed);
                                                if (closest.IsAnimal() && !PED::IS_PED_ON_MOUNT(closestPed))
                                                {
                                                    TASK::TASK_MOUNT_ANIMAL(pedHandle, closestPed, -1, -1, 1.0f, 1, 0, 0);
                                                    vehicleEntryPending = false;
                                                    playerOutOfVehicleTicks = 0;
                                                    ScriptMgr::Yield(std::chrono::milliseconds(500));
                                                    continue;
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (VEHICLE::IS_VEHICLE_SEAT_FREE(vehicle, -2))
                                {
                                    TASK::TASK_ENTER_VEHICLE(pedHandle, vehicle, -1, -2, 1.0f, 1, 0);
                                    vehicleEntryPending = true;
                                    playerOutOfVehicleTicks = 0;
                                }
                                else
                                {
                                    TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(
                                        pedHandle, playerPed.GetHandle(),
                                        -1.0f, -1.0f, 0.0f, 3.0f, -1, 2.0f, true, false, false, true, false, false);
                                    vehicleEntryPending = false;
                                    playerOutOfVehicleTicks = 0;
                                }
                            }
                        }
                        else if (!playerInVehicle && pedInVehicle)
                        {
                            playerOutOfVehicleTicks++;
                            if (playerOutOfVehicleTicks >= 2)
                            {
                                int pedVehicle = PED::GET_VEHICLE_PED_IS_USING(pedHandle);
                                if (ENTITY::DOES_ENTITY_EXIST(pedVehicle))
                                {
                                    TASK::TASK_LEAVE_VEHICLE(pedHandle, pedVehicle, 0, 0);
                                }
                                vehicleEntryPending = false;
                                playerOutOfVehicleTicks = 0;
                            }
                        }
                        else if (!playerInVehicle && !pedInVehicle)
                        {
                            TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(
                                pedHandle, playerPed.GetHandle(),
                                -1.0f, -1.0f, 0.0f, 3.0f, -1, 2.0f, true, false, false, true, false, false);
                            vehicleEntryPending = false;
                            playerOutOfVehicleTicks = 0;
                        }
                        else
                        {
                            vehicleEntryPending = false;
                            playerOutOfVehicleTicks = 0;
                        }
                    }
                }

                ScriptMgr::Yield(std::chrono::milliseconds(500));
            }

            if (ENTITY::DOES_ENTITY_EXIST(pedHandle))
            {
                TASK::CLEAR_PED_TASKS_IMMEDIATELY(pedHandle, false, true);
                int tempHandle = pedHandle;
                ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&tempHandle);
                ENTITY::DELETE_ENTITY(&tempHandle);
            }
        });

        auto blip = MAP::BLIP_ADD_FOR_ENTITY(Joaat("BLIP_STYLE_FRIEND"), ped.GetHandle());
        MAP::BLIP_ADD_MODIFIER(blip, Joaat("BLIP_MODIFIER_FRIENDLY"));
    }

    static const std::map<std::string, std::set<int>> legendaryVariants = {
        {"a_c_alligator_02", {0}},  
        {"a_c_bear_01", {9}},
        {"a_c_beaver_01", {1}},
        {"a_c_bighornram_01", {12}},
        {"a_c_boar_01", {0}},
        {"a_c_boarlegendary_01", {0}},
        {"a_c_buck_01", {4}},
        {"a_c_buffalo_01", {4}},
        {"a_c_cougar_01", {2, 5}},
        {"a_c_coyote_01", {1}},
        {"a_c_elk_01", {1}},
        {"a_c_fishbluegil_01", {0}},
        {"a_c_fishbullheadcat_01", {0}},
        {"a_c_fishchainpickerel_01", {0}},
        {"a_c_fishchannelcatfish_01", {0}},
        {"a_c_fishlargemouthbass_01", {0}},
        {"a_c_fishlongnosegar_01", {0}},
        {"a_c_fishmuskie_01", {0}},
        {"a_c_fishnorthernpike_01", {0}},
        {"a_c_fishperch_01", {0}},
        {"a_c_fishrainbowtrout_01", {0}},
        {"a_c_fishredfinpickerel_01", {0}},
        {"a_c_fishrockbass_01", {0}},
        {"a_c_fishsmallmouthbass_01", {0}},
        {"a_c_fishsockeyesalmon_01", {0}},
        {"a_c_fox_01", {3}},
        {"a_c_moose_01", {6}},
        {"a_c_panther_01", {1}},
        {"a_c_wolf", {3}},
        {"mp_a_c_alligator_01", {0, 1, 2, 3}},
        {"mp_a_c_bear_01", {1, 2, 3}},
        {"mp_a_c_beaver_01", {0, 1, 2}},
        {"mp_a_c_bighornram_01", {0, 1, 2, 3}},
        {"mp_a_c_boar_01", {0, 1, 2, 3}},
        {"mp_a_c_buck_01", {2, 3, 4, 5}},
        {"mp_a_c_buffalo_01", {0, 1, 2}},
        {"mp_a_c_cougar_01", {0, 1, 2}}, 
        {"mp_a_c_coyote_01", {0, 1, 2}},
        {"mp_a_c_elk_01", {1, 2, 3}},
        {"mp_a_c_fox_01", {0, 1, 2}},
        {"mp_a_c_wolf_01", {0, 1, 2}},
    };

    static const std::map<std::string, int> legendaryPresets = {
        {"a_c_alligator_02", 0},
        {"a_c_bear_01", 9},
        {"a_c_beaver_01", 2},
        {"a_c_bighornram_01", 15},
        {"a_c_boar_01", 2},
        {"a_c_boarlegendary_01", 1},
        {"a_c_buck_01", 5},
        {"a_c_buffalo_01", 13},
        {"a_c_cougar_01", 4},
        {"a_c_coyote_01", 5},
        {"a_c_elk_01", 6},
        {"a_c_fishbluegil_01", 1},
        {"a_c_fishbullheadcat_01", 1},
        {"a_c_fishchainpickerel_01", 1},
        {"a_c_fishchannelcatfish_01", 1},
        {"a_c_fishlargemouthbass_01", 1},
        {"a_c_fishlongnosegar_01", 1},
        {"a_c_fishmuskie_01", 1},
        {"a_c_fishnorthernpike_01", 1},
        {"a_c_fishperch_01", 1},
        {"a_c_fishrainbowtrout_01", 1},
        {"a_c_fishredfinpickerel_01", 1},
        {"a_c_fishrockbass_01", 1},
        {"a_c_fishsmallmouthbass_01", 1},
        {"a_c_fishsockeyesalmon_01", 1},
        {"a_c_fox_01", 6},
        {"a_c_moose_01", 6},
        {"a_c_panther_01", 4},
        {"a_c_wolf", 8},
        {"mp_a_c_alligator_01", 3},
        {"mp_a_c_bear_01", 3},
        {"mp_a_c_beaver_01", 2},
        {"mp_a_c_bighornram_01", 4},
        {"mp_a_c_boar_01", 4},
        {"mp_a_c_buck_01", 10},
        {"mp_a_c_buffalo_01", 3},
        {"mp_a_c_cougar_01", 3},
        {"mp_a_c_coyote_01", 3},
        {"mp_a_c_elk_01", 3},
        {"mp_a_c_fox_01", 3},
        {"mp_a_c_wolf_01", 10},
    };

    static const std::map<std::string, int> pedVariations = {
        {"mp_male", 138},
        {"mp_female", 120},
        {"a_c_alligator_01", 4},
        {"a_c_alligator_02", 1},
        {"a_c_alligator_03", 3},
        {"a_c_armadillo_01", 1},
        {"a_c_badger_01", 3},
        {"a_c_bat_01", 3},
        {"a_c_bearblack_01", 4},
        {"a_c_bear_01", 11},
        {"a_c_beaver_01", 4},
        {"a_c_bighornram_01", 19},
        {"a_c_bluejay_01", 2},
        {"a_c_boarlegendary_01", 1},
        {"a_c_boar_01", 4},
        {"a_c_buck_01", 5},
        {"a_c_buffalo_01", 15},
        {"a_c_buffalo_tatanka_01", 1},
        {"a_c_bull_01", 4},
        {"a_c_californiacondor_01", 3},
        {"a_c_cardinal_01", 3},
        {"a_c_carolinaparakeet_01", 1},
        {"a_c_cat_01", 3},
        {"a_c_cedarwaxwing_01", 3},
        {"a_c_chicken_01", 3},
        {"a_c_chipmunk_01", 1},
        {"a_c_cormorant_01", 2},
        {"a_c_cougar_01", 6},
        {"a_c_cow", 22},
        {"a_c_coyote_01", 7},
        {"a_c_crab_01", 3},
        {"a_c_cranewhooping_01", 2},
        {"a_c_crawfish_01", 3},
        {"a_c_crow_01", 1},
        {"a_c_deer_01", 6},
        {"a_c_dogamericanfoxhound_01", 5},
        {"a_c_dogaustraliansheperd_01", 3},
        {"a_c_dogbluetickcoonhound_01", 8},
        {"a_c_dogcatahoulacur_01", 7},
        {"a_c_dogchesbayretriever_01", 3},
        {"a_c_dogcollie_01", 3},
        {"a_c_doghobo_01", 1},
        {"a_c_doghound_01", 8},
        {"a_c_doghusky_01", 3},
        {"a_c_doglab_01", 7},
        {"a_c_doglion_01", 2},
        {"a_c_dogpoodle_01", 3},
        {"a_c_dogrufus_01", 1},
        {"a_c_dogstreet_01", 3},
        {"re_lostdog_dogs_01", 4},
        {"a_c_donkey_01", 8},
        {"a_c_duck_01", 3},
        {"a_c_eagle_01", 4},
        {"a_c_egret_01", 3},
        {"a_c_elk_01", 8},
        {"a_c_fishbluegil_01_ms", 2},
        {"a_c_fishbluegil_01_sm", 2},
        {"a_c_fishbullheadcat_01_ms", 2},
        {"a_c_fishbullheadcat_01_sm", 2},
        {"a_c_fishchainpickerel_01_ms", 2},
        {"a_c_fishchainpickerel_01_sm", 2},
        {"a_c_fishchannelcatfish_01_lg", 3},
        {"a_c_fishchannelcatfish_01_xl", 2},
        {"a_c_fishlakesturgeon_01_lg", 4},
        {"a_c_fishlargemouthbass_01_lg", 2},
        {"a_c_fishlargemouthbass_01_ms", 2},
        {"a_c_fishlongnosegar_01_lg", 6},
        {"a_c_fishmuskie_01_lg", 5},
        {"a_c_fishnorthernpike_01_lg", 7},
        {"a_c_fishperch_01_ms", 2},
        {"a_c_fishperch_01_sm", 1},
        {"a_c_fishrainbowtrout_01_lg", 2},
        {"a_c_fishrainbowtrout_01_ms", 1},
        {"a_c_fishredfinpickerel_01_ms", 2},
        {"a_c_fishredfinpickerel_01_sm", 2},
        {"a_c_fishrockbass_01_ms", 2},
        {"a_c_fishrockbass_01_sm", 4},
        {"a_c_fishsalmonsockeye_01_lg", 2},
        {"a_c_fishsalmonsockeye_01_ml", 1},
        {"a_c_fishsalmonsockeye_01_ms", 2},
        {"a_c_fishsmallmouthbass_01_lg", 2},
        {"a_c_fishsmallmouthbass_01_ms", 2},
        {"a_c_fox_01", 7},
        {"a_c_frogbull_01", 2},
        {"a_c_gilamonster_01", 3},
        {"a_c_goat_01", 3},
        {"a_c_goosecanada_01", 3},
        {"a_c_hawk_01", 3},
        {"a_c_heron_01", 3},
        {"a_c_horsemulepainted_01", 2},
        {"a_c_horsemule_01", 2},
        {"p_c_horse_01", 1},
        {"a_c_horse_americanpaint_greyovero", 1},
        {"a_c_horse_americanpaint_overo", 4},
        {"a_c_horse_americanpaint_splashedwhite", 1},
        {"a_c_horse_americanpaint_tobiano", 3},
        {"a_c_horse_americanstandardbred_black", 1},
        {"a_c_horse_americanstandardbred_buckskin", 2},
        {"a_c_horse_americanstandardbred_lightbuckskin", 5},
        {"a_c_horse_americanstandardbred_palominodapple", 2},
        {"a_c_horse_americanstandardbred_silvertailbuckskin", 1},
        {"a_c_horse_andalusian_darkbay", 1},
        {"a_c_horse_andalusian_perlino", 1},
        {"a_c_horse_andalusian_rosegray", 3},
        {"a_c_horse_appaloosa_blacksnowflake", 2},
        {"a_c_horse_appaloosa_blanket", 1},
        {"a_c_horse_appaloosa_brownleopard", 1},
        {"a_c_horse_appaloosa_fewspotted_pc", 2},
        {"a_c_horse_appaloosa_leopard", 1},
        {"a_c_horse_appaloosa_leopardblanket", 2},
        {"a_c_horse_arabian_black", 1},
        {"a_c_horse_arabian_grey", 1},
        {"a_c_horse_arabian_redchestnut", 1},
        {"a_c_horse_arabian_redchestnut_pc", 1},
        {"a_c_horse_arabian_rosegreybay", 1},
        {"a_c_horse_arabian_warpedbrindle_pc", 1},
        {"a_c_horse_arabian_white", 4},
        {"a_c_horse_ardennes_bayroan", 1},
        {"a_c_horse_ardennes_irongreyroan", 1},
        {"a_c_horse_ardennes_strawberryroan", 1},
        {"a_c_horse_belgian_blondchestnut", 1},
        {"a_c_horse_belgian_mealychestnut", 1},
        {"a_c_horse_breton_grullodun", 1},
        {"a_c_horse_breton_mealydapplebay", 1},
        {"a_c_horse_breton_redroan", 1},
        {"a_c_horse_breton_sealbrown", 1},
        {"a_c_horse_breton_sorrel", 1},
        {"a_c_horse_breton_steelgrey", 1},
        {"a_c_horse_buell_warvets", 4},
        {"a_c_horse_criollo_baybrindle", 1},
        {"a_c_horse_criollo_bayframeovero", 1},
        {"a_c_horse_criollo_blueroanovero", 1},
        {"a_c_horse_criollo_dun", 1},
        {"a_c_horse_criollo_marblesabino", 1},
        {"a_c_horse_criollo_sorrelovero", 1},
        {"a_c_horse_dutchwarmblood_chocolateroan", 1},
        {"a_c_horse_dutchwarmblood_sealbrown", 1},
        {"a_c_horse_dutchwarmblood_sootybuckskin", 1},
        {"a_c_horse_eagleflies", 2},
        {"a_c_horse_gang_bill", 2},
        {"a_c_horse_gang_charles", 4},
        {"a_c_horse_gang_charles_endlesssummer", 2},
        {"a_c_horse_gang_dutch", 2},
        {"a_c_horse_gang_hosea", 2},
        {"a_c_horse_gang_javier", 2},
        {"a_c_horse_gang_john", 2},
        {"a_c_horse_gang_karen", 2},
        {"a_c_horse_gang_kieran", 2},
        {"a_c_horse_gang_lenny", 2},
        {"a_c_horse_gang_micah", 2},
        {"a_c_horse_gang_sadie", 2},
        {"a_c_horse_gang_sadie_endlesssummer", 2},
        {"a_c_horse_gang_sean", 2},
        {"a_c_horse_gang_trelawney", 2},
        {"a_c_horse_gang_uncle", 2},
        {"a_c_horse_gang_uncle_endlesssummer", 2},
        {"a_c_horse_hungarianhalfbred_darkdapplegrey", 1},
        {"a_c_horse_hungarianhalfbred_flaxenchestnut", 1},
        {"a_c_horse_hungarianhalfbred_liverchestnut", 1},
        {"a_c_horse_hungarianhalfbred_piebaldtobiano", 2},
        {"a_c_horse_john_endlesssummer", 2},
        {"a_c_horse_kentuckysaddle_black", 1},
        {"a_c_horse_kentuckysaddle_buttermilkbuckskin_pc", 1},
        {"a_c_horse_kentuckysaddle_chestnutpinto", 1},
        {"a_c_horse_kentuckysaddle_grey", 1},
        {"a_c_horse_kentuckysaddle_silverbay", 1},
        {"a_c_horse_kladruber_black", 1},
        {"a_c_horse_kladruber_cremello", 1},
        {"a_c_horse_kladruber_dapplerosegrey", 1},
        {"a_c_horse_kladruber_grey", 1},
        {"a_c_horse_kladruber_silver", 1},
        {"a_c_horse_kladruber_white", 1},
        {"a_c_horse_missourifoxtrotter_amberchampagne", 1},
        {"a_c_horse_missourifoxtrotter_sablechampagne", 2},
        {"a_c_horse_missourifoxtrotter_silverdapplepinto", 1},
        {"a_c_horse_morgan_bay", 1},
        {"a_c_horse_morgan_bayroan", 1},
        {"a_c_horse_morgan_flaxenchestnut", 1},
        {"a_c_horse_morgan_liverchestnut_pc", 1},
        {"a_c_horse_morgan_palomino", 1},
        {"a_c_horse_mp_mangy_backup", 7},
        {"a_c_horse_murfreebrood_mange_01", 2},
        {"a_c_horse_murfreebrood_mange_02", 2},
        {"a_c_horse_murfreebrood_mange_03", 2},
        {"a_c_horse_mustang_goldendun", 2},
        {"a_c_horse_mustang_grullodun", 1},
        {"a_c_horse_mustang_tigerstripedbay", 1},
        {"a_c_horse_mustang_wildbay", 1},
        {"a_c_horse_nokota_blueroan", 2},
        {"a_c_horse_nokota_reversedappleroan", 1},
        {"a_c_horse_nokota_whiteroan", 2},
        {"a_c_horse_shire_darkbay", 1},
        {"a_c_horse_shire_lightgrey", 1},
        {"a_c_horse_shire_ravenblack", 1},
        {"a_c_horse_suffolkpunch_redchestnut", 1},
        {"a_c_horse_suffolkpunch_sorrel", 1},
        {"a_c_horse_tennesseewalker_blackrabicano", 1},
        {"a_c_horse_tennesseewalker_chestnut", 1},
        {"a_c_horse_tennesseewalker_dapplebay", 1},
        {"a_c_horse_tennesseewalker_flaxenroan", 1},
        {"a_c_horse_tennesseewalker_goldpalomino_pc", 1},
        {"a_c_horse_tennesseewalker_mahoganybay", 1},
        {"a_c_horse_tennesseewalker_redroan", 1},
        {"a_c_horse_thoroughbred_blackchestnut", 1},
        {"a_c_horse_thoroughbred_bloodbay", 1},
        {"a_c_horse_thoroughbred_brindle", 1},
        {"a_c_horse_thoroughbred_dapplegrey", 3},
        {"a_c_horse_thoroughbred_reversedappleblack", 1},
        {"a_c_horse_turkoman_darkbay", 1},
        {"a_c_horse_turkoman_gold", 1},
        {"a_c_horse_turkoman_silver", 1},
        {"a_c_horse_winter02_01", 2},
        {"a_c_iguanadesert_01", 3},
        {"a_c_iguana_01", 1},
        {"a_c_javelina_01", 3},
        {"a_c_lionmangy_01", 2},
        {"a_c_loon_01", 3},
        {"a_c_moose_01", 7},
        {"a_c_muskrat_01", 3},
        {"a_c_oriole_01", 3},
        {"a_c_owl_01", 3},
        {"a_c_ox_01", 2},
        {"a_c_panther_01", 5},
        {"a_c_parrot_01", 3},
        {"a_c_pelican_01", 3},
        {"a_c_pheasant_01", 3},
        {"a_c_pigeon", 3},
        {"a_c_pig_01", 3},
        {"a_c_possum_01", 3},
        {"a_c_prairiechicken_01", 2},
        {"a_c_pronghorn_01", 20},
        {"a_c_quail_01", 3},
        {"a_c_rabbit_01", 5},
        {"a_c_raccoon_01", 7},
        {"a_c_rat_01", 5},
        {"a_c_raven_01", 2},
        {"a_c_redfootedbooby_01", 3},
        {"a_c_robin_01", 1},
        {"a_c_rooster_01", 3},
        {"a_c_roseatespoonbill_01", 3},
        {"a_c_seagull_01", 3},
        {"a_c_sharkhammerhead_01", 3},
        {"a_c_sharktiger", 3},
        {"a_c_sheep_01", 8},
        {"a_c_skunk_01", 4},
        {"a_c_snakeblacktailrattle_01", 2},
        {"a_c_snakeblacktailrattle_pelt_01", 2},
        {"a_c_snakeferdelance_01", 3},
        {"a_c_snakeferdelance_pelt_01", 3},
        {"a_c_snakeredboa10ft_01", 1},
        {"a_c_snakeredboa_01", 3},
        {"a_c_snakeredboa_pelt_01", 3},
        {"a_c_snakewater_01", 3},
        {"a_c_snakewater_pelt_01", 3},
        {"a_c_snake_01", 4},
        {"a_c_snake_pelt_01", 4},
        {"a_c_songbird_01", 2},
        {"a_c_sparrow_01", 3},
        {"a_c_squirrel_01", 6},
        {"a_c_toad_01", 5},
        {"a_c_turkeywild_01", 3},
        {"a_c_turkey_01", 3},
        {"a_c_turkey_02", 3},
        {"a_c_turtlesea_01", 1},
        {"a_c_turtlesnapping_01", 3},
        {"a_c_vulture_01", 4},
        {"A_C_Wolf", 9},
        {"a_c_wolf_medium", 9},
        {"a_c_wolf_small", 5},
        {"a_c_woodpecker_01", 1},
        {"a_c_woodpecker_02", 1},
        {"amsp_robsdgunsmith_males_01", 5},
        {"am_valentinedoctors_females_01", 1},
        {"a_f_m_armcholeracorpse_01", 40},
        {"a_f_m_armtownfolk_01", 34},
        {"a_f_m_armtownfolk_02", 24},
        {"a_f_m_asbtownfolk_01", 35},
        {"a_f_m_bivfancytravellers_01", 40},
        {"a_f_m_blwtownfolk_01", 40},
        {"a_f_m_blwtownfolk_02", 40},
        {"a_f_m_blwupperclass_01", 50},
        {"a_f_m_btchillbilly_01", 17},
        {"a_f_m_btcobesewomen_01", 1},
        {"a_f_m_bynfancytravellers_01", 30},
        {"a_f_m_familytravelers_cool_01", 20},
        {"a_f_m_familytravelers_warm_01", 20},
        {"a_f_m_gamhighsociety_01", 39},
        {"a_f_m_grifancytravellers_01", 40},
        {"a_f_m_guatownfolk_01", 25},
        {"a_f_m_htlfancytravellers_01", 40},
        {"a_f_m_lagtownfolk_01", 12},
        {"a_f_m_lowersdtownfolk_01", 42},
        {"a_f_m_lowersdtownfolk_02", 40},
        {"a_f_m_lowersdtownfolk_03", 42},
        {"a_f_m_lowertrainpassengers_01", 25},
        {"a_f_m_middlesdtownfolk_01", 45},
        {"a_f_m_middlesdtownfolk_02", 30},
        {"a_f_m_middlesdtownfolk_03", 20},
        {"a_f_m_middletrainpassengers_01", 25},
        {"a_f_m_nbxslums_01", 42},
        {"a_f_m_nbxupperclass_01", 45},
        {"a_f_m_nbxwhore_01", 25},
        {"a_f_m_rhdprostitute_01", 35},
        {"a_f_m_rhdtownfolk_01", 23},
        {"a_f_m_rhdtownfolk_02", 22},
        {"a_f_m_rhdupperclass_01", 50},
        {"a_f_m_rkrfancytravellers_01", 40},
        {"a_f_m_roughtravellers_01", 30},
        {"a_f_m_sclfancytravellers_01", 21},
        {"a_f_m_sdchinatown_01", 20},
        {"a_f_m_sdfancywhore_01", 20},
        {"a_f_m_sdobesewomen_01", 1},
        {"a_f_m_sdserversformal_01", 10},
        {"a_f_m_sdslums_02", 42},
        {"a_f_m_skpprisononline_01", 8},
        {"a_f_m_strtownfolk_01", 20},
        {"a_f_m_tumtownfolk_01", 22},
        {"a_f_m_tumtownfolk_02", 22},
        {"a_f_m_unicorpse_01", 49},
        {"a_f_m_uppertrainpassengers_01", 25},
        {"a_f_m_valprostitute_01", 23},
        {"a_f_m_valtownfolk_01", 20},
        {"a_f_m_vhtprostitute_01", 11},
        {"a_f_m_vhttownfolk_01", 25},
        {"a_f_m_waptownfolk_01", 25},
        {"a_f_o_blwupperclass_01", 45},
        {"a_f_o_btchillbilly_01", 11},
        {"a_f_o_guatownfolk_01", 15},
        {"a_f_o_lagtownfolk_01", 12},
        {"a_f_o_sdchinatown_01", 20},
        {"a_f_o_sdupperclass_01", 45},
        {"a_f_o_waptownfolk_01", 21},
        {"a_m_m_armcholeracorpse_01", 38},
        {"a_m_m_armdeputyresident_01", 56},
        {"a_m_m_armtownfolk_01", 49},
        {"a_m_m_armtownfolk_02", 60},
        {"a_m_m_asbboatcrew_01", 30},
        {"a_m_m_asbdeputyresident_01", 21},
        {"a_m_m_asbminer_01", 61},
        {"a_m_m_asbminer_02", 42},
        {"a_m_m_asbminer_03", 81},
        {"a_m_m_asbminer_04", 69},
        {"a_m_m_asbtownfolk_01", 86},
        {"a_m_m_asbtownfolk_01_laborer", 70},
        {"a_m_m_bivfancydrivers_01", 10},
        {"a_m_m_bivfancytravellers_01", 21},
        {"a_m_m_bivroughtravellers_01", 41},
        {"a_m_m_bivworker_01", 29},
        {"a_m_m_blwforeman_01", 35},
        {"a_m_m_blwlaborer_01", 35},
        {"a_m_m_blwlaborer_02", 45},
        {"a_m_m_blwobesemen_01", 3},
        {"a_m_m_blwtownfolk_01", 77},
        {"a_m_m_blwupperclass_01", 85},
        {"a_m_m_btchillbilly_01", 42},
        {"a_m_m_btcobesemen_01", 1},
        {"a_m_m_bynfancydrivers_01", 10},
        {"a_m_m_bynfancytravellers_01", 20},
        {"a_m_m_bynroughtravellers_01", 40},
        {"a_m_m_bynsurvivalist_01", 15},
        {"a_m_m_cardgameplayers_01", 84},
        {"a_m_m_chelonian_01", 24},
        {"a_m_m_deliverytravelers_cool_01", 20},
        {"a_m_m_deliverytravelers_warm_01", 20},
        {"a_m_m_dominoesplayers_01", 18},
        {"a_m_m_emrfarmhand_01", 20},
        {"a_m_m_familytravelers_cool_01", 20},
        {"a_m_m_familytravelers_warm_01", 20},
        {"a_m_m_farmtravelers_cool_01", 20},
        {"a_m_m_farmtravelers_warm_01", 20},
        {"a_m_m_fivefingerfilletplayers_01", 12},
        {"a_m_m_foreman", 10},
        {"a_m_m_gamhighsociety_01", 40},
        {"a_m_m_grifancydrivers_01", 10},
        {"a_m_m_grifancytravellers_01", 20},
        {"a_m_m_griroughtravellers_01", 40},
        {"a_m_m_grisurvivalist_01", 15},
        {"a_m_m_guatownfolk_01", 27},
        {"a_m_m_htlfancydrivers_01", 10},
        {"a_m_m_htlfancytravellers_01", 20},
        {"a_m_m_htlroughtravellers_01", 40},
        {"a_m_m_htlsurvivalist_01", 15},
        {"a_m_m_huntertravelers_cool_01", 20},
        {"a_m_m_huntertravelers_warm_01", 21},
        {"a_m_m_jamesonguard_01", 56},
        {"a_m_m_lagtownfolk_01", 12},
        {"a_m_m_lowersdtownfolk_01", 91},
        {"a_m_m_lowersdtownfolk_02", 91},
        {"a_m_m_lowertrainpassengers_01", 25},
        {"a_m_m_middlesdtownfolk_01", 65},
        {"a_m_m_middlesdtownfolk_02", 51},
        {"a_m_m_middlesdtownfolk_03", 51},
        {"a_m_m_middletrainpassengers_01", 25},
        {"a_m_m_moonshiners_01", 10},
        {"a_m_m_nbxdockworkers_01", 70},
        {"a_m_m_nbxlaborers_01", 70},
        {"a_m_m_nbxslums_01", 85},
        {"a_m_m_nbxupperclass_01", 50},
        {"a_m_m_nearoughtravellers_01", 35},
        {"a_m_m_ranchertravelers_cool_01", 20},
        {"a_m_m_ranchertravelers_warm_01", 20},
        {"a_m_m_rancher_01", 20},
        {"a_m_m_rhddeputyresident_01", 23},
        {"a_m_m_rhdforeman_01", 30},
        {"a_m_m_rhdobesemen_01", 3},
        {"a_m_m_rhdtownfolk_01", 42},
        {"a_m_m_rhdtownfolk_01_laborer", 10},
        {"a_m_m_rhdtownfolk_02", 35},
        {"a_m_m_rhdupperclass_01", 63},
        {"a_m_m_rkrfancydrivers_01", 10},
        {"a_m_m_rkrfancytravellers_01", 20},
        {"a_m_m_rkrroughtravellers_01", 40},
        {"a_m_m_rkrsurvivalist_01", 15},
        {"a_m_m_sclfancydrivers_01", 10},
        {"a_m_m_sclfancytravellers_01", 20},
        {"a_m_m_sclroughtravellers_01", 41},
        {"a_m_m_sdchinatown_01", 21},
        {"a_m_m_sddockforeman_01", 25},
        {"a_m_m_sddockworkers_02", 70},
        {"a_m_m_sdfancytravellers_01", 20},
        {"a_m_m_sdlaborers_02", 70},
        {"a_m_m_sdobesemen_01", 3},
        {"a_m_m_sdroughtravellers_01", 40},
        {"a_m_m_sdserversformal_01", 6},
        {"a_m_m_sdslums_02", 85},
        {"a_m_m_skpprisoner_01", 20},
        {"a_m_m_skpprisonline_01", 24},
        {"a_m_m_smhthug_01", 24},
        {"a_m_m_strdeputyresident_01", 21},
        {"a_m_m_strfancytourist_01", 5},
        {"a_m_m_strlaborer_01", 18},
        {"a_m_m_strtownfolk_01", 26},
        {"a_m_m_tumtownfolk_01", 60},
        {"a_m_m_tumtownfolk_02", 60},
        {"a_m_m_uniboatcrew_01", 20},
        {"a_m_m_unicoachguards_01", 20},
        {"a_m_m_unicorpse_01", 186},
        {"a_m_m_unigunslinger_01", 40},
        {"a_m_m_uppertrainpassengers_01", 25},
        {"a_m_m_valcriminals_01", 10},
        {"a_m_m_valdeputyresident_01", 30},
        {"a_m_m_valfarmer_01", 29},
        {"a_m_m_vallaborer_01", 58},
        {"a_m_m_valtownfolk_01", 36},
        {"a_m_m_valtownfolk_02", 44},
        {"a_m_m_vhtboatcrew_01", 30},
        {"a_m_m_vhtthug_01", 40},
        {"a_m_m_vhttownfolk_01", 86},
        {"a_m_m_wapwarriors_01", 69},
        {"a_m_o_blwupperclass_01", 80},
        {"a_m_o_btchillbilly_01", 29},
        {"a_m_o_guatownfolk_01", 15},
        {"a_m_o_lagtownfolk_01", 12},
        {"a_m_o_sdchinatown_01", 21},
        {"a_m_o_sdupperclass_01", 50},
        {"a_m_o_waptownfolk_01", 26},
        {"a_m_y_asbminer_01", 60},
        {"a_m_y_asbminer_02", 42},
        {"a_m_y_asbminer_03", 81},
        {"a_m_y_asbminer_04", 69},
        {"a_m_y_nbxstreetkids_01", 8},
        {"a_m_y_nbxstreetkids_slums_01", 20},
        {"a_m_y_sdstreetkids_slums_02", 8},
        {"a_m_y_unicorpse_01", 2},
        {"casp_coachrobbery_lenny_males_01", 2},
        {"casp_coachrobbery_micah_males_01", 1},
        {"casp_hunting02_males_01", 2},
        {"cr_strawberry_males_01", 19},
        {"cs_abe", 2},
        {"cs_aberdeenpigfarmer", 1},
        {"cs_aberdeensister", 2},
        {"cs_abigailroberts", 15},
        {"cs_acrobat", 1},
        {"cs_adamgray", 1},
        {"cs_agnesdowd", 4},
        {"cs_albertcakeesquire", 1},
        {"cs_albertmason", 2},
        {"cs_andershelgerson", 4},
        {"cs_angel", 1},
        {"cs_angryhusband", 1},
        {"cs_angusgeddes", 2},
        {"cs_ansel_atherton", 1},
        {"cs_antonyforemen", 2},
        {"cs_archerfordham", 2},
        {"cs_archibaldjameson", 2},
        {"cs_archiedown", 4},
        {"cs_artappraiser", 1},
        {"cs_asbdeputy_01", 1},
        {"cs_ashton", 1},
        {"cs_balloonoperator", 4},
        {"cs_bandbassist", 1},
        {"cs_banddrummer", 1},
        {"cs_bandpianist", 1},
        {"cs_bandsinger", 1},
        {"cs_baptiste", 1},
        {"cs_bartholomewbraithwaite", 2},
        {"cs_bathingladies_01", 7},
        {"cs_beatenupcaptain", 1},
        {"cs_beaugray", 3},
        {"cs_billwilliamson", 28},
        {"cs_bivcoachdriver", 1},
        {"cs_blwphotographer", 1},
        {"cs_blwwitness", 2},
        {"cs_braithwaitebutler", 1},
        {"cs_braithwaitemaid", 1},
        {"cs_braithwaiteservant", 1},
        {"cs_brendacrawley", 2},
        {"cs_bronte", 4},
        {"cs_brontesbutler", 1},
        {"cs_brotherdorkins", 1},
        {"cs_brynntildon", 2},
        {"cs_bubba", 2},
        {"cs_cabaretmc", 1},
        {"cs_cajun", 1},
        {"cs_cancanman_01", 1},
        {"cs_cancan_01", 1},
        {"cs_cancan_02", 1},
        {"cs_cancan_03", 1},
        {"cs_cancan_04", 1},
        {"cs_captainmonroe", 1},
        {"cs_cassidy", 3},
        {"cs_catherinebraithwaite", 2},
        {"cs_cattlerustler", 1},
        {"cs_cavehermit", 1},
        {"cs_chainprisoner_01", 4},
        {"cs_chainprisoner_02", 4},
        {"cs_charlessmith", 34},
        {"cs_chelonianmaster", 5},
        {"cs_cigcardguy", 1},
        {"cs_clay", 1},
        {"cs_cleet", 3},
        {"cs_clive", 1},
        {"cs_colfavours", 1},
        {"cs_colmodriscoll", 6},
        {"cs_cooper", 1},
        {"cs_cornwalltrainconductor", 1},
        {"cs_crackpotinventor", 5},
        {"cs_crackpotrobot", 2},
        {"cs_creepyoldlady", 1},
        {"cs_creolecaptain", 1},
        {"cs_creoledoctor", 1},
        {"cs_creoleguy", 1},
        {"cs_dalemaroney", 2},
        {"cs_daveycallender", 1},
        {"cs_davidgeddes", 2},
        {"cs_desmond", 1},
        {"cs_didsbury", 1},
        {"cs_dinoboneslady", 2},
        {"cs_disguisedduster_01", 1},
        {"cs_disguisedduster_02", 1},
        {"cs_disguisedduster_03", 1},
        {"cs_doroetheawicklow", 3},
        {"cs_drhiggins", 1},
        {"cs_drmalcolmmacintosh", 1},
        {"cs_duncangeddes", 1},
        {"cs_dusterinformant_01", 1},
        {"cs_dutch", 31},
        {"cs_eagleflies", 10},
        {"cs_edgarross", 2},
        {"cs_edithdown", 7},
        {"cs_edith_john", 1},
        {"cs_edmundlowry", 1},
        {"cs_escapeartist", 5},
        {"cs_escapeartistassistant", 1},
        {"cs_evelynmiller", 3},
        {"cs_exconfedinformant", 1},
        {"cs_exconfedsleader_01", 1},
        {"cs_exoticcollector", 2},
        {"cs_famousgunslinger_01", 2},
        {"cs_famousgunslinger_02", 1},
        {"cs_famousgunslinger_03", 1},
        {"cs_famousgunslinger_04", 1},
        {"cs_famousgunslinger_05", 1},
        {"cs_famousgunslinger_06", 3},
        {"cs_featherstonchambers", 1},
        {"cs_featsofstrength", 2},
        {"cs_fightref", 1},
        {"cs_fire_breather", 4},
        {"cs_fishcollector", 4},
        {"cs_forgivenhusband_01", 1},
        {"cs_forgivenwife_01", 1},
        {"cs_formyartbigwoman", 1},
        {"cs_francis_sinclair", 1},
        {"cs_frenchartist", 2},
        {"cs_frenchman_01", 1},
        {"cs_fussar", 2},
        {"cs_garethbraithwaite", 1},
        {"cs_gavin", 2},
        {"cs_genstoryfemale", 2},
        {"cs_genstorymale", 1},
        {"cs_geraldbraithwaite", 2},
        {"cs_germandaughter", 1},
        {"cs_germanfather", 5},
        {"cs_germanmother", 3},
        {"cs_germanson", 2},
        {"cs_gilbertknightly", 1},
        {"cs_gloria", 1},
        {"cs_grizzledjon", 2},
        {"cs_guidomartelli", 3},
        {"cs_hamish", 1},
        {"cs_hectorfellowes", 3},
        {"cs_henrilemiux", 2},
        {"cs_herbalist", 1},
        {"cs_hercule", 2},
        {"cs_hestonjameson", 2},
        {"cs_hobartcrawley", 2},
        {"cs_hoseamatthews", 13},
        {"cs_iangray", 1},
        {"cs_jackmarston", 8},
        {"cs_jackmarston_teen", 10},
        {"cs_jamie", 1},
        {"cs_janson", 1},
        {"cs_javierescuella", 32},
        {"cs_jeb", 3},
        {"cs_jimcalloway", 1},
        {"cs_jockgray", 1},
        {"cs_joe", 2},
        {"cs_joebutler", 2},
        {"cs_johnmarston", 31},
        {"cs_johnthebaptisingmadman", 2},
        {"cs_johnweathers", 1},
        {"cs_josiahtrelawny", 10},
        {"cs_jules", 2},
        {"cs_karen", 16},
        {"cs_karensjohn_01", 2},
        {"cs_kieran", 11},
        {"cs_laramie", 2},
        {"cs_leighgray", 1},
        {"cs_lemiuxassistant", 2},
        {"cs_lenny", 17},
        {"cs_leon", 3},
        {"cs_leostrauss", 11},
        {"cs_levisimon", 1},
        {"cs_leviticuscornwall", 1},
        {"cs_lillianpowell", 3},
        {"cs_lillymillet", 1},
        {"cs_londonderryson", 1},
        {"cs_lucanapoli", 1},
        {"cs_magnifico", 2},
        {"cs_mamawatson", 1},
        {"cs_marshall_thurwell", 1},
        {"cs_marybeth", 8},
        {"cs_marylinton", 5},
        {"cs_meditatingmonk", 1},
        {"cs_meredith", 1},
        {"cs_meredithsmother", 1},
        {"cs_micahbell", 32},
        {"cs_micahsnemesis", 1},
        {"cs_mickey", 2},
        {"cs_miltonandrews", 2},
        {"cs_missmarjorie", 3},
        {"cs_mixedracekid", 2},
        {"cs_moira", 1},
        {"cs_mollyoshea", 8},
        {"cs_mp_alfredo_montez", 4},
        {"cs_mp_allison", 1},
        {"cs_mp_amos_lansing", 1},
        {"cs_mp_bonnie", 1},
        {"cs_mp_bountyhunter", 1},
        {"cs_mp_camp_cook", 3},
        {"cs_mp_cliff", 1},
        {"cs_mp_cripps", 25},
        {"cs_mp_grace_lancing", 1},
        {"cs_mp_hans", 1},
        {"cs_mp_henchman", 3},
        {"cs_mp_horley", 1},
        {"cs_mp_jeremiah_shaw", 1},
        {"cs_mp_jessica", 3},
        {"cs_mp_jorge_montez", 2},
        {"cs_mp_langston", 1},
        {"cs_mp_lee", 1},
        {"cs_mp_mabel", 1},
        {"cs_mp_marshall_davies", 2},
        {"cs_mp_moonshiner", 1},
        {"cs_mp_mradler", 1},
        {"cs_mp_oldman_jones", 2},
        {"cs_mp_revenge_marshall", 1},
        {"cs_mp_samson_finch", 3},
        {"cs_mp_shaky", 1},
        {"cs_mp_sherifffreeman", 1},
        {"cs_mp_teddybrown", 2},
        {"cs_mp_terrance", 1},
        {"cs_mp_the_boy", 2},
        {"cs_mp_travellingsaleswoman", 1},
        {"cs_mp_went", 3},
        {"cs_mradler", 3},
        {"cs_mrdevon", 1},
        {"cs_mrlinton", 1},
        {"cs_mrpearson", 9},
        {"cs_mrsadler", 22},
        {"cs_mrsfellows", 1},
        {"cs_mrsgeddes", 1},
        {"cs_mrslondonderry", 1},
        {"cs_mrsweathers", 1},
        {"cs_mrs_calhoun", 1},
        {"cs_mrs_sinclair", 1},
        {"cs_mrwayne", 1},
        {"cs_mud2bigguy", 6},
        {"cs_mysteriousstranger", 1},
        {"cs_nbxdrunk", 2},
        {"cs_nbxexecuted", 2},
        {"cs_nbxpolicechiefformal", 1},
        {"cs_nbxreceptionist_01", 1},
        {"cs_nial_whelan", 1},
        {"cs_nicholastimmins", 2},
        {"cs_nils", 1},
        {"cs_norrisforsythe", 2},
        {"cs_obediahhinton", 1},
        {"cs_oddfellowspinhead", 2},
        {"cs_odprostitute", 1},
        {"cs_operasinger", 1},
        {"cs_paytah", 4},
        {"cs_penelopebraithwaite", 2},
        {"cs_pinkertongoon", 1},
        {"cs_poisonwellshaman", 2},
        {"cs_poorjoe", 1},
        {"cs_priest_wedding", 1},
        {"cs_princessisabeau", 1},
        {"cs_professorbell", 1},
        {"cs_rainsfall", 5},
        {"cs_ramon_cortez", 1},
        {"cs_reverendfortheringham", 2},
        {"cs_revswanson", 7},
        {"cs_rhodeputy_01", 1},
        {"cs_rhodeputy_02", 1},
        {"cs_rhodesassistant", 1},
        {"cs_rhodeskidnapvictim", 1},
        {"cs_rhodessaloonbouncer", 1},
        {"cs_ringmaster", 1},
        {"cs_rockyseven_widow", 5},
        {"cs_samaritan", 1},
        {"cs_scottgray", 1},
        {"cs_sddoctor_01", 1},
        {"cs_sdpriest", 1},
        {"cs_sdsaloondrunk_01", 1},
        {"cs_sdstreetkidthief", 1},
        {"cs_sd_streetkid_01", 1},
        {"cs_sd_streetkid_01a", 1},
        {"cs_sd_streetkid_01b", 1},
        {"cs_sd_streetkid_02", 1},
        {"cs_sean", 17},
        {"cs_sherifffreeman", 2},
        {"cs_sheriffowens", 2},
        {"cs_sistercalderon", 1},
        {"cs_slavecatcher", 1},
        {"cs_soothsayer", 2},
        {"cs_strawberryoutlaw_01", 1},
        {"cs_strawberryoutlaw_02", 1},
        {"cs_strdeputy_01", 1},
        {"cs_strdeputy_02", 1},
        {"cs_strsheriff_01", 1},
        {"cs_sunworshipper", 1},
        {"cs_susangrimshaw", 12},
        {"cs_swampfreak", 1},
        {"cs_swampweirdosonny", 3},
        {"cs_sworddancer", 1},
        {"cs_tavishgray", 1},
        {"cs_taxidermist", 1},
        {"cs_theodorelevin", 2},
        {"cs_thomasdown", 2},
        {"cs_tigerhandler", 1},
        {"cs_tilly", 12},
        {"cs_timothydonahue", 2},
        {"cs_tinyhermit", 2},
        {"cs_tomdickens", 3},
        {"cs_towncrier", 2},
        {"cs_treasurehunter", 1},
        {"cs_twinbrother_01", 2},
        {"cs_twinbrother_02", 2},
        {"cs_twingroupie_01", 1},
        {"cs_twingroupie_02", 1},
        {"cs_uncle", 12},
        {"cs_unidusterjail_01", 1},
        {"cs_valauctionboss_01", 2},
        {"cs_valdeputy_01", 1},
        {"cs_valprayingman", 1},
        {"cs_valprostitute_01", 1},
        {"cs_valprostitute_02", 1},
        {"cs_valsheriff", 2},
        {"cs_vampire", 1},
        {"cs_vht_bathgirl", 1},
        {"cs_wapitiboy", 1},
        {"cs_warvet", 3},
        {"cs_watson_01", 1},
        {"cs_watson_02", 1},
        {"cs_watson_03", 1},
        {"cs_welshfighter", 1},
        {"cs_wintonholmes", 3},
        {"cs_wrobel", 1},
        {"female_skeleton", 7},
        {"gc_lemoynecaptive_males_01", 1},
        {"gc_skinnertorture_males_01", 2},
        {"ge_delloboparty_females_01", 2},
        {"g_f_m_uniduster_01", 3},
        {"g_m_m_bountyhunters_01", 65},
        {"g_m_m_uniafricanamericangang_01", 42},
        {"g_m_m_unibanditos_01", 167},
        {"g_m_m_unibraithwaites_01", 50},
        {"g_m_m_unibrontegoons_01", 83},
        {"g_m_m_unicornwallgoons_01", 86},
        {"g_m_m_unicriminals_01", 150},
        {"g_m_m_unicriminals_02", 60},
        {"g_m_m_uniduster_01", 184},
        {"g_m_m_uniduster_02", 56},
        {"g_m_m_uniduster_03", 37},
        {"g_m_m_uniduster_04", 9},
        {"g_m_m_uniduster_05", 26},
        {"g_m_m_unigrays_01", 51},
        {"g_m_m_unigrays_02", 30},
        {"g_m_m_uniinbred_01", 131},
        {"g_m_m_unilangstonboys_01", 35},
        {"g_m_m_unimicahgoons_01", 31},
        {"g_m_m_unimountainmen_01", 129},
        {"g_m_m_uniranchers_01", 87},
        {"g_m_m_uniswamp_01", 43},
        {"g_m_o_uniexconfeds_01", 93},
        {"g_m_y_uniexconfeds_01", 124},
        {"g_m_y_uniexconfeds_02", 33},
        {"loansharking_asbminer_males_01", 1},
        {"loansharking_horsechase1_males_01", 2},
        {"loansharking_undertaker_females_01", 4},
        {"loansharking_undertaker_males_01", 3},
        {"male_skeleton", 4},
        {"mbh_rhodesrancher_females_01", 1},
        {"mbh_rhodesrancher_teens_01", 1},
        {"mbh_skinnersearch_males_01", 2},
        {"mes_abigail2_males_01", 4},
        {"mes_finale2_females_01", 1},
        {"mes_finale2_males_01", 4},
        {"mes_finale3_males_01", 3},
        {"mes_marston1_males_01", 2},
        {"mes_marston2_males_01", 4},
        {"mes_marston5_2_males_01", 1},
        {"mes_marston6_females_01", 1},
        {"mes_marston6_males_01", 29},
        {"mes_marston6_teens_01", 1},
        {"mes_sadie4_males_01", 1},
        {"mes_sadie5_males_01", 7},
        {"mp_asntrk_elysianpool_males_01", 6},
        {"mp_asntrk_grizzlieswest_males_01", 6},
        {"mp_asntrk_hagenorchard_males_01", 6},
        {"mp_asntrk_isabella_males_01", 6},
        {"mp_asntrk_talltrees_males_01", 6},
        {"mp_asn_benedictpoint_females_01", 1},
        {"mp_asn_benedictpoint_males_01", 6},
        {"mp_asn_blackwater_males_01", 6},
        {"mp_asn_braithwaitemanor_males_01", 2},
        {"mp_asn_braithwaitemanor_males_02", 4},
        {"mp_asn_braithwaitemanor_males_03", 5},
        {"mp_asn_civilwarfort_males_01", 6},
        {"mp_asn_gaptoothbreach_males_01", 6},
        {"mp_asn_pikesbasin_males_01", 5},
        {"mp_asn_sdpolicestation_males_01", 6},
        {"mp_asn_sdwedding_females_01", 3},
        {"mp_asn_sdwedding_males_01", 5},
        {"mp_asn_shadybelle_females_01", 1},
        {"mp_asn_stillwater_males_01", 6},
        {"MP_A_C_HORSECORPSE_01", 16},
        {"mp_a_f_m_cardgameplayers_01", 40},
        {"MP_A_F_M_UniCorpse_01", 44},
        {"mp_a_m_m_laboruprisers_01", 53},
        {"MP_A_M_M_UniCorpse_01", 142},
        {"mp_campdef_bluewater_females_01", 1},
        {"mp_campdef_bluewater_males_01", 1},
        {"mp_campdef_chollasprings_females_01", 1},
        {"mp_campdef_chollasprings_males_01", 2},
        {"mp_campdef_eastnewhanover_females_01", 1},
        {"mp_campdef_eastnewhanover_males_01", 1},
        {"mp_campdef_gaptoothbreach_females_01", 1},
        {"mp_campdef_gaptoothbreach_males_01", 3},
        {"mp_campdef_gaptoothridge_females_01", 1},
        {"mp_campdef_gaptoothridge_males_01", 1},
        {"mp_campdef_greatplains_males_01", 2},
        {"mp_campdef_grizzlies_males_01", 2},
        {"mp_campdef_heartlands1_males_01", 2},
        {"mp_campdef_heartlands2_females_01", 1},
        {"mp_campdef_heartlands2_males_01", 2},
        {"mp_campdef_hennigans_females_01", 1},
        {"mp_campdef_hennigans_males_01", 1},
        {"mp_campdef_littlecreek_females_01", 1},
        {"mp_campdef_littlecreek_males_01", 1},
        {"mp_campdef_radleyspasture_females_01", 1},
        {"mp_campdef_radleyspasture_males_01", 1},
        {"mp_campdef_riobravo_females_01", 1},
        {"mp_campdef_riobravo_males_01", 1},
        {"mp_campdef_roanoke_females_01", 1},
        {"mp_campdef_roanoke_males_01", 1},
        {"mp_campdef_talltrees_females_01", 1},
        {"mp_campdef_talltrees_males_01", 1},
        {"mp_campdef_tworocks_females_01", 2},
        {"mp_chu_kid_armadillo_males_01", 4},
        {"mp_chu_kid_diabloridge_males_01", 4},
        {"mp_chu_kid_emrstation_males_01", 4},
        {"mp_chu_kid_greatplains2_males_01", 4},
        {"mp_chu_kid_greatplains_males_01", 4},
        {"mp_chu_kid_heartlands_males_01", 4},
        {"mp_chu_kid_lagras_males_01", 4},
        {"mp_chu_kid_lemoyne_females_01", 2},
        {"mp_chu_kid_lemoyne_males_01", 2},
        {"mp_chu_kid_recipient_males_01", 22},
        {"mp_chu_kid_rhodes_males_01", 4},
        {"mp_chu_kid_saintdenis_females_01", 1},
        {"mp_chu_kid_saintdenis_males_01", 3},
        {"mp_chu_kid_scarlettmeadows_males_01", 4},
        {"mp_chu_kid_tumbleweed_males_01", 4},
        {"mp_chu_kid_valentine_males_01", 4},
        {"mp_chu_rob_ambarino_males_01", 3},
        {"mp_chu_rob_annesburg_males_01", 4},
        {"mp_chu_rob_benedictpoint_females_01", 2},
        {"mp_chu_rob_benedictpoint_males_01", 2},
        {"mp_chu_rob_blackwater_males_01", 4},
        {"mp_chu_rob_caligahall_males_01", 4},
        {"mp_chu_rob_coronado_males_01", 4},
        {"mp_chu_rob_cumberland_males_01", 4},
        {"mp_chu_rob_fortmercer_females_01", 2},
        {"mp_chu_rob_fortmercer_males_01", 2},
        {"mp_chu_rob_greenhollow_males_01", 4},
        {"mp_chu_rob_macfarlanes_females_01", 2},
        {"mp_chu_rob_macfarlanes_males_01", 2},
        {"mp_chu_rob_macleans_males_01", 4},
        {"mp_chu_rob_millesani_males_01", 4},
        {"mp_chu_rob_montanariver_males_01", 4},
        {"mp_chu_rob_paintedsky_males_01", 4},
        {"mp_chu_rob_rathskeller_males_01", 4},
        {"mp_chu_rob_recipient_males_01", 33},
        {"mp_chu_rob_rhodes_males_01", 4},
        {"mp_chu_rob_strawberry_males_01", 4},
        {"mp_clay", 1},
        {"mp_convoy_recipient_females_01", 2},
        {"mp_convoy_recipient_males_01", 19},
        {"mp_de_u_f_m_bigvalley_01", 1},
        {"mp_de_u_f_m_bluewatermarsh_01", 1},
        {"mp_de_u_f_m_braithwaite_01", 2},
        {"mp_de_u_f_m_doverhill_01", 1},
        {"mp_de_u_f_m_greatplains_01", 1},
        {"mp_de_u_f_m_hangingrock_01", 1},
        {"mp_de_u_f_m_heartlands_01", 1},
        {"mp_de_u_f_m_hennigansstead_01", 2},
        {"mp_de_u_f_m_silentstead_01", 1},
        {"mp_de_u_m_m_aurorabasin_01", 1},
        {"mp_de_u_m_m_barrowlagoon_01", 1},
        {"mp_de_u_m_m_bigvalleygraves_01", 1},
        {"mp_de_u_m_m_centralunionrr_01", 1},
        {"mp_de_u_m_m_pleasance_01", 1},
        {"mp_de_u_m_m_rileyscharge_01", 1},
        {"mp_de_u_m_m_vanhorn_01", 1},
        {"mp_de_u_m_m_westernhomestead_01", 1},
        {"mp_dr_u_f_m_bayougatorfood_01", 1},
        {"mp_dr_u_f_m_bigvalleycave_01", 1},
        {"mp_dr_u_f_m_bigvalleycliff_01", 1},
        {"mp_dr_u_f_m_bluewaterkidnap_01", 1},
        {"mp_dr_u_f_m_colterbandits_01", 1},
        {"mp_dr_u_f_m_colterbandits_02", 1},
        {"mp_dr_u_f_m_missingfisherman_01", 1},
        {"mp_dr_u_f_m_missingfisherman_02", 1},
        {"mp_dr_u_f_m_mistakenbounties_01", 1},
        {"mp_dr_u_f_m_plaguetown_01", 1},
        {"mp_dr_u_f_m_quakerscove_01", 1},
        {"mp_dr_u_f_m_quakerscove_02", 1},
        {"mp_dr_u_f_m_sdgraveyard_01", 1},
        {"mp_dr_u_m_m_bigvalleycave_01", 1},
        {"mp_dr_u_m_m_bigvalleycliff_01", 1},
        {"mp_dr_u_m_m_bluewaterkidnap_01", 1},
        {"mp_dr_u_m_m_canoeescape_01", 1},
        {"mp_dr_u_m_m_hwyrobbery_01", 1},
        {"mp_dr_u_m_m_mistakenbounties_01", 1},
        {"mp_dr_u_m_m_pikesbasin_01", 1},
        {"mp_dr_u_m_m_pikesbasin_02", 1},
        {"mp_dr_u_m_m_plaguetown_01", 1},
        {"mp_dr_u_m_m_roanokestandoff_01", 1},
        {"mp_dr_u_m_m_sdgraveyard_01", 1},
        {"mp_dr_u_m_m_sdmugging_01", 1},
        {"mp_dr_u_m_m_sdmugging_02", 1},
        {"mp_freeroam_tut_females_01", 5},
        {"mp_freeroam_tut_males_01", 8},
        {"mp_gunvoutd2_males_01", 11},
        {"mp_gunvoutd3_bht_01", 2},
        {"mp_gunvoutd3_males_01", 3},
        {"mp_g_f_m_laperlegang_01", 26},
        {"mp_g_f_m_laperlevips_01", 13},
        {"mp_g_f_m_owlhootfamily_01", 17},
        {"mp_g_m_m_armoredjuggernauts_01", 12},
        {"mp_g_m_m_bountyhunters_01", 50},
        {"mp_g_m_m_owlhootfamily_01", 25},
        {"mp_g_m_m_redbengang_01", 38},
        {"mp_g_m_m_uniafricanamericangang_01", 42},
        {"mp_g_m_m_unibanditos_01", 141},
        {"mp_g_m_m_unibraithwaites_01", 40},
        {"mp_g_m_m_unibrontegoons_01", 55},
        {"mp_g_m_m_unicornwallgoons_01", 60},
        {"mp_g_m_m_unicriminals_01", 88},
        {"mp_g_m_m_unicriminals_02", 67},
        {"mp_g_m_m_uniduster_01", 111},
        {"mp_g_m_m_uniduster_02", 53},
        {"mp_g_m_m_uniduster_03", 30},
        {"mp_g_m_m_unigrays_01", 45},
        {"mp_g_m_m_uniinbred_01", 67},
        {"mp_g_m_m_unilangstonboys_01", 35},
        {"mp_g_m_m_unimountainmen_01", 80},
        {"mp_g_m_m_uniranchers_01", 61},
        {"mp_g_m_m_uniswamp_01", 30},
        {"mp_g_m_o_uniexconfeds_01", 64},
        {"mp_g_m_y_uniexconfeds_01", 63},
        {"mp_horse_owlhootvictim_01", 2},
        {"mp_intercept_recipient_females_01", 8},
        {"mp_intercept_recipient_males_01", 29},
        {"mp_intro_females_01", 7},
        {"mp_intro_males_01", 15},
        {"mp_jailbreak_males_01", 4},
        {"mp_jailbreak_recipient_males_01", 6},
        {"mp_lbt_m3_males_01", 8},
        {"mp_lbt_m6_females_01", 2},
        {"mp_lbt_m6_males_01", 5},
        {"mp_lbt_m7_males_01", 8},
        {"mp_oth_recipient_males_01", 5},
        {"mp_outlaw1_males_01", 6},
        {"mp_outlaw2_males_01", 1},
        {"mp_post_multipackage_females_01", 10},
        {"mp_post_multipackage_males_01", 34},
        {"mp_post_multirelay_females_01", 5},
        {"mp_post_multirelay_males_01", 20},
        {"mp_post_relay_females_01", 1},
        {"mp_post_relay_males_01", 23},
        {"mp_predator", 8},
        {"mp_prsn_asn_males_01", 14},
        {"mp_recover_recipient_females_01", 1},
        {"mp_recover_recipient_males_01", 5},
        {"mp_repoboat_recipient_females_01", 2},
        {"mp_repoboat_recipient_males_01", 5},
        {"mp_repo_recipient_females_01", 2},
        {"mp_repo_recipient_males_01", 12},
        {"mp_rescue_bottletree_females_01", 1},
        {"mp_rescue_bottletree_males_01", 2},
        {"mp_rescue_colter_males_01", 1},
        {"mp_rescue_cratersacrifice_males_01", 2},
        {"mp_rescue_heartlands_males_01", 1},
        {"mp_rescue_loftkidnap_males_01", 1},
        {"mp_rescue_lonniesshack_males_01", 3},
        {"mp_rescue_moonstone_males_01", 1},
        {"mp_rescue_mtnmanshack_males_01", 3},
        {"mp_rescue_recipient_females_01", 1},
        {"mp_rescue_recipient_males_01", 7},
        {"mp_rescue_rivalshack_males_01", 2},
        {"mp_rescue_scarlettmeadows_males_01", 1},
        {"mp_rescue_sddogfight_females_01", 1},
        {"mp_rescue_sddogfight_males_01", 1},
        {"mp_resupply_recipient_females_01", 7},
        {"mp_resupply_recipient_males_01", 11},
        {"mp_revenge1_males_01", 3},
        {"mp_re_animalattack_females_01", 4},
        {"mp_re_animalattack_males_01", 4},
        {"mp_re_duel_females_01", 4},
        {"mp_re_duel_males_01", 4},
        {"mp_re_graverobber_females_01", 1},
        {"mp_re_graverobber_males_01", 1},
        {"mp_re_hobodog_females_01", 2},
        {"mp_re_hobodog_males_01", 4},
        {"mp_re_kidnapped_females_01", 6},
        {"mp_re_kidnapped_males_01", 6},
        {"mp_re_photography_females_01", 2},
        {"mp_re_photography_females_02", 2},
        {"mp_re_photography_males_01", 2},
        {"mp_re_rivalcollector_males_01", 20},
        {"mp_re_runawaywagon_females_01", 3},
        {"mp_re_runawaywagon_males_01", 3},
        {"mp_re_treasurehunter_females_01", 2},
        {"mp_re_treasurehunter_males_01", 2},
        {"mp_re_wildman_males_01", 1},
        {"mp_stealboat_recipient_males_01", 10},
        {"mp_stealhorse_recipient_males_01", 29},
        {"mp_stealwagon_recipient_males_01", 22},
        {"mp_s_m_m_cornwallguard_01", 32},
        {"mp_s_m_m_pinlaw_01", 40},
        {"mp_tattoo_female", 1},
        {"mp_tattoo_male", 1},
        {"mp_u_f_m_bountytarget_001", 2},
        {"mp_u_f_m_bountytarget_002", 2},
        {"mp_u_f_m_bountytarget_003", 2},
        {"mp_u_f_m_bountytarget_004", 2},
        {"mp_u_f_m_bountytarget_005", 2},
        {"mp_u_f_m_bountytarget_006", 2},
        {"mp_u_f_m_bountytarget_007", 2},
        {"mp_u_f_m_bountytarget_008", 2},
        {"mp_u_f_m_bountytarget_009", 2},
        {"mp_u_f_m_bountytarget_010", 2},
        {"mp_u_f_m_bountytarget_011", 2},
        {"MP_U_F_M_BOUNTYTARGET_012", 1},
        {"mp_u_f_m_bountytarget_013", 2},
        {"mp_u_f_m_bountytarget_014", 2},
        {"mp_u_f_m_gunslinger3_rifleman_02", 1},
        {"mp_u_f_m_gunslinger3_sharpshooter_01", 1},
        {"mp_u_f_m_laperlevipmasked_01", 1},
        {"mp_u_f_m_laperlevipmasked_02", 1},
        {"mp_u_f_m_laperlevipmasked_03", 1},
        {"mp_u_f_m_laperlevipmasked_04", 1},
        {"mp_u_f_m_laperlevipunmasked_01", 1},
        {"mp_u_f_m_laperlevipunmasked_02", 1},
        {"mp_u_f_m_laperlevipunmasked_03", 1},
        {"mp_u_f_m_laperlevipunmasked_04", 1},
        {"mp_u_f_m_lbt_owlhootvictim_01", 1},
        {"mp_u_f_m_legendarybounty_001", 1},
        {"mp_u_f_m_legendarybounty_002", 8},
        {"mp_u_f_m_outlaw3_warner_01", 1},
        {"mp_u_f_m_outlaw3_warner_02", 1},
        {"mp_u_f_m_revenge2_passerby_01", 1},
        {"mp_u_m_m_armsheriff_01", 1},
        {"mp_u_m_m_bountyinjuredman_01", 1},
        {"mp_u_m_m_bountytarget_001", 2},
        {"mp_u_m_m_bountytarget_002", 2},
        {"mp_u_m_m_bountytarget_003", 2},
        {"mp_u_m_m_bountytarget_005", 2},
        {"mp_u_m_m_bountytarget_008", 2},
        {"mp_u_m_m_bountytarget_009", 2},
        {"mp_u_m_m_bountytarget_010", 2},
        {"mp_u_m_m_bountytarget_011", 2},
        {"mp_u_m_m_bountytarget_012", 2},
        {"mp_u_m_m_bountytarget_013", 2},
        {"mp_u_m_m_bountytarget_014", 2},
        {"mp_u_m_m_bountytarget_015", 2},
        {"mp_u_m_m_bountytarget_016", 2},
        {"mp_u_m_m_bountytarget_017", 2},
        {"mp_u_m_m_bountytarget_018", 2},
        {"mp_u_m_m_bountytarget_019", 2},
        {"mp_u_m_m_bountytarget_020", 2},
        {"mp_u_m_m_bountytarget_021", 2},
        {"mp_u_m_m_bountytarget_022", 2},
        {"mp_u_m_m_bountytarget_023", 2},
        {"mp_u_m_m_bountytarget_024", 2},
        {"mp_u_m_m_bountytarget_025", 2},
        {"mp_u_m_m_bountytarget_026", 2},
        {"mp_u_m_m_bountytarget_027", 2},
        {"mp_u_m_m_bountytarget_028", 2},
        {"mp_u_m_m_bountytarget_029", 2},
        {"mp_u_m_m_bountytarget_030", 2},
        {"mp_u_m_m_bountytarget_031", 2},
        {"mp_u_m_m_bountytarget_032", 2},
        {"mp_u_m_m_bountytarget_033", 2},
        {"mp_u_m_m_bountytarget_034", 2},
        {"mp_u_m_m_bountytarget_035", 2},
        {"mp_u_m_m_bountytarget_036", 2},
        {"mp_u_m_m_bountytarget_037", 2},
        {"mp_u_m_m_bountytarget_038", 2},
        {"mp_u_m_m_bountytarget_039", 2},
        {"mp_u_m_m_bountytarget_044", 2},
        {"mp_u_m_m_bountytarget_045", 2},
        {"mp_u_m_m_bountytarget_046", 2},
        {"mp_u_m_m_bountytarget_047", 2},
        {"mp_u_m_m_bountytarget_048", 2},
        {"mp_u_m_m_bountytarget_049", 2},
        {"mp_u_m_m_bountytarget_050", 2},
        {"mp_u_m_m_bountytarget_051", 2},
        {"mp_u_m_m_bountytarget_052", 2},
        {"mp_u_m_m_bountytarget_053", 2},
        {"mp_u_m_m_bountytarget_054", 2},
        {"mp_u_m_m_bountytarget_055", 2},
        {"mp_u_m_m_gunforhireclerk_01", 2},
        {"mp_u_m_m_gunslinger3_rifleman_01", 1},
        {"mp_u_m_m_gunslinger3_sharpshooter_02", 1},
        {"mp_u_m_m_gunslinger3_shotgunner_01", 1},
        {"mp_u_m_m_gunslinger3_shotgunner_02", 1},
        {"mp_u_m_m_gunslinger4_warner_01", 1},
        {"mp_u_m_m_lbt_accomplice_01", 1},
        {"mp_u_m_m_lbt_barbsvictim_01", 1},
        {"mp_u_m_m_lbt_bribeinformant_01", 1},
        {"mp_u_m_m_lbt_coachdriver_01", 1},
        {"mp_u_m_m_lbt_hostagemarshal_01", 1},
        {"mp_u_m_m_lbt_owlhootvictim_01", 1},
        {"mp_u_m_m_lbt_owlhootvictim_02", 1},
        {"mp_u_m_m_lbt_philipsvictim_01", 2},
        {"mp_u_m_m_legendarybounty_001", 1},
        {"mp_u_m_m_legendarybounty_002", 1},
        {"mp_u_m_m_legendarybounty_003", 1},
        {"mp_u_m_m_legendarybounty_004", 1},
        {"mp_u_m_m_legendarybounty_005", 2},
        {"mp_u_m_m_legendarybounty_006", 2},
        {"mp_u_m_m_legendarybounty_007", 1},
        {"mp_u_m_m_outlaw3_prisoner_01", 1},
        {"mp_u_m_m_outlaw3_prisoner_02", 1},
        {"mp_u_m_m_outlaw3_warner_01", 1},
        {"mp_u_m_m_outlaw3_warner_02", 1},
        {"mp_u_m_m_prisonwagon_01", 2},
        {"mp_u_m_m_prisonwagon_02", 2},
        {"mp_u_m_m_prisonwagon_03", 2},
        {"mp_u_m_m_prisonwagon_04", 2},
        {"mp_u_m_m_prisonwagon_05", 2},
        {"mp_u_m_m_prisonwagon_06", 2},
        {"mp_u_m_m_revenge2_handshaker_01", 1},
        {"mp_u_m_m_revenge2_passerby_01", 1},
        {"mp_u_m_m_traderintroclerk_01", 1},
        {"mp_u_m_m_trader_01", 2},
        {"mp_u_m_m_tvlfence_01", 1},
        {"mp_u_m_o_blwpolicechief_01", 8},
        {"mp_wgnbrkout_recipient_males_01", 27},
        {"mp_wgnthief_recipient_males_01", 24},
        {"msp_bountyhunter1_females_01", 1},
        {"msp_braithwaites1_males_01", 1},
        {"msp_feud1_males_01", 4},
        {"msp_fussar2_males_01", 3},
        {"msp_gang2_males_01", 1},
        {"msp_gang3_males_01", 2},
        {"msp_grays1_males_01", 1},
        {"msp_grays2_males_01", 1},
        {"msp_guarma2_males_01", 3},
        {"msp_industry1_females_01", 7},
        {"msp_industry1_males_01", 8},
        {"msp_industry3_females_01", 1},
        {"msp_industry3_males_01", 8},
        {"msp_mary1_females_01", 1},
        {"msp_mary1_males_01", 2},
        {"msp_mary3_males_01", 6},
        {"msp_mob0_males_01", 1},
        {"msp_mob1_females_01", 8},
        {"msp_mob1_males_01", 7},
        {"msp_mob1_teens_01", 7},
        {"msp_mob3_females_01", 2},
        {"msp_mob3_males_01", 4},
        {"msp_mudtown3b_females_01", 20},
        {"msp_mudtown3b_males_01", 20},
        {"msp_mudtown3_males_01", 2},
        {"msp_mudtown5_males_01", 5},
        {"msp_native1_males_01", 3},
        {"msp_reverend1_males_01", 4},
        {"msp_saintdenis1_females_01", 6},
        {"msp_saintdenis1_males_01", 17},
        {"msp_saloon1_females_01", 22},
        {"msp_saloon1_males_01", 45},
        {"msp_smuggler2_males_01", 2},
        {"msp_trainrobbery2_males_01", 4},
        {"msp_trelawny1_males_01", 7},
        {"msp_utopia1_males_01", 10},
        {"msp_winter4_males_01", 1},
        {"player_three", 33},
        {"player_zero", 74},
        {"rces_abigail3_females_01", 2},
        {"rces_abigail3_males_01", 1},
        {"rces_beechers1_males_01", 4},
        {"rces_evelynmiller_males_01", 3},
        {"rcsp_beauandpenelope1_females_01", 16},
        {"rcsp_beauandpenelope_males_01", 5},
        {"rcsp_calderonstage2_males_01", 2},
        {"rcsp_calderonstage2_teens_01", 3},
        {"rcsp_calderon_males_01", 3},
        {"rcsp_calloway_males_01", 5},
        {"rcsp_coachrobbery_males_01", 5},
        {"rcsp_crackpot_females_01", 2},
        {"rcsp_crackpot_males_01", 4},
        {"rcsp_creole_males_01", 1},
        {"rcsp_dutch1_males_01", 8},
        {"rcsp_dutch3_males_01", 2},
        {"rcsp_edithdownes2_males_01", 2},
        {"rcsp_formyart_females_01", 6},
        {"rcsp_formyart_males_01", 10},
        {"rcsp_gunslingerduel4_males_01", 4},
        {"rcsp_herekittykitty_males_01", 2},
        {"rcsp_hunting1_males_01", 2},
        {"rcsp_mrmayor_males_01", 1},
        {"rcsp_native1s2_males_01", 5},
        {"rcsp_native_americanfathers_males_01", 1},
        {"rcsp_oddfellows_males_01", 1},
        {"rcsp_odriscolls2_females_01", 3},
        {"rcsp_poisonedwell_females_01", 4},
        {"rcsp_poisonedwell_males_01", 6},
        {"rcsp_poisonedwell_teens_01", 2},
        {"rcsp_ridethelightning_females_01", 3},
        {"rcsp_ridethelightning_males_01", 1},
        {"rcsp_sadie1_males_01", 4},
        {"rcsp_slavecatcher_males_01", 2},
        {"re_animalattack_females_01", 1},
        {"re_animalattack_males_01", 2},
        {"re_animalmauling_males_01", 3},
        {"re_approach_males_01", 2},
        {"re_beartrap_males_01", 4},
        {"re_boatattack_males_01", 3},
        {"re_burningbodies_males_01", 3},
        {"re_checkpoint_males_01", 2},
        {"re_coachrobbery_females_01", 1},
        {"re_coachrobbery_males_01", 8},
        {"re_consequence_males_01", 8},
        {"re_corpsecart_females_01", 2},
        {"re_corpsecart_males_01", 3},
        {"re_crashedwagon_males_01", 5},
        {"re_darkalleyambush_males_01", 4},
        {"re_darkalleybum_males_01", 5},
        {"re_darkalleystabbing_males_01", 5},
        {"re_deadbodies_males_01", 2},
        {"re_deadjohn_females_01", 1},
        {"re_deadjohn_males_01", 2},
        {"re_disabledbeggar_males_01", 5},
        {"re_domesticdispute_females_01", 3},
        {"re_domesticdispute_males_01", 4},
        {"re_drownmurder_females_01", 3},
        {"re_drownmurder_males_01", 3},
        {"re_drunkcamp_males_01", 5},
        {"re_drunkdueler_males_01", 2},
        {"re_duelboaster_males_01", 2},
        {"re_duelwinner_females_01", 6},
        {"re_duelwinner_males_01", 22},
        {"re_escort_females_01", 2},
        {"re_executions_males_01", 5},
        {"re_fleeingfamily_females_01", 1},
        {"re_fleeingfamily_males_01", 1},
        {"re_footrobbery_males_01", 3},
        {"re_friendlyoutdoorsman_males_01", 1},
        {"re_frozentodeath_females_01", 1},
        {"re_frozentodeath_males_01", 1},
        {"re_fundraiser_females_01", 2},
        {"re_fussarchase_males_01", 2},
        {"re_goldpanner_males_01", 4},
        {"re_horserace_females_01", 2},
        {"re_horserace_males_01", 2},
        {"re_hostagerescue_females_01", 2},
        {"re_hostagerescue_males_01", 4},
        {"re_inbredkidnap_females_01", 2},
        {"re_inbredkidnap_males_01", 2},
        {"re_injuredrider_males_01", 9},
        {"re_kidnappedvictim_females_01", 4},
        {"re_laramiegangrustling_males_01", 2},
        {"re_loneprisoner_males_01", 2},
        {"re_lostdog_teens_01", 2},
        {"re_lostdrunk_females_01", 3},
        {"re_lostdrunk_males_01", 4},
        {"re_lostfriend_males_01", 5},
        {"re_lostman_males_01", 1},
        {"re_moonshinecamp_males_01", 4},
        {"re_murdercamp_males_01", 3},
        {"re_murdersuicide_females_01", 3},
        {"re_murdersuicide_males_01", 3},
        {"re_nakedswimmer_males_01", 1},
        {"re_ontherun_males_01", 2},
        {"re_outlawlooter_males_01", 24},
        {"re_parlorambush_males_01", 1},
        {"re_peepingtom_females_01", 3},
        {"re_peepingtom_males_01", 8},
        {"re_pickpocket_males_01", 3},
        {"re_pisspot_females_01", 6},
        {"re_pisspot_males_01", 10},
        {"re_playercampstrangers_females_01", 2},
        {"re_playercampstrangers_males_01", 1},
        {"re_poisoned_males_01", 2},
        {"re_policechase_males_01", 4},
        {"re_prisonwagon_females_01", 5},
        {"re_prisonwagon_males_01", 4},
        {"re_publichanging_females_01", 17},
        {"re_publichanging_males_01", 74},
        {"re_publichanging_teens_01", 2},
        {"re_rallydispute_males_01", 4},
        {"re_rallysetup_males_01", 3},
        {"re_rally_males_01", 13},
        {"re_ratinfestation_males_01", 1},
        {"re_rowdydrunks_males_01", 13},
        {"re_savageaftermath_females_01", 2},
        {"re_savageaftermath_males_01", 5},
        {"re_savagefight_females_01", 3},
        {"re_savagefight_males_01", 3},
        {"re_savagewagon_females_01", 4},
        {"re_savagewagon_males_01", 6},
        {"re_savagewarning_males_01", 7},
        {"re_sharpshooter_males_01", 2},
        {"re_showoff_males_01", 8},
        {"re_skippingstones_males_01", 1},
        {"re_skippingstones_teens_01", 1},
        {"re_slumambush_females_01", 2},
        {"re_snakebite_males_01", 2},
        {"re_stalkinghunter_males_01", 3},
        {"re_strandedrider_males_01", 4},
        {"re_street_fight_males_01", 24},
        {"re_taunting_01", 2},
        {"re_taunting_males_01", 3},
        {"re_torturingcaptive_males_01", 2},
        {"re_townburial_males_01", 18},
        {"re_townconfrontation_females_01", 1},
        {"re_townconfrontation_males_01", 4},
        {"re_townrobbery_males_01", 2},
        {"re_townwidow_females_01", 3},
        {"re_trainholdup_females_01", 3},
        {"re_trainholdup_males_01", 30},
        {"re_trappedwoman_females_01", 3},
        {"re_treasurehunter_males_01", 4},
        {"re_voice_females_01", 1},
        {"re_wagonthreat_females_01", 1},
        {"re_wagonthreat_males_01", 2},
        {"re_washedashore_males_01", 2},
        {"re_wealthycouple_females_01", 2},
        {"re_wealthycouple_males_01", 2},
        {"re_wildman_01", 2},
        {"shack_missinghusband_males_01", 1},
        {"shack_ontherun_males_01", 3},
        {"s_f_m_bwmworker_01", 20},
        {"s_f_m_cghworker_01", 20},
        {"s_f_m_mapworker_01", 10},
        {"s_m_m_ambientblwpolice_01", 43},
        {"s_m_m_ambientlawrural_01", 95},
        {"s_m_m_ambientsdpolice_01", 44},
        {"s_m_m_army_01", 59},
        {"s_m_m_asbcowpoke_01", 30},
        {"s_m_m_asbdealer_01", 10},
        {"s_m_m_bankclerk_01", 13},
        {"s_m_m_barber_01", 7},
        {"s_m_m_blwcowpoke_01", 30},
        {"s_m_m_blwdealer_01", 8},
        {"s_m_m_bwmworker_01", 20},
        {"s_m_m_cghworker_01", 30},
        {"s_m_m_cktworker_01", 50},
        {"s_m_m_coachtaxidriver_01", 16},
        {"s_m_m_cornwallguard_01", 43},
        {"s_m_m_dispatchlawrural_01", 65},
        {"s_m_m_dispatchleaderpolice_01", 20},
        {"s_m_m_dispatchleaderrural_01", 11},
        {"s_m_m_dispatchpolice_01", 40},
        {"s_m_m_fussarhenchman_01", 67},
        {"s_m_m_genconductor_01", 11},
        {"s_m_m_hofguard_01", 20},
        {"s_m_m_liveryworker_01", 24},
        {"s_m_m_magiclantern_01", 4},
        {"s_m_m_mapworker_01", 10},
        {"s_m_m_marketvendor_01", 16},
        {"s_m_m_marshallsrural_01", 33},
        {"s_m_m_micguard_01", 23},
        {"s_m_m_nbxriverboatdealers_01", 20},
        {"s_m_m_nbxriverboatguards_01", 25},
        {"s_m_m_orpguard_01", 20},
        {"s_m_m_pinlaw_01", 55},
        {"s_m_m_racrailguards_01", 20},
        {"s_m_m_racrailworker_01", 30},
        {"s_m_m_rhdcowpoke_01", 20},
        {"s_m_m_rhddealer_01", 8},
        {"s_m_m_sdcowpoke_01", 35},
        {"s_m_m_sddealer_01", 8},
        {"s_m_m_sdticketseller_01", 6},
        {"s_m_m_skpguard_01", 20},
        {"s_m_m_stgsailor_01", 20},
        {"s_m_m_strcowpoke_01", 21},
        {"s_m_m_strdealer_01", 8},
        {"s_m_m_strlumberjack_01", 16},
        {"s_m_m_tailor_01", 7},
        {"s_m_m_trainstationworker_01", 28},
        {"s_m_m_tumdeputies_01", 22},
        {"s_m_m_unibutchers_01", 13},
        {"s_m_m_unitrainengineer_01", 10},
        {"s_m_m_unitrainguards_01", 21},
        {"s_m_m_valbankguards_01", 10},
        {"s_m_m_valcowpoke_01", 21},
        {"s_m_m_valdealer_01", 8},
        {"s_m_m_valdeputy_01", 20},
        {"s_m_m_vhtdealer_01", 8},
        {"s_m_o_cktworker_01", 8},
        {"s_m_y_army_01", 43},
        {"s_m_y_newspaperboy_01", 16},
        {"s_m_y_racrailworker_01", 30},
        {"u_f_m_bht_wife", 1},
        {"u_f_m_circuswagon_01", 1},
        {"u_f_m_emrdaughter_01", 5},
        {"u_f_m_fussar1lady_01", 1},
        {"u_f_m_htlwife_01", 1},
        {"u_f_m_lagmother_01", 9},
        {"u_f_m_nbxresident_01", 2},
        {"u_f_m_rhdnudewoman_01", 1},
        {"u_f_m_rkshomesteadtenant_01", 1},
        {"u_f_m_story_blackbelle_01", 1},
        {"u_f_m_story_nightfolk_01", 1},
        {"u_f_m_tljbartender_01", 9},
        {"u_f_m_tumgeneralstoreowner_01", 9},
        {"u_f_m_valtownfolk_01", 2},
        {"u_f_m_valtownfolk_02", 2},
        {"u_f_m_vhtbartender_01", 7},
        {"u_f_o_hermit_woman_01", 1},
        {"u_f_o_wtctownfolk_01", 1},
        {"u_f_y_braithwaitessecret_01", 2},
        {"u_f_y_czphomesteaddaughter_01", 1},
        {"u_m_m_announcer_01", 2},
        {"u_m_m_apfdeadman_01", 1},
        {"u_m_m_armgeneralstoreowner_01", 9},
        {"u_m_m_armtrainstationworker_01", 9},
        {"u_m_m_armundertaker_01", 9},
        {"u_m_m_armytrn4_01", 1},
        {"u_m_m_asbgunsmith_01", 9},
        {"u_m_m_asbprisoner_01", 2},
        {"u_m_m_asbprisoner_02", 2},
        {"u_m_m_bht_banditomine", 2},
        {"u_m_m_bht_banditoshack", 4},
        {"u_m_m_bht_benedictallbright", 1},
        {"u_m_m_bht_blackwaterhunt", 1},
        {"u_m_m_bht_exconfedcampreturn", 1},
        {"u_m_m_bht_laramiesleeping", 1},
        {"u_m_m_bht_lover", 1},
        {"u_m_m_bht_mineforeman", 1},
        {"u_m_m_bht_nathankirk", 1},
        {"u_m_m_bht_odriscolldrunk", 1},
        {"u_m_m_bht_odriscollmauled", 1},
        {"u_m_m_bht_odriscollsleeping", 1},
        {"u_m_m_bht_oldman", 1},
        {"U_M_M_BHT_OUTLAWMAULED", 1},
        {"u_m_m_bht_saintdenissaloon", 1},
        {"u_m_m_bht_shackescape", 1},
        {"u_m_m_bht_skinnerbrother", 3},
        {"u_m_m_bht_skinnersearch", 3},
        {"u_m_m_bht_strawberryduel", 1},
        {"u_m_m_bivforeman_01", 5},
        {"u_m_m_blwtrainstationworker_01", 9},
        {"u_m_m_bulletcatchvolunteer_01", 2},
        {"u_m_m_bwmstablehand_01", 4},
        {"u_m_m_cabaretfirehat_01", 1},
        {"u_m_m_cajhomestead_01", 1},
        {"u_m_m_chelonianjumper_01", 2},
        {"u_m_m_chelonianjumper_02", 2},
        {"u_m_m_chelonianjumper_03", 2},
        {"u_m_m_chelonianjumper_04", 2},
        {"u_m_m_circuswagon_01", 1},
        {"u_m_m_cktmanager_01", 1},
        {"u_m_m_cornwalldriver_01", 1},
        {"u_m_m_crdhomesteadtenant_01", 2},
        {"u_m_m_crdhomesteadtenant_02", 2},
        {"u_m_m_crdwitness_01", 2},
        {"u_m_m_creolecaptain_01", 1},
        {"u_m_m_czphomesteadfather_01", 1},
        {"u_m_m_dorhomesteadhusband_01", 1},
        {"u_m_m_emrfarmhand_03", 2},
        {"u_m_m_emrfather_01", 5},
        {"u_m_m_executioner_01", 1},
        {"u_m_m_fatduster_01", 2},
        {"u_m_m_finale2_aa_upperclass_01", 2},
        {"u_m_m_galastringquartet_01", 2},
        {"u_m_m_galastringquartet_02", 2},
        {"u_m_m_galastringquartet_03", 2},
        {"u_m_m_galastringquartet_04", 2},
        {"u_m_m_gamdoorman_01", 2},
        {"u_m_m_hhrrancher_01", 2},
        {"u_m_m_htlforeman_01", 5},
        {"u_m_m_htlhusband_01", 1},
        {"u_m_m_htlrancherbounty_01", 2},
        {"u_m_m_islbum_01", 1},
        {"u_m_m_lnsoutlaw_01", 2},
        {"u_m_m_lnsoutlaw_02", 2},
        {"u_m_m_lnsoutlaw_03", 2},
        {"u_m_m_lnsoutlaw_04", 2},
        {"u_m_m_lnsworker_01", 2},
        {"u_m_m_lnsworker_02", 2},
        {"u_m_m_lnsworker_03", 2},
        {"u_m_m_lnsworker_04", 2},
        {"u_m_m_lrshomesteadtenant_01", 2},
        {"u_m_m_mfrrancher_01", 2},
        {"u_m_m_mud3pimp_01", 2},
        {"u_m_m_nbxbankerbounty_01", 2},
        {"u_m_m_nbxbartender_01", 9},
        {"u_m_m_nbxbartender_02", 9},
        {"u_m_m_nbxboatticketseller_01", 5},
        {"u_m_m_nbxbronteasc_01", 1},
        {"u_m_m_nbxbrontegoon_01", 1},
        {"u_m_m_nbxbrontesecform_01", 1},
        {"u_m_m_nbxgeneralstoreowner_01", 9},
        {"u_m_m_nbxgraverobber_01", 2},
        {"u_m_m_nbxgraverobber_02", 2},
        {"u_m_m_nbxgraverobber_03", 2},
        {"u_m_m_nbxgraverobber_04", 2},
        {"u_m_m_nbxgraverobber_05", 2},
        {"u_m_m_nbxgunsmith_01", 9},
        {"u_m_m_nbxliveryworker_01", 2},
        {"u_m_m_nbxmusician_01", 2},
        {"u_m_m_nbxpriest_01", 1},
        {"u_m_m_nbxresident_01", 2},
        {"u_m_m_nbxresident_02", 2},
        {"u_m_m_nbxresident_03", 2},
        {"u_m_m_nbxresident_04", 2},
        {"u_m_m_nbxriverboatpitboss_01", 2},
        {"u_m_m_nbxriverboattarget_01", 2},
        {"u_m_m_nbxshadydealer_01", 9},
        {"u_m_m_nbxskiffdriver_01", 2},
        {"u_m_m_oddfellowparticipant_01", 2},
        {"u_m_m_odriscollbrawler_01", 3},
        {"u_m_m_orpguard_01", 1},
        {"u_m_m_racforeman_01", 5},
        {"u_m_m_racquartermaster_01", 2},
        {"u_m_m_rhdbackupdeputy_01", 2},
        {"u_m_m_rhdbackupdeputy_02", 2},
        {"u_m_m_rhdbartender_01", 7},
        {"u_m_m_rhddoctor_01", 1},
        {"u_m_m_rhdfiddleplayer_01", 2},
        {"u_m_m_rhdgenstoreowner_01", 9},
        {"u_m_m_rhdgenstoreowner_02", 9},
        {"u_m_m_rhdgunsmith_01", 9},
        {"u_m_m_rhdpreacher_01", 1},
        {"u_m_m_rhdsheriff_01", 5},
        {"u_m_m_rhdtrainstationworker_01", 5},
        {"u_m_m_rhdundertaker_01", 1},
        {"u_m_m_riodonkeyrider_01", 1},
        {"u_m_m_rkfrancher_01", 1},
        {"u_m_m_rkrdonkeyrider_01", 1},
        {"u_m_m_rwfrancher_01", 1},
        {"u_m_m_sdbankguard_01", 4},
        {"u_m_m_sdcustomvendor_01", 9},
        {"u_m_m_sdexoticsshopkeeper_01", 10},
        {"u_m_m_sdphotographer_01", 2},
        {"u_m_m_sdpolicechief_01", 5},
        {"u_m_m_sdstrongwomanassistant_01", 1},
        {"u_m_m_sdtrapper_01", 1},
        {"u_m_m_sdwealthytraveller_01", 5},
        {"u_m_m_shackserialkiller_01", 1},
        {"u_m_m_shacktwin_01", 2},
        {"u_m_m_shacktwin_02", 2},
        {"u_m_m_skinnyoldguy_01", 1},
        {"u_m_m_story_armadillo_01", 1},
        {"u_m_m_story_cannibal_01", 1},
        {"u_m_m_story_chelonian_01", 1},
        {"u_m_m_story_copperhead_01", 1},
        {"u_m_m_story_creeper_01", 1},
        {"u_m_m_story_emeraldranch_01", 1},
        {"u_m_m_story_hunter_01", 1},
        {"u_m_m_story_manzanita_01", 2},
        {"u_m_m_story_murfee_01", 1},
        {"u_m_m_story_pigfarm_01", 1},
        {"u_m_m_story_princess_01", 1},
        {"u_m_m_story_redharlow_01", 1},
        {"u_m_m_story_rhodes_01", 1},
        {"u_m_m_story_sdstatue_01", 1},
        {"u_m_m_story_spectre_01", 1},
        {"u_m_m_story_treasure_01", 1},
        {"u_m_m_story_tumbleweed_01", 1},
        {"u_m_m_story_valentine_01", 1},
        {"u_m_m_strfreightstationowner_01", 5},
        {"u_m_m_strgenstoreowner_01", 9},
        {"u_m_m_strsherriff_01", 5},
        {"u_m_m_strwelcomecenter_01", 9},
        {"u_m_m_tumbartender_01", 7},
        {"u_m_m_tumbutcher_01", 9},
        {"u_m_m_tumgunsmith_01", 9},
        {"u_m_m_tumtrainstationworker_01", 1},
        {"u_m_m_unibountyhunter_01", 1},
        {"u_m_m_unibountyhunter_02", 1},
        {"u_m_m_unidusterhenchman_01", 2},
        {"u_m_m_unidusterhenchman_02", 2},
        {"u_m_m_unidusterhenchman_03", 2},
        {"u_m_m_unidusterleader_01", 2},
        {"u_m_m_uniexconfedsbounty_01", 2},
        {"u_m_m_unionleader_01", 2},
        {"u_m_m_unionleader_02", 2},
        {"u_m_m_unipeepingtom_01", 2},
        {"u_m_m_valauctionforman_01", 7},
        {"u_m_m_valauctionforman_02", 6},
        {"u_m_m_valbarber_01", 9},
        {"u_m_m_valbartender_01", 9},
        {"u_m_m_valbeartrap_01", 5},
        {"u_m_m_valbutcher_01", 9},
        {"u_m_m_valdoctor_01", 9},
        {"u_m_m_valgenstoreowner_01", 9},
        {"u_m_m_valgunsmith_01", 9},
        {"u_m_m_valhotelowner_01", 9},
        {"u_m_m_valpokerplayer_01", 5},
        {"u_m_m_valpokerplayer_02", 5},
        {"u_m_m_valpoopingman_01", 4},
        {"u_m_m_valsheriff_01", 5},
        {"u_m_m_valtheman_01", 9},
        {"u_m_m_valtownfolk_01", 4},
        {"u_m_m_valtownfolk_02", 4},
        {"u_m_m_vhtstationclerk_01", 9},
        {"u_m_m_walgeneralstoreowner_01", 5},
        {"u_m_m_wapofficial_01", 2},
        {"u_m_m_wtccowboy_04", 2},
        {"u_m_o_armbartender_01", 9},
        {"u_m_o_asbsheriff_01", 5},
        {"u_m_o_bht_docwormwood", 2},
        {"u_m_o_blwbartender_01", 9},
        {"u_m_o_blwgeneralstoreowner_01", 9},
        {"u_m_o_blwphotographer_01", 9},
        {"u_m_o_blwpolicechief_01", 7},
        {"u_m_o_cajhomestead_01", 1},
        {"u_m_o_cmrcivilwarcommando_01", 2},
        {"u_m_o_mapwiseoldman_01", 5},
        {"u_m_o_oldcajun_01", 1},
        {"u_m_o_pshrancher_01", 5},
        {"u_m_o_rigtrainstationworker_01", 5},
        {"u_m_o_valbartender_01", 8},
        {"u_m_o_vhtexoticshopkeeper_01", 9},
        {"u_m_y_cajhomestead_01", 1},
        {"u_m_y_czphomesteadson_01", 1},
        {"u_m_y_czphomesteadson_02", 1},
        {"u_m_y_czphomesteadson_03", 1},
        {"u_m_y_czphomesteadson_04", 1},
        {"u_m_y_czphomesteadson_05", 1},
        {"u_m_y_duellistbounty_01", 2},
        {"u_m_y_emrson_01", 4},
        {"u_m_y_htlworker_01", 5},
        {"u_m_y_htlworker_02", 5},
        {"u_m_y_shackstarvingkid_01", 3},
        {"a_c_horse_norfolkroadster_black", 1},
        {"a_c_horse_norfolkroadster_dappledbuckskin", 1},
        {"a_c_horse_norfolkroadster_piebaldroan", 1},
        {"a_c_horse_norfolkroadster_rosegrey", 1},
        {"a_c_horse_norfolkroadster_speckledgrey", 1},
        {"a_c_horse_norfolkroadster_spottedtricolor", 1},
        {"cs_mp_agent_hixon", 2},
        {"cs_mp_dannylee", 2},
        {"cs_mp_gus_macmillan", 2},
        {"cs_mp_harriet_davenport", 1},
        {"cs_mp_lem", 3},
        {"cs_mp_maggie", 2},
        {"cs_mp_seth", 1},
        {"cs_mp_the_boy", 2},
        {"mp_a_c_alligator_01", 4},
        {"mp_a_c_beaver_01", 3},
        {"mp_a_c_boar_01", 5},
        {"mp_a_c_cougar_01", 4},
        {"mp_a_c_coyote_01", 4},
        {"mp_a_c_panther_01", 4},
        {"mp_a_c_wolf_01", 5},
        {"mp_a_f_m_saloonpatrons_01", 20},
        {"mp_a_f_m_saloonpatrons_02", 22},
        {"mp_a_f_m_saloonpatrons_03", 20},
        {"mp_a_f_m_saloonpatrons_04", 20},
        {"mp_a_f_m_saloonpatrons_05", 20},
        {"mp_a_m_m_moonshinemakers_01", 30},
        {"mp_a_m_m_saloonpatrons_01", 35},
        {"mp_a_m_m_saloonpatrons_02", 35},
        {"mp_a_m_m_saloonpatrons_03", 35},
        {"mp_a_m_m_saloonpatrons_04", 35},
        {"mp_a_m_m_saloonpatrons_05", 35},
        {"mp_g_m_m_animalpoachers_01", 71},
        {"mp_g_m_m_unicriminals_03", 55},
        {"mp_g_m_m_unicriminals_04", 55},
        {"mp_g_m_m_unicriminals_05", 55},
        {"mp_g_m_m_unicriminals_06", 50},
        {"mp_g_m_m_unicriminals_07", 55},
        {"mp_g_m_m_unicriminals_08", 50},
        {"mp_g_m_m_unicriminals_09", 50},
        {"mp_re_moonshinecamp_males_01", 10},
        {"mp_s_m_m_revenueagents_01", 49},
        {"mp_u_f_m_buyer_improved_01", 1},
        {"mp_u_f_m_buyer_improved_02", 1},
        {"mp_u_f_m_buyer_regular_01", 1},
        {"mp_u_f_m_buyer_regular_02", 1},
        {"mp_u_f_m_buyer_special_01", 1},
        {"mp_u_f_m_buyer_special_02", 1},
        {"mp_u_m_m_buyer_default_01", 1},
        {"mp_u_m_m_buyer_improved_01", 1},
        {"mp_u_m_m_buyer_improved_02", 1},
        {"mp_u_m_m_buyer_improved_03", 1},
        {"mp_u_m_m_buyer_improved_04", 1},
        {"mp_u_m_m_buyer_improved_05", 1},
        {"mp_u_m_m_buyer_improved_06", 1},
        {"mp_u_m_m_buyer_improved_07", 1},
        {"mp_u_m_m_buyer_improved_08", 1},
        {"mp_u_m_m_buyer_regular_01", 1},
        {"mp_u_m_m_buyer_regular_02", 1},
        {"mp_u_m_m_buyer_regular_03", 1},
        {"mp_u_m_m_buyer_regular_04", 1},
        {"mp_u_m_m_buyer_regular_05", 1},
        {"mp_u_m_m_buyer_regular_06", 1},
        {"mp_u_m_m_buyer_regular_07", 1},
        {"mp_u_m_m_buyer_regular_08", 1},
        {"mp_u_m_m_buyer_special_01", 1},
        {"mp_u_m_m_buyer_special_02", 1},
        {"mp_u_m_m_buyer_special_03", 1},
        {"mp_u_m_m_buyer_special_04", 1},
        {"mp_u_m_m_buyer_special_05", 1},
        {"mp_u_m_m_buyer_special_06", 1},
        {"mp_u_m_m_buyer_special_07", 1},
        {"mp_u_m_m_buyer_special_08", 1},
        {"mp_u_m_m_lawcamp_prisoner_01", 1},
        {"mp_u_m_m_saloonbrawler_01", 1},
        {"mp_u_m_m_saloonbrawler_02", 1},
        {"mp_u_m_m_saloonbrawler_03", 1},
        {"mp_u_m_m_saloonbrawler_04", 1},
        {"mp_u_m_m_saloonbrawler_05", 1},
        {"mp_u_m_m_saloonbrawler_06", 1},
        {"mp_u_m_m_saloonbrawler_07", 1},
        {"mp_u_m_m_saloonbrawler_08", 1},
        {"mp_u_m_m_saloonbrawler_09", 1},
        {"mp_u_m_m_saloonbrawler_10", 1},
        {"mp_u_m_m_saloonbrawler_11", 1},
        {"mp_u_m_m_saloonbrawler_12", 1},
        {"mp_u_m_m_saloonbrawler_13", 1},
        {"mp_u_m_m_saloonbrawler_14", 1},
        {"a_c_horse_gypsycob_palominoblagdon", 1},
        {"a_c_horse_gypsycob_piebald", 1},
        {"a_c_horse_gypsycob_skewbald", 1},
        {"a_c_horse_gypsycob_splashedbay", 1},
        {"a_c_horse_gypsycob_splashedpiebald", 1},
        {"a_c_horse_gypsycob_whiteblagdon", 1},
        {"mp_a_c_bear_01", 4},
        {"mp_a_c_bighornram_01", 5},
        {"mp_a_c_buffalo_01", 4},
        {"mp_a_c_dogamericanfoxhound_01", 2},
        {"mp_a_c_elk_01", 4},
        {"mp_a_c_fox_01", 4},
        {"mp_a_c_moose_01", 4},
        {"mp_a_c_owl_01", 1},
        {"mp_a_c_possum_01", 1},
        {"mp_a_c_pronghorn_01", 1},
        {"mp_a_c_rabbit_01", 1},
        {"mp_a_c_sheep_01", 1},
        {"mp_a_f_m_saloonband_females_01", 5},
        {"mp_u_m_m_animalpoacher_01", 1},
        {"mp_u_m_m_animalpoacher_02", 1},
        {"mp_u_m_m_animalpoacher_03", 1},
        {"mp_u_m_m_animalpoacher_04", 1},
        {"mp_u_m_m_animalpoacher_05", 2},
        {"mp_u_m_m_animalpoacher_06", 1},
        {"mp_u_m_m_animalpoacher_07", 1},
        {"mp_u_m_m_animalpoacher_08", 1},
        {"mp_u_m_m_animalpoacher_09", 1},
        {"mp_g_f_m_armyoffear_01", 20},
        {"mp_g_m_m_armyoffear_01", 30},
        {"cs_mp_bessie_adair", 1},
        {"mp_a_c_chicken_01", 2},
        {"mp_a_c_deer_01", 1},
        {"mp_a_m_m_saloonband_males_01", 20},
        {"mp_re_slumpedhunter_females_01", 4},
        {"mp_re_slumpedhunter_males_01", 6},
        {"mp_re_suspendedhunter_males_01", 2},
        {"mp_u_f_m_nat_traveler_01", 1},
        {"mp_u_f_m_nat_worker_01", 1},
        {"mp_u_f_m_nat_worker_02", 1},
        {"mp_u_f_m_saloonpianist_01", 1},
        {"mp_u_m_m_dyingpoacher_01", 1},
        {"mp_u_m_m_dyingpoacher_02", 1},
        {"mp_u_m_m_dyingpoacher_03", 1},
        {"mp_u_m_m_dyingpoacher_04", 1},
        {"mp_u_m_m_dyingpoacher_05", 1},
        {"mp_u_m_m_lawcamp_lawman_01", 1},
        {"mp_u_m_m_lawcamp_lawman_02", 1},
        {"mp_u_m_m_lawcamp_leadofficer_01", 1},
        {"mp_u_m_m_nat_farmer_01", 1},
        {"mp_u_m_m_nat_farmer_02", 1},
        {"mp_u_m_m_nat_farmer_03", 1},
        {"mp_u_m_m_nat_farmer_04", 1},
        {"mp_u_m_m_nat_photographer_01", 1},
        {"mp_u_m_m_nat_photographer_02", 1},
        {"mp_u_m_m_nat_rancher_01", 1},
        {"mp_u_m_m_nat_rancher_02", 1},
        {"mp_u_m_m_nat_townfolk_01", 1},
        {"mp_u_m_m_strwelcomecenter_02", 1},
        {"a_c_horse_missourifoxtrotter_blacktovero", 1},
        {"a_c_horse_missourifoxtrotter_blueroan", 1},
        {"a_c_horse_missourifoxtrotter_buckskinbrindle", 1},
        {"a_c_horse_missourifoxtrotter_dapplegrey", 1},
        {"a_c_horse_mustang_blackovero", 2},
        {"a_c_horse_mustang_buckskin", 2},
        {"a_c_horse_mustang_chestnuttovero", 2},
        {"a_c_horse_mustang_reddunovero", 2},
        {"a_c_horse_turkoman_black", 1},
        {"a_c_horse_turkoman_chestnut", 1},
        {"a_c_horse_turkoman_grey", 1},
        {"a_c_horse_turkoman_perlino", 1},
        {"MP_BEAU_BINK_FEMALES_01", 1},
        {"MP_BEAU_BINK_MALES_01", 7},
        {"MP_CARMELA_BINK_VICTIM_MALES_01", 5},
        {"MP_CD_REVENGEMAYOR_01", 21},
        {"mp_fm_bounty_caged_males_01", 2},
        {"mp_fm_bounty_ct_corpses_01", 2},
        {"mp_fm_bounty_hideout_males_01", 1},
        {"mp_fm_bounty_horde_males_01", 5},
        {"mp_fm_bounty_infiltration_males_01", 2},
        {"MP_FM_BOUNTYTARGET_MALES_DLC008_01", 53},
        {"mp_fm_knownbounty_guards_01", 1},
        {"MP_FM_KNOWNBOUNTY_INFORMANTS_FEMALES_01", 1},
        {"mp_fm_knownbounty_informants_males_01", 7},
        {"mp_fm_multitrack_victims_males_01", 2},
        {"mp_fm_stakeout_corpses_males_01", 2},
        {"mp_fm_stakeout_poker_males_01", 2},
        {"mp_fm_stakeout_target_males_01", 6},
        {"mp_fm_track_prospector_01", 1},
        {"mp_fm_track_sd_lawman_01", 1},
        {"mp_fm_track_targets_males_01", 3},
        {"MP_G_F_M_CULTGUARDS_01", 16},
        {"mp_g_f_m_cultmembers_01", 13},
        {"MP_G_M_M_CULTGUARDS_01", 27},
        {"mp_g_m_m_cultmembers_01", 22},
        {"MP_G_M_M_MERCS_01", 2},
        {"MP_G_M_M_RIFLECRONIES_01", 6},
        {"MP_LBM_CARMELA_BANDITOS_01", 4},
        {"MP_LM_STEALHORSE_BUYERS_01", 3},
        {"MP_U_F_M_CULTPRIEST_01", 1},
        {"MP_U_F_M_LEGENDARYBOUNTY_03", 5},
        {"MP_U_M_M_BANKPRISONER_01", 1},
        {"MP_U_M_M_BINKMERCS_01", 7},
        {"MP_U_M_M_CULTPRIEST_01", 3},
        {"MP_U_M_M_DROPOFF_JOSIAH_01", 1},
        {"MP_U_M_M_LEGENDARYBOUNTY_08", 3},
        {"MP_U_M_M_LEGENDARYBOUNTY_09", 1},
        {"mp_U_M_M_RHD_BOUNTYTARGET_01", 1},
        {"mp_U_M_M_RHD_BOUNTYTARGET_02", 1},
        {"mp_U_M_M_RHD_BOUNTYTARGET_03", 1},
        {"mp_U_M_M_RHD_BOUNTYTARGET_03B", 1},
        {"MP_U_M_M_LOM_TRAIN_CONDUCTOR_01", 1},
        {"MP_U_M_M_OUTLAW_COACHDRIVER_01", 1},
        {"MP_U_M_M_FOS_DOCKWORKER_01", 1},
        {"MP_U_M_M_DROPOFF_BRONTE_01", 1},
        {"MP_U_M_M_HCTEL_ARM_HOSTAGE_02", 1},
        {"MP_U_M_M_FOS_HARBORMASTER_01", 1},
        {"MP_U_M_M_FOS_TOWN_VIGILANTE_01", 1},
        {"MP_U_M_M_OUTLAW_COVINGTON_01", 1},
        {"MP_A_F_M_PROTECT_ENDFLOW_BLACKWATER_01", 5},
        {"MP_U_M_M_FOS_BAGHOLDERS_01", 5},
        {"MP_U_M_M_HCTEL_ARM_HOSTAGE_01", 1},
        {"MP_U_M_M_LOM_TRAIN_BARRICADE_01", 7},
        {"MP_U_M_M_FOS_SDSALOON_RECIPIENT_02", 1},
        {"MP_U_M_M_FOS_MUSICIAN_01", 1},
        {"MP_CS_ANTONYFOREMEN", 2},
        {"MP_A_M_M_PROTECT_ENDFLOW_BLACKWATER_01", 5},
        {"MP_U_M_M_FOS_RAILWAY_FOREMAN_01", 1},
        {"MP_FM_BOUNTYTARGET_FEMALES_DLC008_01", 5},
        {"MP_U_M_M_PROTECT_ARMADILLO_01", 1},
        {"MP_U_M_M_PROTECT_FRIENDLY_ARMADILLO_01", 5},
        {"MP_U_M_M_LOM_TRAIN_PRISONERS_01", 2},
        {"MP_A_M_M_COACHGUARDS_01", 15},
        {"MP_U_M_M_FOS_ROGUETHIEF_01", 1},
        {"MP_U_M_M_FOS_RAILWAY_DRIVER_01", 1},
        {"MP_U_M_M_LOM_ASBMERCS_01", 2},
        {"MP_U_M_M_LOM_TRAIN_WAGONDROPOFF_01", 1},
        {"MP_U_M_O_LOM_ASBFOREMAN_01", 1},
        {"MP_G_M_M_MOUNTAINMEN_01", 25},
        {"MP_U_M_M_PROTECT_STRAWBERRY_01", 1},
        {"MP_U_M_M_PROTECT_HALLOWEEN_NED_01", 1},
        {"MP_U_M_M_OUTLAW_MPVICTIM_01", 1},
        {"MP_U_M_M_LOM_SD_DOCKWORKER_01", 1},
        {"MP_U_M_M_FOS_TOWN_OUTLAW_01", 4},
        {"MP_U_M_M_FOS_RECOVERY_RECIPIENT_01", 6},
        {"MP_U_M_M_HCTEL_ARM_HOSTAGE_03", 1},
        {"MP_U_M_M_FOS_DROPOFF_01", 1},
        {"MP_U_M_M_FOS_SDSALOON_GAMBLER_01", 1},
        {"MP_U_M_M_LOM_TRAIN_LAWTARGET_01", 1},
        {"MP_G_M_M_FOS_DEBTGANGCAPITALI_01", 5},
        {"MP_A_M_M_LOM_ASBMINERS_01", 2},
        {"MP_FM_BOUNTY_HORDE_LAW_01", 2},
        {"MP_U_M_M_LOM_HEAD_SECURITY_01", 4},
        {"MP_U_M_M_FOS_SDSALOON_OWNER_01", 1},
        {"MP_U_M_M_MUSICIAN_01", 1},
        {"MP_U_M_M_OUTLAW_ARRESTEDTHIEF_01", 1},
        {"MP_U_M_M_FOS_RAILWAY_BARON_01", 1},
        {"MP_G_M_O_UNIEXCONFEDS_CAP_01", 1},
        {"MP_U_M_M_FOS_SDSALOON_RECIPIENT_01", 2},
        {"MP_U_M_M_INTERROGATOR_01", 1},
        {"CS_MP_POLICECHIEF_LAMBERT", 1},
        {"MP_A_M_M_ASBMINERS_01", 2},
        {"MP_U_M_M_ASBDEPUTY_01", 1},
        {"MP_U_M_M_LOM_DROPOFF_BRONTE_01", 1},
        {"MP_BINK_EMBER_OF_THE_EAST_MALES_01", 9},
        {"mp_U_M_M_FOS_RAILWAY_GUARDS_01", 3},
        {"MP_G_M_M_FOS_VIGILANTES_01", 10},
        {"mp_S_M_M_REVENUEAGENTS_CAP_01", 3},
        {"MP_U_M_M_LOM_SALOON_DRUNK_01", 1},
        {"MP_A_M_M_ASBMINERS_02", 2},
        {"CS_MP_SENATOR_RICARD", 2},
        {"MP_U_M_M_LOM_TRAIN_CLERK_01", 1},
        {"MP_U_M_M_PROTECT_VALENTINE_01", 1},
        {"MP_S_M_M_FOS_HARBORGUARDS_01", 20},
        {"MP_U_M_M_PROTECT_MERCER_CONTACT_01", 1},
        {"MP_U_M_M_OUTLAW_RHD_NOBLE_01", 4},
        {"MP_A_M_M_JAMESONGUARD_01", 50},
        {"MP_U_M_M_FOS_RAILWAY_RECIPIENT_01", 1},
        {"MP_U_M_M_PROTECT_BLACKWATER_01", 1},
        {"MP_U_M_M_HCTEL_SD_TARGET_02", 2},
        {"MP_G_M_M_FOS_DEBTGANG_01", 15},
        {"MP_U_M_M_FOS_SABOTEUR_01", 1},
        {"MP_U_M_M_HCTEL_SD_GANG_01", 3},
        {"MP_U_M_M_PROTECT_MACFARLANES_CONTACT_01", 1},
        {"MP_U_M_M_LOM_DOCKWORKER_01", 1},
        {"MP_U_M_M_FOS_DOCKRECIPIENTS_01", 2},
        {"MP_U_F_M_OUTLAW_SOCIETYLADY_01", 1},
        {"MP_U_M_M_LOM_RHD_SHERIFF_01", 1},
        {"MP_U_M_M_FOS_CORNWALL_BANDITS_01", 3},
        {"MP_U_M_M_PROTECT_STRAWBERRY", 1},
        {"MP_GuidoMartelli", 1},
        {"MP_U_M_M_HARBORMASTER_01", 1},
        {"MP_U_M_M_LOM_RHD_SMITHASSISTANT_01", 1},
        {"MP_U_M_M_FOS_INTERROGATOR_02", 1},
        {"MP_U_M_M_FOS_INTERROGATOR_01", 1},
        {"MP_U_M_M_HCTEL_SD_TARGET_01", 1},
        {"MP_U_M_M_FOS_CORNWALLGUARD_01", 2},
        {"MP_A_M_M_FOS_COACHGUARDS_01", 15},
        {"MP_U_F_M_PROTECT_MERCER_01", 5},
        {"MP_U_M_M_DOCKRECIPIENTS_01", 2},
        {"MP_U_M_M_LOM_RHD_DEALERS_01", 3},
        {"MP_U_M_M_FOS_COACHHOLDUP_RECIPIENT_01", 6},
        {"MP_U_M_M_HCTEL_SD_TARGET_03", 6},
        {"MP_U_M_M_FOS_RAILWAY_HUNTER_01", 1},
    };

    void RenderPedSpawnerMenu()
    {
        ImGui::PushID("peds"_J);

        static auto model_hook = ([]() {
            NativeHooks::AddHook("long_update"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);
            NativeHooks::AddHook("long_update"_J, NativeIndex::_GET_META_PED_TYPE, _GET_META_PED_TYPE);
            return true;
        }());

        struct LastSpawnedPed {
            uint32_t modelHash;
            int variationIndex; 
        };

        static std::string pedModelBuffer;
        static std::string hashInputBuffer;
        static std::vector<std::string> recentHashes;
        static float scale = 1.0f;
        static bool dead = false;
        static bool invis = false;
        static bool godmode = false;
        static bool freeze = false;
        static bool companion = false;
        static bool sedated = false;
        static bool kill_em_all = false;
        static bool body_guard = false;
        static int formation = 0;
        static int selectedOutfitPreset = 0;
        static int maxOutfitPresets = 0;
        static std::vector<YimMenu::Ped> spawnedPeds;
        static std::vector<LastSpawnedPed> last10Spawned;

        float item_height = ImGui::GetTextLineHeightWithSpacing();
        float max_dropdown_height = (item_height * 10.0f) + (ImGui::GetStyle().WindowPadding.y * 2.0f);

        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_dropdown_height));
        if (ImGui::BeginCombo("Legendary Presets", "Select a Legendary Ped..."))
        {
            for (const auto& [modelKey, preset] : legendaryPresets)
            {
                std::string displayModel = modelKey;
                for (const auto& [hash, pedModel] : Data::g_PedModels)
                {
                    if (!pedModel) continue;
                    std::string pedModelLower = pedModel;
                    std::transform(pedModelLower.begin(), pedModelLower.end(), pedModelLower.begin(), ::tolower);
                    pedModelLower.erase(std::remove_if(pedModelLower.begin(), pedModelLower.end(), ::isspace), pedModelLower.end());
                    if (pedModelLower == modelKey)
                    {
                        displayModel = pedModel;
                        break;
                    }
                }

                if (ImGui::Selectable(displayModel.c_str()))
                {
                    pedModelBuffer = displayModel;
                    hashInputBuffer.clear();
                    selectedOutfitPreset = preset;
                }
            }
            ImGui::EndCombo();
        }

        InputTextWithHint("##pedmodel", "Ped Model", &pedModelBuffer, ImGuiInputTextFlags_CallbackCompletion, nullptr, PedSpawnerInputCallback).Draw();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Press Tab to auto fill or pick a Legendary Preset above.");

        bool is_input_active = ImGui::IsItemActive();
        if (!pedModelBuffer.empty() && !IsPedModelInList(pedModelBuffer))
        {
            if (is_input_active)
            {
                ImGui::OpenPopup("##ped_search_popup");
            }
        }

        ImVec2 input_pos = ImGui::GetItemRectMin();
        ImVec2 input_size = ImGui::GetItemRectSize();
        ImGui::SetNextWindowPos({input_pos.x, input_pos.y + input_size.y});
        ImGui::SetNextWindowSize({input_size.x, 0});
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_dropdown_height));

        if (ImGui::BeginPopup("##ped_search_popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
        {
            std::string bufferLower = pedModelBuffer;
            std::transform(bufferLower.begin(), bufferLower.end(), bufferLower.begin(), ::tolower);
            for (const auto& [hash, model] : Data::g_PedModels)
            {
                if (!model) continue;
                std::string pedModelLower = model;
                std::transform(pedModelLower.begin(), pedModelLower.end(), pedModelLower.begin(), ::tolower);
                if (pedModelLower.find(bufferLower) != std::string::npos)
                {
                    if (ImGui::Selectable(model))
                    {
                        pedModelBuffer = model;
                        hashInputBuffer.clear();
                        selectedOutfitPreset = 0;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            if (!is_input_active && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                 ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        const char* comboLabel = hashInputBuffer.empty() ? "Enter hash (e.g., 0x9770DD23)" : hashInputBuffer.c_str();
        if (false && ImGui::BeginCombo("Model Hash", comboLabel, 0))
        {
            static char hashInput[32] = "";
            ImGui::InputText("##hashinput", hashInput, sizeof(hashInput));
            if (ImGui::Button("Apply Hash"))
            {
                std::string input = hashInput;
                if (input.substr(0, 2) == "0x" || input.substr(0, 2) == "0X")
                {
                    try
                    {
                        Hash modelHash = std::stoul(input, nullptr, 16);
                        if (STREAMING::IS_MODEL_VALID(modelHash))
                        {
                            bool foundModel = false;
                            for (const auto& [hash, model] : Data::g_PedModels)
                            {
                                if (hash == modelHash)
                                {
                                    if (model) pedModelBuffer = model;
                                    else pedModelBuffer = input;
                                    foundModel = true;
                                    break;
                                }
                            }
                            if (!foundModel)
                            {
                                pedModelBuffer = input;
                            }
                            hashInputBuffer = input;
                            if (std::find(recentHashes.begin(), recentHashes.end(), input) == recentHashes.end())
                            {
                                recentHashes.push_back(input);
                                if (recentHashes.size() > 10)
                                    recentHashes.erase(recentHashes.begin());
                            }
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }

            for (const auto& hash : recentHashes)
            {
                if (ImGui::Selectable(hash.c_str()))
                {
                    hashInputBuffer = hash;
                    try
                    {
                        Hash modelHash = std::stoul(hash, nullptr, 16);
                        bool foundModel = false;
                        for (const auto& [h, model] : Data::g_PedModels)
                        {
                            if (h == modelHash)
                            {
                                if (model) pedModelBuffer = model;
                                else pedModelBuffer = hash;
                                foundModel = true;
                                break;
                            }
                        }
                        if (!foundModel)
                        {
                            pedModelBuffer = hash;
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Enter a model hash (e.g., 0x9770DD23) or select a recent hash.");

        std::string searchKey = pedModelBuffer;
        std::transform(searchKey.begin(), searchKey.end(), searchKey.begin(), ::tolower);
        searchKey.erase(std::remove_if(searchKey.begin(), searchKey.end(), ::isspace), searchKey.end());
        
        maxOutfitPresets = 0;
        bool foundVariation = false;
        if (!searchKey.empty())
        {
            for (const auto& [model, presets] : pedVariations)
            {
                std::string modelLower = model;
                std::transform(modelLower.begin(), modelLower.end(), modelLower.begin(), ::tolower);
                modelLower.erase(std::remove_if(modelLower.begin(), modelLower.end(), ::isspace), modelLower.end());
                if (modelLower == searchKey)
                {
                    foundVariation = true;
                    maxOutfitPresets = presets;
                    break;
                }
            }

            if (!foundVariation && legendaryPresets.count(searchKey))
            {
                foundVariation = true;
                maxOutfitPresets = 11;
            }
        }

        // Enhanced legendary detection logic
        bool is_legendary = false;
        auto it_legend = legendaryVariants.find(searchKey);
        if (it_legend != legendaryVariants.end()) {
            is_legendary = it_legend->second.count(selectedOutfitPreset) > 0;
        }

        if (foundVariation && maxOutfitPresets > 0)
        {
            ImGui::Text("Outfit Variation:");
            if (ImGui::SliderInt("##outfitSlider", &selectedOutfitPreset, 0, maxOutfitPresets > 1 ? maxOutfitPresets -1 : 0))
            {
                LOG("Selected outfit preset changed to: " << selectedOutfitPreset);
            }

            if (is_legendary)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "(Legendary)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("This is the legendary variant of the ped");
            }
            if (it_legend != legendaryVariants.end() && !it_legend->second.empty()) {
                std::string legendStr = "Legendary variants: ";
                for (auto idx : it_legend->second) legendStr += std::to_string(idx) + " ";
                ImGui::TextDisabled("%s", legendStr.c_str());
            }
        }

        ImGui::Checkbox("Spawn Dead", &dead);
        ImGui::Checkbox("Sedated", &sedated);
        ImGui::Checkbox("Invisible", &invis);
        ImGui::Checkbox("GodMode", &godmode);
        ImGui::Checkbox("Frozen", &freeze);
        ImGui::Checkbox("Companion", &companion);
        ImGui::Checkbox("Kill'em All", &kill_em_all);
        ImGui::Checkbox("BodyGuard", &body_guard);

        if (companion)
        {
            static const std::pair<int, const char*> groupFormations[] = {
                {0, "Default"},
                {1, "Circle"},
                {2, "Line"},
                {3, "Wedge"}
            };
            if (ImGui::BeginCombo("Formation", groupFormations[formation].second, 0))
            {
                for (const auto& [num, name] : groupFormations)
                {
                    bool is_selected = (formation == num);
                    if (ImGui::Selectable(name, is_selected))
                    {
                        formation = num;
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("What formation should spawned companion use?");
        }

        ImGui::SliderFloat("Scale", &scale, 0.1f, 10.0f);

        if (ImGui::Button("Spawn"))
        {
            FiberPool::Push([&, foundVariation, is_legendary] {
                Hash modelHash;
                if (!hashInputBuffer.empty() && hashInputBuffer.substr(0, 2) == "0x")
                {
                    try
                    {
                        modelHash = std::stoul(hashInputBuffer, nullptr, 16);
                        if (!STREAMING::IS_MODEL_VALID(modelHash))
                        {
                            LOG("Invalid model hash in input: " << hashInputBuffer);
                            return;
                        }
                    }
                    catch (...)
                    {
                        LOG("Error parsing hash input: " << hashInputBuffer);
                        return;
                    }
                }
                else
                {
                    modelHash = Joaat(pedModelBuffer);
                    if (!STREAMING::IS_MODEL_VALID(modelHash))
                    {
                        LOG("Invalid model hash for '" << pedModelBuffer << "': 0x" << std::hex << modelHash << std::dec);
                        return;
                    }
                }

                STREAMING::REQUEST_MODEL(modelHash, false);
                auto startTime = std::chrono::steady_clock::now();
                while (!STREAMING::HAS_MODEL_LOADED(modelHash))
                {
                    ScriptMgr::Yield();
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() > 5)
                    {
                        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
                        LOG("Model loading timeout for hash 0x" << std::hex << modelHash << std::dec);
                        return;
                    }
                }

                auto ped = Ped::Create(modelHash, Self::GetPed().GetPosition());
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);

                if (!ped)
                {
                    LOG("Failed to create ped for hash 0x" << std::hex << modelHash << std::dec);
                    return;
                }

                ped.SetFrozen(freeze);

                if (dead)
                {
                    ped.Kill();
                    if (ped.IsAnimal())
                    {
                        ped.SetQuality(3);
                    }
                }

                ped.SetInvincible(godmode);
                ped.SetVisible(!invis);

                if (scale != 1.0f)
                    ped.SetScale(scale);

                ped.SetConfigFlag(PedConfigFlag::IsTranquilized, sedated);
                
                if (foundVariation && selectedOutfitPreset >= 0 && selectedOutfitPreset < maxOutfitPresets)
                {
                    PED::_EQUIP_META_PED_OUTFIT_PRESET(ped.GetHandle(), selectedOutfitPreset, is_legendary);
                }

                if (body_guard)
                {
                    ApplyBodyGuardSettings(ped);
                }
                else if (kill_em_all)
                {
                    ApplyKillEmAllSettings(ped);
                }
                else if (companion)
                {
                    ApplyCompanionSettings(ped);
                    PED::SET_GROUP_FORMATION(PED::GET_PED_GROUP_INDEX(ped.GetHandle()), formation);
                    DECORATOR::DECOR_SET_INT(ped.GetHandle(), "SH_CMP_companion", 0);
                }

                spawnedPeds.push_back(ped);

                LastSpawnedPed last;
                last.modelHash = modelHash;
                last.variationIndex = foundVariation ? selectedOutfitPreset : -1;
                last10Spawned.push_back(last);
                if (last10Spawned.size() > 10)
                {
                    last10Spawned.erase(last10Spawned.begin());
                }
            });
        }

        ImGui::SameLine();
        if (ImGui::Button("Set Model"))
        {
            FiberPool::Push([&, foundVariation, is_legendary] {
                Hash modelHash;
                if (!hashInputBuffer.empty() && hashInputBuffer.substr(0, 2) == "0x")
                {
                    try
                    {
                        modelHash = std::stoul(hashInputBuffer, nullptr, 16);
                        if (!STREAMING::IS_MODEL_VALID(modelHash))
                        {
                             LOG("Invalid model hash for player: " << hashInputBuffer);
                            return;
                        }
                    }
                    catch (...)
                    {
                        LOG("Error parsing hash for player: " << hashInputBuffer);
                        return;
                    }
                }
                else
                {
                    modelHash = Joaat(pedModelBuffer);
                }

                for (int i = 0; i < 30 && !STREAMING::HAS_MODEL_LOADED(modelHash); i++)
                {
                    STREAMING::REQUEST_MODEL(modelHash, false);
                    ScriptMgr::Yield();
                }
                PLAYER::SET_PLAYER_MODEL(Self::GetPlayer().GetId(), modelHash, false);
                Self::Update();
                PED::_SET_RANDOM_OUTFIT_VARIATION(Self::GetPed().GetHandle(), true);
                
                if (foundVariation && selectedOutfitPreset >= 0 && selectedOutfitPreset < maxOutfitPresets)
                {
                    PED::_EQUIP_META_PED_OUTFIT_PRESET(Self::GetPed().GetHandle(), selectedOutfitPreset, is_legendary);
                }
                
                LastSpawnedPed last;
                last.modelHash = modelHash;
                last.variationIndex = foundVariation ? selectedOutfitPreset : -1;
                last10Spawned.push_back(last);
                if (last10Spawned.size() > 10)
                    last10Spawned.erase(last10Spawned.begin());
STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
            });
        }

        ImGui::SameLine();
        if (ImGui::Button("Cleanup Peds"))
        {
            FiberPool::Push([] {
                for (auto it = spawnedPeds.begin(); it != spawnedPeds.end();)
                {
                    if (it->IsValid())
                    {
                        if (it->GetMount())
                        {
                            it->GetMount().ForceControl();
                            it->GetMount().Delete();
                        }
                        it->ForceControl();
                        it->Delete();
                    }
                    it = spawnedPeds.erase(it);
                }
                LOG("Cleaned up all spawned peds");
            });
        }

        if (ImGui::CollapsingHeader("Last 10 Spawned Peds"))
        {
            for (auto it = last10Spawned.rbegin(); it != last10Spawned.rend(); ++it)
            {
                const auto& last = *it;
                std::string displayName;
                if (Data::g_PedModels.count(last.modelHash))
                {
                    const char* modelName = Data::g_PedModels.at(last.modelHash);
                    if (modelName)
                        displayName = modelName;
                    else
                    {
                        char hexBuffer[16];
                        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", last.modelHash);
                        displayName = hexBuffer;
                    }
                }
                else
                {
                    char hexBuffer[16];
                    snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", last.modelHash);
                    displayName = hexBuffer;
                }
                if (last.variationIndex != -1)
                {
                    displayName += " (Var: " + std::to_string(last.variationIndex) + ")";
                }
                if (ImGui::Button(displayName.c_str()))
                {
                    if (Data::g_PedModels.count(last.modelHash))
                    {
                         const char* modelName = Data::g_PedModels.at(last.modelHash);
                        if (modelName)
                        {
                            pedModelBuffer = modelName;
                            hashInputBuffer.clear();
                        }
                    }
                    else
                    {
                        char hexBuffer[16];
                        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", last.modelHash);
                        pedModelBuffer = hexBuffer;
                        hashInputBuffer = hexBuffer;
                    }
                    selectedOutfitPreset = last.variationIndex != -1 ? last.variationIndex : 0;
                }
            }
        }

        ImGui::PopID();
    }
}