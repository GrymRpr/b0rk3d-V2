#include "Toxic.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    std::shared_ptr<Category> BuildToxicMenu()
    {
        auto menu = std::make_shared<Category>("Toxic");

        // General section
        auto general = std::make_shared<Group>("General");
        general->AddItem(std::make_shared<PlayerCommandItem>("defensive"_J));
        general->AddItem(std::make_shared<PlayerCommandItem>("offensive"_J));
        general->AddItem(std::make_shared<PlayerCommandItem>("remotebolas"_J));

        // Remote Bolas Configuration section
        auto remoteBolasConfig = std::make_shared<Group>("Remote Bolas Settings");
        
        // Striker visibility toggle
        remoteBolasConfig->AddItem(std::make_shared<BoolCommandItem>("remotebolas_show_striker"_J, "Show Striker Ped"));
        
        // Use random direction toggle
        remoteBolasConfig->AddItem(std::make_shared<BoolCommandItem>("remotebolas_random_direction"_J, "Random Strike Direction"));
        
        // REMOVED: Sound toggle has been taken out as requested.
        
        // Striker distance slider
        remoteBolasConfig->AddItem(std::make_shared<FloatCommandItem>("remotebolas_striker_distance"_J, "Striker Distance"));
        
        // Damage slider
        remoteBolasConfig->AddItem(std::make_shared<IntCommandItem>("remotebolas_damage"_J, "Bolas Damage"));
        
        // Bolas type dropdown
        remoteBolasConfig->AddItem(std::make_shared<ListCommandItem>("remotebolas_type"_J, "Bolas Type"));

        // Events section
        auto events = std::make_shared<Group>("Events");
        events->AddItem(std::make_shared<PlayerCommandItem>("maxhonor"_J));
        events->AddItem(std::make_shared<PlayerCommandItem>("minhonor"_J));
        events->AddItem(std::make_shared<PlayerCommandItem>("startparlay"_J));
        events->AddItem(std::make_shared<PlayerCommandItem>("endparlay"_J));
        events->AddItem(std::make_shared<PlayerCommandItem>("increasebounty"_J));
        events->AddItem(std::make_shared<PlayerCommandItem>("sendticker"_J));
        events->AddItem(std::make_shared<ListCommandItem>("tickermessage"_J, "Message"));
        events->AddItem(std::make_shared<PlayerCommandItem>("sendstablemountevent"_J));
        events->AddItem(std::make_shared<ListCommandItem>("mountinstance"_J, "Instance"));
        events->AddItem(std::make_shared<ListCommandItem>("stablemountevent"_J, "Event"));

        // Mount section
        auto mount = std::make_shared<Group>("Mount");
        mount->AddItem(std::make_shared<PlayerCommandItem>("kickhorse"_J));
        mount->AddItem(std::make_shared<PlayerCommandItem>("deletehorse"_J));

        // Vehicle section
        auto vehicle = std::make_shared<Group>("Vehicle");
        vehicle->AddItem(std::make_shared<PlayerCommandItem>("deletevehicle"_J));

        // Add all groups to the menu
        menu->AddItem(general);
        menu->AddItem(remoteBolasConfig);
        menu->AddItem(events);
        menu->AddItem(mount);
        menu->AddItem(vehicle);

        return menu;
    }
}