#include "SendMessage.hpp"
#include <iostream>
#include <algorithm>
namespace Protections {
    namespace SendMessage {
        static bool isValidMessage(const std::string& msg) {
            if (msg.empty() || msg.size() > 256) return false;
            return std::all_of(msg.begin(), msg.end(), [](unsigned char c) {
                return c >= 32 && c <= 126;
            });
        }
        void showMessageToPlayer(int targetPlayer, const std::string& msg) {
            if (!isValidMessage(msg)) return;
            std::cout << "[Message to player " << targetPlayer << "]: " << msg << std::endl;
        }
    }
}