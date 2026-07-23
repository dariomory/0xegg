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

inline nlohmann::json ActionToJson(const Action& action) {
    nlohmann::json j;
    switch (action.type) {
    case ActionType::MoveTo:
        j["type"] = "move";
        j["x"]    = action.x;
        j["y"]    = action.y;
        break;
    case ActionType::LeftClick:
        j["type"]   = "click";
        j["button"] = "left";
        break;
    case ActionType::RightClick:
        j["type"]   = "click";
        j["button"] = "right";
        break;
    case ActionType::Wait:
        j["type"] = "wait";
        j["ms"]   = action.ms;
        break;
    case ActionType::Type: {
        std::string text = action.text;
        bool pressEnter = !text.empty() && text.back() == '\r';
        if (pressEnter) text.pop_back();

        j["type"]        = "type";
        j["text"]        = text;
        j["msPerChar"]   = action.ms;
        j["pressEnter"]  = pressEnter;
        break;
    }
    }
    return j;
}

// Symmetric with LoadMacroFromFile: writes the same schema it reads, so a
// recorded or hand-built Macro can round-trip through save/load unchanged.
inline void SaveMacroToFile(const Macro& macro, const std::string& path) {
    nlohmann::json j;
    j["name"]         = macro.name;
    j["repeat"]        = macro.repeat;
    j["humanization"]  = macro.humanization;
    j["maxRuntimeMs"]  = macro.maxRuntimeMs;

    nlohmann::json actions = nlohmann::json::array();
    for (const Action& action : macro.actions) actions.push_back(ActionToJson(action));
    j["actions"] = actions;

    std::ofstream out(path);
    if (!out) throw std::runtime_error("could not write macro file: " + path);
    out << j.dump(2);
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
