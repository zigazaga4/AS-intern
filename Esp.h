#pragma once
#include <d2d1_1.h>
#include "DLLInterface.h"

void RenderESP(ID2D1DeviceContext* pRenderTarget, ID2D1SolidColorBrush* pBrush, const ViewMatrix& viewMatrix, const Player* players, int playerCount, int screenWidth, int screenHeight, int mainPlayerTeam);