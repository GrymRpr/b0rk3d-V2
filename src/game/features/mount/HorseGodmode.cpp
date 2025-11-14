#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
    class HorseGodmode : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;
        Ped* m_LastMount; // Pointer to avoid default construction

    public:
        HorseGodmode() : LoopedCommand("horsegodmode", "Godmode", "Blocks all incoming damage for your horse"), m_LastMount(nullptr)
        {
        }

        virtual void OnEnable() override
        {
            if (auto mount = Self::GetMount())
            {
                m_LastMount = &mount;
                ApplyGodmode(mount);
            }
        }

        virtual void OnTick() override
        {
            auto currentMount = Self::GetMount();
            if (currentMount && (!m_LastMount || currentMount != *m_LastMount))
            {
                m_LastMount = &currentMount; // Fixed: Assign pointer correctly
                ApplyGodmode(currentMount);
            }
            else if (!currentMount && m_LastMount && m_LastMount->IsValid())
            {
                ApplyGodmode(*m_LastMount); // Protect last mount if still valid
            }
        }

        virtual void OnDisable() override
        {
            if (m_LastMount && m_LastMount->IsValid())
            {
                m_LastMount->SetInvincible(false);
                ENTITY::SET_ENTITY_PROOFS(m_LastMount->GetHandle(), 0, false); // Disable all proofs
                m_LastMount = nullptr;
            }
        }

    private:
        void ApplyGodmode(Ped& mount)
        {
            if (mount.IsValid())
            {
                // Set invincibility
                mount.SetInvincible(true);

                // Set all proof types (bullet, fire, explosion, collision, melee, steam, p7, drown)
                int proofFlags = 0xFF; // 255 = full protection (8 bits)
                ENTITY::SET_ENTITY_PROOFS(mount.GetHandle(), proofFlags, false);

                // Restore health to max
                int maxHealth = ENTITY::GET_ENTITY_MAX_HEALTH(mount.GetHandle(), 0);
                int currentHealth = ENTITY::GET_ENTITY_HEALTH(mount.GetHandle());
                if (currentHealth < maxHealth)
                {
                    ENTITY::SET_ENTITY_HEALTH(mount.GetHandle(), maxHealth, 0);
                }
            }
        }
    };

    static HorseGodmode _HorseGodmode{"horsegodmode", "Godmode", "Blocks all incoming damage for your horse"};
}