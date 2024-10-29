#include "Utils.h"
#include <cmath>

bool worldToScreen(const Vector3& pos, Vector2& screen, const ViewMatrix& matrix, int windowWidth, int windowHeight) {
    Vector4 clip_coords;
    clip_coords.x = pos.x * matrix.matrix[0] + pos.y * matrix.matrix[4] + pos.z * matrix.matrix[8] + matrix.matrix[12];
    clip_coords.y = pos.x * matrix.matrix[1] + pos.y * matrix.matrix[5] + pos.z * matrix.matrix[9] + matrix.matrix[13];
    clip_coords.z = pos.x * matrix.matrix[2] + pos.y * matrix.matrix[6] + pos.z * matrix.matrix[10] + matrix.matrix[14];
    clip_coords.w = pos.x * matrix.matrix[3] + pos.y * matrix.matrix[7] + pos.z * matrix.matrix[11] + matrix.matrix[15];

    if (clip_coords.w < 0.1f) {
        return false;
    }

    Vector3 NDC;
    NDC.x = clip_coords.x / clip_coords.w;
    NDC.y = clip_coords.y / clip_coords.w;
    NDC.z = clip_coords.z / clip_coords.w;

    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);

    return true;
}