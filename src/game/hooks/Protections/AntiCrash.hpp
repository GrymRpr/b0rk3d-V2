#pragma once
namespace Protections {
    namespace AntiCrash {
        void init();
        bool handleEvent(int eventId, int sourcePlayer);
        void tick();
    }
}