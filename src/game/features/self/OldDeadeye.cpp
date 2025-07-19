#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Entity.hpp"
#include <vector>
#include <cmath>

namespace YimMenu::Features
{
	class OldDeadeye : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		bool wasDeadeyeActive = false;

		virtual void OnTick() override
		{
			auto ped = Self::GetPed();
			if (!ped.IsValid())
				return;

			auto player = Self::GetPlayer();
			auto playerId = player.GetId();

			bool isDeadeyeActive = PLAYER::_IS_SPECIAL_ABILITY_ACTIVE(playerId);

			if (wasDeadeyeActive && !isDeadeyeActive)
			{
				MISC::SET_TIME_SCALE(1.0f);
				PLAYER::_MODIFY_INFINITE_TRAIL_VISION(playerId, false);
			}

			wasDeadeyeActive = isDeadeyeActive;

			if (!isDeadeyeActive)
				return;

			PLAYER::_SET_LOCAL_PLAYER_PERSONA_ABILITY_FLAG(PERSONA_DISABLE_DEADEYE_PERFECT_ACCURACY, false);

			if (_SlipperyBastardAccuracy.GetState())
			{
				Self::GetPed().SetAccuracy(100);
			}

			if (_MomentToRecuperate.GetState())
			{
				PLAYER::_SET_LOCAL_PLAYER_PERSONA_ABILITY_FLAG(ePersonaAbilityFlag::PERSONA_EXIT_DEADEYE_ON_TAKING_DAMAGE, false);
			}

			if (_DeadeyeTagging.GetState())
			{
				rage::fvector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
				rage::fvector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
				rage::fvector3 camDir = RotationToDirection(camRot);

				const float MaxScanDistance = 100.0f;
				const float AimFOV = 20.0f;

				int nearbyPedHandles[64];
				int numNearby = PED::GET_PED_NEARBY_PEDS(
					ped.GetHandle(),
					nearbyPedHandles,
					0, 
					0  
				);

				int maxPlayers = 32;
				int playerPeds[32];
				int playerPedCount = 0;
				for (int i = 0; i < maxPlayers; ++i)
				{
					int remotePlayerPed = PLAYER::GET_PLAYER_PED(i);
					if (remotePlayerPed != 0 && remotePlayerPed != ped.GetHandle())
					{
						playerPeds[playerPedCount++] = remotePlayerPed;
					}
				}

				struct TargetInfo
				{
					int handle;
					rage::fvector3 position;
				};
				std::vector<TargetInfo> allTargets;

				for (int i = 0; i < numNearby; ++i)
				{
					int handle = nearbyPedHandles[i];
					if (handle == ped.GetHandle())
						continue;
					if (!ENTITY::DOES_ENTITY_EXIST(handle))
						continue;
					allTargets.push_back({handle, ENTITY::GET_ENTITY_COORDS(handle, false, true)});
				}
				for (int i = 0; i < playerPedCount; ++i)
				{
					if (!ENTITY::DOES_ENTITY_EXIST(playerPeds[i]))
						continue;
					allTargets.push_back({playerPeds[i], ENTITY::GET_ENTITY_COORDS(playerPeds[i], false, true)});
				}

				int closestTargetHandle = 0;
				float closestDist = MaxScanDistance + 1.0f;

				for (const auto& target : allTargets)
				{
					if (ENTITY::IS_ENTITY_DEAD(target.handle))
						continue;
					if (!ENTITY::IS_ENTITY_ON_SCREEN(target.handle))
						continue;
					if (!ENTITY::IS_ENTITY_A_PED(target.handle))
						continue;

					rage::fvector3 toTarget = target.position - camPos;
					float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
					if (distance > MaxScanDistance)
						continue;

					float len = std::sqrt(toTarget.x*toTarget.x + toTarget.y*toTarget.y + toTarget.z*toTarget.z);
					if (len == 0.0f)
						continue;
					toTarget.x /= len;
					toTarget.y /= len;
					toTarget.z /= len;

					float dot = camDir.x * toTarget.x + camDir.y * toTarget.y + camDir.z * toTarget.z;
					float angle = std::acos(dot) * (180.0f / 3.14159265f);
					if (angle > AimFOV)
						continue;

					if (distance < closestDist)
					{
						closestDist = distance;
						closestTargetHandle = target.handle;
					}
				}

				if (closestTargetHandle != 0)
				{
					PLAYER::_SET_DEADEYE_TAGGING_CONFIG(playerId, 6);
					PLAYER::_SET_DEADEYE_TAGGING_ENABLED(playerId, true);
				}
				else
				{
					PLAYER::_SET_DEADEYE_TAGGING_ENABLED(playerId, true);
				}
			}

			if (_UnlockDeadeyeAbilities.GetState())
			{
				for (int abilityId = 1; abilityId <= 5; ++abilityId)
				{
					if (PLAYER::_IS_DEADEYE_ABILITY_LOCKED(playerId, abilityId))
					{
						PLAYER::_SET_DEADEYE_ABILITY_LOCKED(playerId, abilityId, false);
					}
				}
			}

			if (_EnhancedDeadeye.GetState())
			{
				MISC::SET_TIME_SCALE(0.3f);

				if (_SlowDeadeyeDrain.GetState())
				{
					PLAYER::_MODIFY_INFINITE_TRAIL_VISION(playerId, true);
				}
			}
		}

