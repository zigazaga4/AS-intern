#pragma once

#include "DLLInterface.h"

struct Vector2 {
    float x, y;
};

bool worldToScreen(const Vector3& pos, Vector2& screen, const ViewMatrix& matrix, int windowWidth, int windowHeight);