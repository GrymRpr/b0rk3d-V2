#include "core/commands/BoolCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/hooking/DetourHook.hpp"
#include "game/hooks/Hooks.hpp"
#include "game/rdr/Enums.hpp"
#include "util/Joaat.hpp"
#include "game/backend/PlayerDatabase.hpp"

#include <network/rlGamerInfo.hpp>
#include <nlohmann/json.hpp>

namespace YimMenu::Features
{
    BoolCommand _LogPresenceEvents{"logpresenceevents", "Log Presence Events", "Logs Presence Events"};

    BoolCommand _DetectAdmins{"detectadmins", "Detect Admins", "Detects if a Rockstar Admin joins the session"};
}

namespace YimMenu::Hooks
{
    void Protections::HandlePresenceEvent(int localGamerIndex, __int64 data, __int64 source)
    {
        const auto payload = *(char**)(data + 8);

        if (YimMenu::Features::_LogPresenceEvents.GetState())
        {
            LOG(VERBOSE) << "HandlePresenceEvent :: " << payload;
        }

        BaseHook::Get<Protections::HandlePresenceEvent, DetourHook<decltype(&Protections::HandlePresenceEvent)>>()->Original()(localGamerIndex, data, source);

        if (!YimMenu::Features::_DetectAdmins.GetState())
            return;

        nlohmann::json json;
        try
        {
            json = nlohmann::json::parse(payload);
        }
        catch (const std::exception&)
        {
            return;
        }

        const char* key = "gm.evt";
        if (json[key].is_null())
        {
            if (json["gta5.game.event"].is_null())
                return;
            key = "gta5.game.event";
        }

        nlohmann::json& event_payload = json[key];
        std::string sender_str = "Unknown";
        uint64_t sender_rid = 0;

        if (!event_payload["d"].is_null() && !event_payload["d"]["n"].is_null())
            sender_str = event_payload["d"]["n"].get<std::string>();
        if (!event_payload["d"].is_null() && !event_payload["d"]["r"].is_null())
            sender_rid = event_payload["d"]["r"].get<uint64_t>();

        uint32_t presence_hash = Joaat(event_payload["e"].get<std::string_view>());

        if (presence_hash == static_cast<uint32_t>(PresenceEvents::PRESENCE_ADMIN_JOIN_EVENT))
        {
            Notifications::Show("Network", std::format("Rockstar Admin {} is joining!", sender_str), NotificationType::Warning);
            if (sender_rid != 0 && g_PlayerDatabase)
            {
                auto player = g_PlayerDatabase->GetOrCreatePlayer(sender_rid, sender_str);
                if (player && !player->is_admin)
                {
                    player->is_admin = true;
                    g_PlayerDatabase->Save();
                }
            }
        }
    }
}