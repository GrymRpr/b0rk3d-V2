#pragma once

#include "common.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Pools.hpp"
#include "game/rdr/Scripts.hpp"
#include "util/Math.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "game/rdr/invoker/Invoker.hpp"
#include "imgui.h"

namespace YimMenu
{
    class ESP
    {
    public:
        static ImColor GetColorBasedOnDistance(float distance);
        static void DrawTextInfo(ImDrawList* drawList, const ImVec2& position, const std::string& text, ImColor color);
        static ImVec2 boneToScreen(rage::fvector3 bone);
        static void DrawSkeleton(Ped ped, ImDrawList* drawList, ImColor color);
        static void DrawHorseSkeleton(Ped horse, ImDrawList* drawList, ImColor color);
        static void DrawPlayer(Player& plyr, ImDrawList* drawList);
        static void DrawPeds(Ped ped, ImDrawList* drawList);
        static void Draw();
    };
}