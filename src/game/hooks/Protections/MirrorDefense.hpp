#pragma once
namespace Protections {
    namespace MirrorDefense {
        void init();
        void tick();
        bool handleEvent(int eventId, int sourcePlayer);
    }
}