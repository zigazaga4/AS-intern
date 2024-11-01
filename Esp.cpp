#include "ESP.h"
#include "Utils.h"
#include <iostream>

void RenderESP(ID2D1DeviceContext* pRenderTarget, ID2D1SolidColorBrush* pBrush, const ViewMatrix& viewMatrix, const Player* players, int playerCount, int screenWidth, int screenHeight, int mainPlayerTeam) {
    for (int i = 0; i < playerCount; ++i) {
        const Player& player = players[i];
        if (!player.isAlive) continue;

        // Calculate feet position
        Vector3 feet = player.head;
        feet.z -= 4.5f;

        Vector2 screenPosHead;
        Vector2 screenPosFeet;

        if (worldToScreen(player.head, screenPosHead, viewMatrix, screenWidth, screenHeight) &&
            worldToScreen(feet, screenPosFeet, viewMatrix, screenWidth, screenHeight)) {

            float boxHeight = screenPosFeet.y - screenPosHead.y;
            float boxWidth = boxHeight / 2.0f;

            if (boxHeight > 0 && boxWidth > 0) {
                D2D1_RECT_F rect = D2D1::RectF(screenPosHead.x - boxWidth / 2, screenPosHead.y, screenPosHead.x + boxWidth / 2, screenPosFeet.y);

                // Set color based on team
                if (player.team == mainPlayerTeam) {
                    pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));  // Friendly
                }
                else {
                    pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));  // Enemy
                }

                pRenderTarget->DrawRectangle(rect, pBrush, 2.0f);

                // Draw health bar
                float healthPercentage = player.health / 100.0f;
                D2D1_RECT_F healthBar = D2D1::RectF(
                    rect.left - 5,
                    rect.top,
                    rect.left - 2,
                    rect.top + (rect.bottom - rect.top) * healthPercentage
                );
                pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
                pRenderTarget->FillRectangle(healthBar, pBrush);
            }
        }
    }
}
