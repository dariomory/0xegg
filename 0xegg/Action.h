#pragma once

#include <string>

enum class ActionType { MoveTo, LeftClick, RightClick, Type, Wait };

struct Action {
    ActionType  type;
    int         x    = 0;
    int         y    = 0;
    std::string text;
    int         ms   = 0;
};
