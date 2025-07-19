#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class SuperJump : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        virtual void OnTick() override
        {
            if (Self::GetPlayer().IsValid())
            {
                MISC::SET_SUPER_JUMP_THIS_FRAME(Self::GetPlayer().GetId());
            }
        }
    };

    static SuperJump _SuperJump{"superjump", "Super Jump", "Jump higher than normal"};
} 