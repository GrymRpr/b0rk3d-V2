#include "core/commands/BoolCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/hooking/DetourHook.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "game/frontend/ChatDisplay.hpp"
#include "game/hooks/Hooks.hpp"
#include "game/backend/PlayerDatabase.hpp"

#include <player/CPlayerInfo.hpp>
#include <network/CNetGamePlayer.hpp>

namespace YimMenu::Features
{
    BoolCommand _DetectSpoofedNames{"detectspoofednames", "Detect Spoofed Names", "Detects If a Player is Possibly Spoofing Their Name"};
    extern BoolCommand _DetectAdmins; // Defined in HandlePresenceEvent.cpp
}

namespace YimMenu::Hooks
{
    void Info::PlayerHasJoined(CNetGamePlayer* player)
    {
        BaseHook::Get<Info::PlayerHasJoined, DetourHook<decltype(&Info::PlayerHasJoined)>>()->Original()(player);

        if (g_Running && player && player->m_PlayerInfo)
        {
            Players::OnPlayerJoin(player);
            uint64_t rid = player->m_PlayerInfo->m_GamerInfo.m_GamerHandle2.m_RockstarId;
            const char* name = player->GetName();
            netAddress ipaddr = player->m_PlayerInfo->m_GamerInfo.m_ExternalAddress;
            std::string ip_str = std::format("{}.{}.{}.{}", ipaddr.m_field1, ipaddr.m_field2, ipaddr.m_field3, ipaddr.m_field4);

            // Restore full logging
            LOG(INFO) << std::format("{} joined the session. Reserved slot #{}. RID: {} | IP: {}",
                name,
                (int)player->m_PlayerIndex,
                (int)rid,
                ip_str);

            // Database update
            if (g_PlayerDatabase && rid != 0)
            {
                auto db_player = g_PlayerDatabase->GetOrCreatePlayer(rid, name);
            }

            // Admin detection
            if (Features::_DetectAdmins.GetState() && g_PlayerDatabase)
            {
                auto db_player = g_PlayerDatabase->GetPlayer(rid);
                if (db_player && db_player->is_admin)
                {
                    Notifications::Show("Network", std::format("Rockstar Admin {} joined!", name), NotificationType::Warning);
                }
            }
        }
    }

    void Info::PlayerHasLeft(CNetGamePlayer* player)
    {
        BaseHook::Get<Info::PlayerHasLeft, DetourHook<decltype(&Info::PlayerHasLeft)>>()->Original()(player);

        if (g_Running && player && player->m_PlayerInfo) // Added m_PlayerInfo check for consistency
        {
            if (player == Self::GetPlayer().GetHandle())
            {
                ChatDisplay::Clear();
            }
            Players::OnPlayerLeave(player);
            uint64_t rid = player->m_PlayerInfo->m_GamerInfo.m_GamerHandle2.m_RockstarId;
            const char* name = player->GetName();
            netAddress ipaddr = player->m_PlayerInfo->m_GamerInfo.m_ExternalAddress;
            std::string ip_str = std::format("{}.{}.{}.{}", ipaddr.m_field1, ipaddr.m_field2, ipaddr.m_field3, ipaddr.m_field4);

            // Restore full logging
            LOG(INFO) << std::format("{} left the session. Freeing slot #{}. RID: {} | IP: {}",
                name,
                (int)player->m_PlayerIndex,
                (int)rid,
                ip_str);
        }
    }
}