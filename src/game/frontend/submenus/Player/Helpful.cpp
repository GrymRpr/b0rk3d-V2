#include "Helpful.hpp"

#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Vehicle.hpp"
#include "util/Rewards.hpp"

namespace YimMenu::Submenus
{
	/*struct sGuid
	{
		alignas(8) int data1;
		alignas(8) int data2;
		alignas(8) int data3;
		alignas(8) int data4;
	};

	struct sSlotInfo
	{
		alignas(8) sGuid guid;
		alignas(8) int f_1;
		alignas(8) int f_2;
		alignas(8) int f_3;
		alignas(8) int slotId;
	};*/

	inline const joaat_t flowers[]{"COMPOSITE_LOOTABLE_AGARITA_DEF"_J, "COMPOSITE_LOOTABLE_BITTERWEED_DEF"_J, "COMPOSITE_LOOTABLE_BLOODFLOWER_DEF"_J, "COMPOSITE_LOOTABLE_CARDINAL_FLOWER_DEF"_J, "COMPOSITE_LOOTABLE_CHOC_DAISY_DEF"_J, "COMPOSITE_LOOTABLE_CREEKPLUM_DEF"_J, "COMPOSITE_LOOTABLE_TEXAS_BONNET_DEF"_J, "COMPOSITE_LOOTABLE_WILD_RHUBARB_DEF"_J, "COMPOSITE_LOOTABLE_WISTERIA_DEF"_J};

	void SpawnReqestedReward(const Rewards::RewardInfo& info)
	{
		FiberPool::Push([=] {
			if (!STREAMING::HAS_MODEL_LOADED(info.model_hash))
			{
				STREAMING::REQUEST_MODEL(info.model_hash, false);
				ScriptMgr::Yield();
			}

			auto forward = ENTITY::GET_ENTITY_FORWARD_VECTOR(Players::GetSelected().GetPed().GetHandle());
			auto pos = Players::GetSelected().GetPed().GetPosition();
			auto heading = ENTITY::GET_ENTITY_HEADING(Players::GetSelected().GetPed().GetHandle());
			float distance = 1.75f;
			float x = pos.x + (forward.x * distance);
			float y = pos.y + (forward.y * distance);
			float z = pos.z;

			Object obj = OBJECT::CREATE_OBJECT(info.model_hash, x, y, z + 1, true, false, true, false, false);
			OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(obj, true);
			ScriptMgr::Yield();

			ENTITY::SET_ENTITY_VISIBLE(obj, true);
			ScriptMgr::Yield();
			OBJECT::_MAKE_ITEM_CARRIABLE(obj);

			NETWORK::NETWORK_REGISTER_ENTITY_AS_NETWORKED(obj);
			if (NETWORK::NETWORK_DOES_NETWORK_ID_EXIST(NETWORK::OBJ_TO_NET(obj)))
			{
				ENTITY::SET_ENTITY_SHOULD_FREEZE_WAITING_ON_COLLISION(obj, true);
				NETWORK::SET_NETWORK_ID_EXISTS_ON_ALL_MACHINES(NETWORK::OBJ_TO_NET(obj), true);
				NETWORK::NETWORK_REQUEST_CONTROL_OF_ENTITY(obj);
			}
			ScriptMgr::Yield();

			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(info.model_hash);
			ENTITY::SET_OBJECT_AS_NO_LONGER_NEEDED(&obj);
		});
	}

