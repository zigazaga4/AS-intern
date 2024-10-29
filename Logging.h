#pragma once
#include <string>
#include "DLLInterface.h"

extern std::ofstream mainLogFile;

void MainLog(const std::string& message);
void LogReceivedGameData(const GameData& data);
