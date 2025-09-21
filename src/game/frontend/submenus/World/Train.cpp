#include "Train.hpp"

#include "game/backend/Self.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/data/Trains.hpp"
#include "util/Joaat.hpp"
#include "util/teleport.hpp"

namespace YimMenu::Submenus
{
	static int currentTrainHandle = 0;

	void RenderTrainsMenu()
	{
		static uint32_t selectedTrain = 0;
		static bool hasPax = false;
		static bool hasConductor = false;
		static bool warpOnSpawn = true;
		static bool stopsForStations = false;
		static int direction = 0;
		static bool fastTrain = false;

		ImGui::PushID("trains"_J);
		if (ImGui::BeginCombo("##Preset Model", "Train Model"))
		{
			for (size_t i = 0; i < std::size(Data::g_Trains); ++i)
			{
				const auto& train = Data::g_Trains[i];
				if (ImGui::Selectable(train.name.c_str(), train.model == selectedTrain))
				{
					selectedTrain = train.model;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Checkbox("Passengers", &hasPax);
		ImGui::SameLine();

		ImGui::Checkbox("Conductor", &hasConductor);
		ImGui::SameLine();

		ImGui::Checkbox("Warp Into Seat", &warpOnSpawn);
		ImGui::SameLine();

		ImGui::Checkbox("Station Stops", &stopsForStations);

		ImGui::Separator();

		ImGui::Text("Direction:");
		ImGui::SameLine();
		ImGui::RadioButton("Forward", &direction, 0); ImGui::SameLine();
		ImGui::RadioButton("Backward", &direction, 1);

		ImGui::Separator();

		ImGui::Checkbox("Fast Train", &fastTrain);

		if (ImGui::Button("Spawn"))
		{
			if (selectedTrain != 0)
			{
				float spawnSpeed = fastTrain ? 60.0f : 20.0f;

				FiberPool::Push([=] {
					auto selfCoords = Self::GetPed().GetPosition();
					auto coords = VEHICLE::_GET_NEAREST_TRAIN_TRACK_POSITION(selfCoords.x, selfCoords.y, selfCoords.z);

					auto numcars = VEHICLE::_GET_NUM_CARS_FROM_TRAIN_CONFIG(selectedTrain);

					for (int i = 0; i <= numcars - 1; i++)
					{
						STREAMING::REQUEST_MODEL(VEHICLE::_GET_TRAIN_MODEL_FROM_TRAIN_CONFIG_BY_CAR_INDEX(selectedTrain, i), false);

						while (!STREAMING::HAS_MODEL_LOADED(VEHICLE::_GET_TRAIN_MODEL_FROM_TRAIN_CONFIG_BY_CAR_INDEX(selectedTrain, i)))
						{
							STREAMING::REQUEST_MODEL(VEHICLE::_GET_TRAIN_MODEL_FROM_TRAIN_CONFIG_BY_CAR_INDEX(selectedTrain, i), false);
							ScriptMgr::Yield();
						}
					}

					auto veh = VEHICLE::_CREATE_MISSION_TRAIN(selectedTrain, coords.x, coords.y, coords.z, direction, hasPax, true, hasConductor);

					currentTrainHandle = veh;

					VEHICLE::_0x06A09A6E0C6D2A84(veh, false);
					VEHICLE::_SET_TRAIN_STOPS_FOR_STATIONS(veh, stopsForStations);
					NETWORK::SET_NETWORK_ID_EXISTS_ON_ALL_MACHINES(NETWORK::VEH_TO_NET(veh), true);

					for (int i = 0; i <= numcars - 1; i++)
					{
						auto car = VEHICLE::GET_TRAIN_CARRIAGE(veh, i);
						VEHICLE::_0x762FDC4C19E5A981(car, true);
					}

					if (fastTrain)
					{
						FiberPool::Push([trainHandle = veh]()
						{
							const float speed = 60.0f;
							while (ENTITY::DOES_ENTITY_EXIST(trainHandle))
							{
								VEHICLE::SET_TRAIN_SPEED(trainHandle, speed);
								VEHICLE::SET_TRAIN_CRUISE_SPEED(trainHandle, speed);
								ScriptMgr::Yield(std::chrono::milliseconds(100));
							}
						});
					}

					if (warpOnSpawn)
					{
						Teleport::WarpIntoVehicle(Self::GetPed().GetHandle(), veh);
					}
				});
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Delete Train"))
		{
			if (currentTrainHandle && ENTITY::DOES_ENTITY_EXIST(currentTrainHandle))
			{
				FiberPool::Push([] {
					int trainDriver = VEHICLE::GET_DRIVER_OF_VEHICLE(currentTrainHandle);

					if (trainDriver && trainDriver != 0 && trainDriver != Self::GetPed().GetHandle())
					{
						if (!ENTITY::IS_ENTITY_A_MISSION_ENTITY(trainDriver)) {
							ENTITY::SET_ENTITY_AS_MISSION_ENTITY(trainDriver, true, true);
						}
						TASK::CLEAR_PED_TASKS_IMMEDIATELY(trainDriver, false, true);
						PED::DELETE_PED(&trainDriver);
					}

					if (!ENTITY::IS_ENTITY_A_MISSION_ENTITY(currentTrainHandle)) {
						ENTITY::SET_ENTITY_AS_MISSION_ENTITY(currentTrainHandle, true, true);
					}

					VEHICLE::DELETE_MISSION_TRAIN(&currentTrainHandle);
					currentTrainHandle = 0;
				});
			}
		}

		ImGui::PopID();
	}
}