	void SpawnReqestedRewards(std::vector<Rewards::eRewardType> rewards) // we could spawn a chest but, it's not required
	{
		for (auto& reward : rewards)
		{
			switch (reward)
			{
			case Rewards::eRewardType::HEIRLOOMS:
				for (const auto& heirloom : Rewards::Heirlooms)
				{
					SpawnReqestedReward(heirloom);
				}
				break;
			case Rewards::eRewardType::COINS:
				for (const auto& coin : Rewards::Coins)
				{
					SpawnReqestedReward(coin);
				}
				break;
			case Rewards::eRewardType::ALCBOTTLES:
				for (const auto& alc : Rewards::AlcBottles)
				{
					SpawnReqestedReward(alc);
				}
				break;

			case Rewards::eRewardType::ARROWHEADS:
				for (const auto& arrowhead : Rewards::Arrowheads)
				{
					SpawnReqestedReward(arrowhead);
				}
				break;
			case Rewards::eRewardType::BRACELETS:
				for (const auto& bracelet : Rewards::Bracelets)
				{
					SpawnReqestedReward(bracelet);
				}
				break;
			case Rewards::eRewardType::EARRINGS:
				for (const auto& earring : Rewards::Earrings)
				{
					SpawnReqestedReward(earring);
				}
				break;
			case Rewards::eRewardType::NECKLACES:
				for (const auto& necklace : Rewards::Necklaces)
				{
					SpawnReqestedReward(necklace);
				}
				break;
			case Rewards::eRewardType::RINGS:
				for (const auto& ring : Rewards::Rings)
				{
					SpawnReqestedReward(ring);
				}
				break;
			case Rewards::eRewardType::TAROTCARDS_CUPS:
				for (const auto& card : Rewards::TarotCards_Cups)
				{
					SpawnReqestedReward(card);
				}
				break;
			case Rewards::eRewardType::TAROTCARDS_PENTACLES:
				for (const auto& card : Rewards::TarotCards_Pentacles)
				{
					SpawnReqestedReward(card);
				}
				break;
			case Rewards::eRewardType::TAROTCARDS_SWORDS:
				for (const auto& card : Rewards::TarotCards_Swords)
				{
					SpawnReqestedReward(card);
				}
				break;
			case Rewards::eRewardType::TAROTCARDS_WANDS:
				for (const auto& card : Rewards::TarotCards_Wands)
				{
					SpawnReqestedReward(card);
				}
				break;
			//case Rewards::eRewardType::EGGS: // doesnt work correctly do not add
			//	for (const auto& egg : Rewards::Eggs)
			//	{
			//		SpawnReqestedReward(egg);
			//	}
			//	break;
			case Rewards::eRewardType::FLOWERS: // if we spawn too many, no more will spawn in that area, we could keep a list and delete them or use a cleanup button or move to a new area
				for (const auto& flowerComposite : flowers)
				{
					auto forward = ENTITY::GET_ENTITY_FORWARD_VECTOR(Players::GetSelected().GetPed().GetHandle());
					auto pos = Players::GetSelected().GetPed().GetPosition();
					float distance = 1.75f;
					float x = pos.x + (forward.x * distance);
					float y = pos.y + (forward.y * distance);
					float z = pos.z;
					TASK::_REQUEST_HERB_COMPOSITE_ASSET(flowerComposite);
					for (uint8_t i = 0; !TASK::ARE_COMPOSITE_LOOTABLE_ENTITY_DEF_ASSETS_LOADED(flowerComposite) && i < 500; i++)
					{
						ScriptMgr::Yield();
					}
					if (TASK::ARE_COMPOSITE_LOOTABLE_ENTITY_DEF_ASSETS_LOADED(flowerComposite))
					{
						//sSlotInfo ptr{};
						alignas(8) char bruh[64];
						TASK::_CREATE_HERB_COMPOSITES(flowerComposite, x, y, z, 0.0f, 0, &bruh, -1);
					}
				}
				break;
			}
		}
	}


