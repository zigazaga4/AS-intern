#include "Logging.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>

std::ofstream mainLogFile;

void MainLog(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm time_info;
    localtime_s(&time_info, &now_c);
    std::stringstream ss;
    ss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S") << " - " << message;

    if (mainLogFile.is_open()) {
        mainLogFile << ss.str() << std::endl;
        mainLogFile.flush();
    }
    std::cout << ss.str() << std::endl;
    OutputDebugStringA((ss.str() + "\n").c_str());
}

void LogReceivedGameData(const GameData& data) {
    std::stringstream ss;
    ss << "Received GameData: Players: " << data.playerCount
        << ", WindowSize: " << data.windowWidth << "x" << data.windowHeight
        << ", MainPlayerTeam: " << data.mainPlayerTeam
        << ", DataId: " << data.dataId;
    MainLog(ss.str());
}