		virtual void OnDisable() override
		{
			auto ped = Self::GetPed();
			if (!ped.IsValid())
				return;

			auto playerId = Self::GetPlayer().GetId();

			PLAYER::_SET_LOCAL_PLAYER_PERSONA_ABILITY_FLAG(PERSONA_DISABLE_DEADEYE_PERFECT_ACCURACY, true);
			PLAYER::_SET_LOCAL_PLAYER_PERSONA_ABILITY_FLAG(ePersonaAbilityFlag::PERSONA_EXIT_DEADEYE_ON_TAKING_DAMAGE, true);
			PLAYER::_SET_DEADEYE_TAGGING_ENABLED(playerId, false);
			MISC::SET_TIME_SCALE(1.0f);
			PLAYER::_MODIFY_INFINITE_TRAIL_VISION(playerId, false);

			wasDeadeyeActive = false;
		}

	private:
		static rage::fvector3 RotationToDirection(const rage::fvector3& rot)
		{
			float radZ = rot.z * 0.0174532924f;
			float radX = rot.x * 0.0174532924f;
			float cosX = std::cos(radX);
			return rage::fvector3{
				-std::sin(radZ) * cosX,
				std::cos(radZ) * cosX,
				std::sin(radX)
			};
		}

		BoolCommand _DeadeyeTagging{"deadeyetagging", "Deadeye Auto-Tagging", "Automatically tags enemies and animals in Deadeye", false};
		BoolCommand _UnlockDeadeyeAbilities{"unlockdeadeyeabilities", "Unlock All Deadeye Abilities", "Unlocks all Deadeye abilities regardless of progression", false};
		BoolCommand _EnhancedDeadeye{"enhanceddeadeye", "Enhanced Deadeye", "Adds cinematic time dilation and visual effects", false};
		BoolCommand _SlowDeadeyeDrain{"slowdeadeyedrain", "Slow Deadeye Drain", "Significantly reduces Deadeye drain rate", false};
		BoolCommand _SlipperyBastardAccuracy{"slipperybastardaccuracy", "Slippery Bastard Accuracy", "Ensures perfect accuracy with Slippery Bastard", false};
		BoolCommand _MomentToRecuperate{"momenttorecuperate", "A Moment to Recuperate", "Prevents exiting Deadeye when taking damage", false};
	};

	static OldDeadeye _OldDeadeye{"olddeadeye", "Old Deadeye", "Enhanced Deadeye experience with perfect accuracy and configurable features"};
}  //Praveshan