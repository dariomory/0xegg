#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "Action.h"

struct Macro {
    std::string          name;
    int                  repeat       = 1;
    std::string          humanization = "off"; // "off" | "subtle" | "human"
    int                  maxRuntimeMs = 5 * 60 * 1000;
    std::vector<Action>  actions;
};

inline Action ParseAction(const nlohmann::json& item) {
    std::string type = item.value("type", std::string());
    Action action;

    if (type == "move") {
        action.type = ActionType::MoveTo;
        action.x = item.at("x").get<int>();
        action.y = item.at("y").get<int>();
    } else if (type == "click") {
        std::string button = item.value("button", std::string("left"));
        action.type = (button == "right") ? ActionType::RightClick : ActionType::LeftClick;
    } else if (type == "wait") {
        action.type = ActionType::Wait;
        action.ms = item.at("ms").get<int>();
    } else if (type == "type") {
        action.type = ActionType::Type;
        action.text = item.at("text").get<std::string>();
        action.ms   = item.value("msPerChar", 30);
        if (item.value("pressEnter", false)) action.text += '\r';
    } else {
        throw std::runtime_error("unknown action type \"" + type + "\"");
    }

    return action;
}

inline Macro LoadMacroFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("could not open macro file: " + path);

    nlohmann::json j;
    file >> j;

    Macro macro;
    macro.name         = j.value("name", std::string());
    macro.repeat       = j.value("repeat", 1);
    macro.humanization = j.value("humanization", std::string("off"));
    macro.maxRuntimeMs = j.value("maxRuntimeMs", macro.maxRuntimeMs);

    if (!j.contains("actions") || !j["actions"].is_array())
        throw std::runtime_error("macro must have an \"actions\" array");

    int index = 0;
    for (const auto& item : j["actions"]) {
        try {
            macro.actions.push_back(ParseAction(item));
        } catch (const std::exception& e) {
            throw std::runtime_error("action[" + std::to_string(index) + "]: " + e.what());
        }
        index++;
    }

    return macro;
}
