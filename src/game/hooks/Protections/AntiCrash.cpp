#include "AntiCrash.hpp"
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
namespace Protections {
    namespace AntiCrash {
        using Clock = std::chrono::steady_clock;
        static std::map<int, std::vector<Clock::time_point>> eventHistory;
        static constexpr int MAX_EVENTS = 25;
        static constexpr int TIME_WINDOW = 5;
        static std::mutex historyMutex;
        void init() {
            std::lock_guard<std::mutex> lock(historyMutex);
            eventHistory.clear();
        }
        bool handleEvent(int eventId, int sourcePlayer) {
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(historyMutex);
            auto &history = eventHistory[sourcePlayer];
            history.push_back(now);
            while (!history.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - history.front()).count() > TIME_WINDOW) {
                history.erase(history.begin());
            }
            if (history.size() > MAX_EVENTS) {
                return false;
            }
            return true;
        }
        void tick() {
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(historyMutex);
            for (auto &pair : eventHistory) {
                auto &history = pair.second;
                while (!history.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - history.front()).count() > TIME_WINDOW) {
                    history.erase(history.begin());
                }
            }
        }
    }
}