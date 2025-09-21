#include "MirrorDefense.hpp"
#include "SendMessage.hpp"
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
namespace Protections {
    namespace MirrorDefense {
        using Clock = std::chrono::steady_clock;
        struct EventRecord {
            int eventId;
            Clock::time_point timestamp;
        };
        static std::map<int, std::vector<EventRecord>> playerEvents;
        static constexpr int MAX_EVENTS = 20;
        static constexpr int TIME_WINDOW = 8;
        static std::mutex eventsMutex;
        void init() {
            std::lock_guard<std::mutex> lock(eventsMutex);
            playerEvents.clear();
        }
        bool handleEvent(int eventId, int sourcePlayer) {
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(eventsMutex);
            auto &history = playerEvents[sourcePlayer];
            history.push_back({ eventId, now });
            while (!history.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - history.front().timestamp).count() > TIME_WINDOW) {
                history.erase(history.begin());
            }
            if (history.size() > MAX_EVENTS) {
                Protections::SendMessage::showMessageToPlayer(sourcePlayer, "Mirror event threshold exceeded. Action reflected.");
                return false;
            }
            return true;
        }
        void tick() {
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(eventsMutex);
            for (auto &pair : playerEvents) {
                auto &history = pair.second;
                while (!history.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - history.front().timestamp).count() > TIME_WINDOW) {
                    history.erase(history.begin());
                }
            }
        }
    }
}