#pragma once

#include <string>
#include <fstream>
#include <ctime>

class Logger {
public:
    explicit Logger(std::string path = "egg.log") : path_(std::move(path)) {}

    void Log(const std::string& message) {
        std::ofstream out(path_, std::ios::app);
        if (!out) return;
        out << Timestamp() << " " << message << "\n";
    }

private:
    std::string path_;

    static std::string Timestamp() {
        std::time_t t = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &t);
        char buf[32];
        std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &tm);
        return buf;
    }
};
