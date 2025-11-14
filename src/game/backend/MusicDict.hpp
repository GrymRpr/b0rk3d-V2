#pragma once
#include <vector>
#include <string>

namespace YimMenu
{
    class MusicDict
    {
    public:
        static const std::vector<std::string>& GetAllMusicEvents();
        static void ReloadMusicDict();
    };
}