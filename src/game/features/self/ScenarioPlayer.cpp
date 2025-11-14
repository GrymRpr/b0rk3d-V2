#include "ScenarioPlayer.hpp"

#include "game/backend/FiberPool.hpp"
#include "game/backend/MapEditor/Items/PedScenarios.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features::Self
{
    static int selectedScenarioIndex = 0;

    void PlaySelectedScenario()
    {
        const auto& scenario = YimMenu::g_PedScenarios[selectedScenarioIndex];
        std::uint32_t hash = scenario.m_Hash;

        FiberPool::Push([hash] {
            YimMenu::Ped ped = YimMenu::Self::GetPed();
            if (!ped.IsValid())
                return;

            TASK::CLEAR_PED_TASKS_IMMEDIATELY(ped.GetHandle(), true, true);
            TASK::TASK_START_SCENARIO_IN_PLACE_HASH(ped.GetHandle(), hash, -1, false, 0, -1, false);
        });
    }

    void StopScenario()
    {
        FiberPool::Push([] {
            YimMenu::Ped ped = YimMenu::Self::GetPed();
            if (!ped.IsValid())
                return;

            TASK::CLEAR_PED_TASKS_IMMEDIATELY(ped.GetHandle(), true, true);
        });
    }

    void RenderScenarioPlayer()
    {
        // Use static so we only build once
        static std::vector<const char*> scenarioNames;
        if (scenarioNames.empty())
        {
            scenarioNames.reserve(YimMenu::g_PedScenarios.size());
            for (const auto& scenario : YimMenu::g_PedScenarios)
            {
                // If m_FriendlyName is std::string
                // scenarioNames.push_back(scenario.m_FriendlyName.c_str());
                // If m_FriendlyName is const char*
                scenarioNames.push_back(scenario.m_FriendlyName);
            }
        }

        ImGui::Text("Select a scenario to play");
        ImGui::Combo("Scenario", &selectedScenarioIndex, scenarioNames.data(), static_cast<int>(scenarioNames.size()));

        if (ImGui::Button("Play"))
            PlaySelectedScenario();

        ImGui::SameLine();

        if (ImGui::Button("Stop"))
            StopScenario();
    }
}