	std::shared_ptr<Category> BuildHelpfulMenu()
	{
		auto menu = std::make_shared<Category>("Helpful");

		auto spawnCollectiblesGroup = std::make_shared<Group>("Spawn Collectibles");

		spawnCollectiblesGroup->AddItem(std::make_shared<ImGuiItem>([=] {
			// Sketch stuff
			static Rewards::eRewardType selected{};
			std::map<Rewards::eRewardType, std::string> reward_category_translations = {
			    {Rewards::eRewardType::HEIRLOOMS, "Heirlooms"},
			    {Rewards::eRewardType::COINS, "Coins"},
			    {Rewards::eRewardType::ALCBOTTLES, "Alcohol Bottles"},
			    {Rewards::eRewardType::ARROWHEADS, "Arrowheads"},
			    {Rewards::eRewardType::BRACELETS, "Bracelets"},
			    {Rewards::eRewardType::EARRINGS, "Earrings"},
			    {Rewards::eRewardType::NECKLACES, "Necklaces"},
			    {Rewards::eRewardType::RINGS, "Rings"},
			    {Rewards::eRewardType::TAROTCARDS_CUPS, "Tarot Cards - Cups"},
			    {Rewards::eRewardType::TAROTCARDS_PENTACLES, "Tarot Cards - Pentacles"},
			    {Rewards::eRewardType::TAROTCARDS_SWORDS, "Tarot Cards - Swords"},
			    {Rewards::eRewardType::TAROTCARDS_WANDS, "Tarot Cards - Wands"},
			    {Rewards::eRewardType::FLOWERS, "Wild Flower Collection"},
			    /* {Rewards::eRewardType::EGGS, "Eggs"},*/
			}; // pasted this map from recovery.cpp maybe we can access it another way?

			if (ImGui::BeginCombo("Rewards", reward_category_translations[selected].c_str()))
			{
				for (auto& [type, translation] : reward_category_translations)
				{
					if (ImGui::Selectable(std::string(translation).c_str(), type == selected, ImGuiSelectableFlags_AllowDoubleClick))
					{
						selected = type;
					}
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						FiberPool::Push([=] {
							SpawnReqestedRewards({selected});
						});
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::Button("Spawn Selected"))
			{
				FiberPool::Push([] {
					SpawnReqestedRewards({selected});
				});
			}
		}));

		menu->AddItem(std::make_shared<PlayerCommandItem>("spawngoldchest"_J));
		menu->AddItem(std::make_shared<ImGuiItem>([] {
			// TODO: move, refactor, or remove these
			if (ImGui::Button("Spawn Bounty Wagon for Player"))
			{
				FiberPool::Push([] {
					Vector3 coords = ENTITY::GET_ENTITY_COORDS(Players::GetSelected().GetPed().GetHandle(), true, true);
					float rot = ENTITY::GET_ENTITY_ROTATION(Players::GetSelected().GetPed().GetHandle(), 0).z;
					Vehicle::Create("wagonarmoured01x"_J, coords, rot);
					Notifications::Show("Spawned Wagon", "Spawned Bounty Wagon for Player", NotificationType::Success);
				});
			};

			if (ImGui::Button("Spawn Hunting Wagon for Player"))
			{
				FiberPool::Push([] {
					int id = Players::GetSelected().GetId();
					auto ped = PLAYER::GET_PLAYER_PED_SCRIPT_INDEX(id);
					Vector3 dim1, dim2;
					MISC::GET_MODEL_DIMENSIONS(MISC::GET_HASH_KEY("huntercart01"), &dim1, &dim2);
					float offset = dim2.y * 1.6;

					Vector3 dir = ENTITY::GET_ENTITY_FORWARD_VECTOR(ped);
					float rot = (ENTITY::GET_ENTITY_ROTATION(ped, 0)).z;
					Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, true, true);

					Vehicle::Create("huntercart01"_J,
					    Vector3{pos.x + (dir.x * offset), pos.y + (dir.y * offset), pos.z},
					    ENTITY::GET_ENTITY_ROTATION(ped, 0).z);
					Notifications::Show("Spawned Wagon", "Spawned Hunting Wagon for Player", NotificationType::Success);
				});
			}
		}));

		menu->AddItem(spawnCollectiblesGroup);

		return menu;
	}
}  // ALL Collectable code: Praveshan