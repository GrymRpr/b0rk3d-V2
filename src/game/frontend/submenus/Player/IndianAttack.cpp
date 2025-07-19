// Simplified IndianAttack.cpp - Spawns basic Indian peds that target the selected player

#include "core/commands/LoopedCommand.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/Self.hpp"
#include "game/backend/Players.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"

#include <array>
#include <random>

namespace YimMenu::Features
{
    static constexpr int max_indians_per_wave = 10;
    static constexpr float spawn_distance = 50.f;

    static std::vector<int> indian_index;

    static void CleanupIndians()
    {
        auto it = indian_index.begin();
        while (it != indian_index.end())
        {
            if (!ENTITY::DOES_ENTITY_EXIST(*it) || ENTITY::IS_ENTITY_DEAD(*it))
            {
                PED::DELETE_PED(&(*it));
                it = indian_index.erase(it);
            }
            else
                ++it;
        }
    }

    static void SpawnIndians(YimMenu::Player target_player)
    {
        int targetPed = target_player.GetPed().GetHandle();
        Vector3 targetPos = target_player.GetPed().GetPosition();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.f, 1.f);
        std::uniform_int_distribution<int> modelDist(0, 3);

        constexpr std::array<const char*, 4> indianModels = {
            "a_m_m_wapwarriors_01",
            "u_m_m_nbxindianowner_01",
            "a_f_m_wapnative_01",
            "u_m_m_bht_skinnersearch_01"
        };

        for (int i = 0; i < max_indians_per_wave; ++i)
        {
            float x = dist(gen) * spawn_distance;
            float y = dist(gen) * spawn_distance;
            Vector3 spawnPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(targetPed, x, y, 0.0f);

            const char* modelName = indianModels[modelDist(gen)];
            Hash modelHash = Joaat(modelName);
            
            if (!STREAMING::IS_MODEL_VALID(modelHash))
                continue;

            STREAMING::REQUEST_MODEL(modelHash, false);
            int tries = 0;
            while (!STREAMING::HAS_MODEL_LOADED(modelHash) && tries < 100)
            {
                ScriptMgr::Yield();
                ++tries;
            }

            if (!STREAMING::HAS_MODEL_LOADED(modelHash))
                continue;

            Ped indian = Ped::Create(modelHash, spawnPos, 0.f);

            if (!indian.IsValid())
                continue;

            int indianHandle = indian.GetHandle();

            TASK::TASK_COMBAT_PED(indianHandle, targetPed, 0, 16);
            indian_index.push_back(indianHandle);
        }
    }

    class SimpleIndianAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            YimMenu::Player target_player = YimMenu::Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                CleanupIndians();
                return;
            }

            if (indian_index.empty())
                SpawnIndians(target_player);

            CleanupIndians();
        }

        void OnDisable() override
        {
            CleanupIndians();
        }
    };

    static SimpleIndianAttack _SimpleIndianAttack{"indianattack", "Indian Attack", "Spawns waves of Indian peds that attack the selected player."};
}  //GrymsArchive