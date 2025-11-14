#include "core/commands/BoolCommand.hpp"
#include "core/hooking/DetourHook.hpp"
#include "game/backend/Self.hpp"
#include "game/hooks/Hooks.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Pools.hpp"
#include "util/Joaat.hpp"

#include <entity/fwEntity.hpp>
#include <network/netObject.hpp>

namespace YimMenu::Features
{
	BoolCommand _BetterEntityCheck("betterentitycheck", "Better Entity Check (BETA)", "Attempts to fix several entity spawning bugs. This is still experimental, use with caution", true);
}

namespace YimMenu::Hooks
{
	static void DeleteAmbientObjects(rage::fwBasePool* pool, int to_delete)
	{
		// Temporary container for ambient objects to be deleted.
		std::vector<decltype(*PoolUtils(pool).begin())> objectsToDelete;
		for (auto entity : PoolUtils(pool))
		{
			if (entity.IsNetworked() && !entity.IsPlayer() && !entity.IsMissionEntity())
			{
				objectsToDelete.push_back(entity);
				if ((int)objectsToDelete.size() >= to_delete)
					break;
			}
		}
		// Now delete the collected objects.
		for (auto entity : objectsToDelete)
		{
			entity.Delete();
		}
	}

	bool Protections::CanCreateNetworkObject(NetObjType type, int count, bool mission, bool a4, bool a5)
	{
		using OriginalFnType = decltype(&Protections::CanCreateNetworkObject);
		OriginalFnType originalFn = BaseHook::Get<Protections::CanCreateNetworkObject, DetourHook<OriginalFnType>>()->Original();

		if (Features::_BetterEntityCheck.GetState())
		{
			bool is_ped = type == NetObjType::Ped || type == NetObjType::Horse || type == NetObjType::Animal;
			bool is_veh = type == NetObjType::Boat || type == NetObjType::DraftVeh || type == NetObjType::Automobile || type == NetObjType::Heli || type == NetObjType::Submarine;
			bool is_obj = type == NetObjType::Object; // TODO: add more object types

			rage::fwBasePool* pool = nullptr;

			if (is_ped)
				pool = GetPedPool();
			else if (is_veh)
				pool = GetVehiclePool();
			else if (is_obj)
				pool = GetObjectPool();

			if (pool)
			{
				bool can_create_object = originalFn(type, count, mission, a4, a5) && pool->GetNumFreeSlots() >= count;

				if (can_create_object)
					return true;

				// our pools are full

				if (!mission)
					return false; // we're trying to create an ambient ped, and failures are totally OK

				LOGF(GAME, INFO, "CanCreateNetworkObject: Freeing objects for mission entity");

				// try deleting some objects
				DeleteAmbientObjects(pool, count);

				// recalculate
				can_create_object = originalFn(type, count, mission, a4, a5) && pool->GetNumFreeSlots() >= count;

				if (!can_create_object)
				{
					LOGF(GAME, WARNING, "CanCreateNetworkObject: Failed to clear space for mission entity");
					return false; // don't call original here since it might return true and crash the game
				}
			}
		}

		if (!originalFn(type, count, mission, a4, a5))
			return false;

		return true;
	}
}