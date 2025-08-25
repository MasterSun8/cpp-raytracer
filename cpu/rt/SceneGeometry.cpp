#include <vector>
#include "Camera.cpp"

struct SceneGeometry
{
    std::vector<char> letters = {};
    std::vector<Vec> curves = {};

    Vec lightDirection;
    Vec roomMin,
        roomMax,
        ceilingMin,
        ceilingMax,
        columnMin,
        columnMax;

    Camera cam;
    SceneGeometry() {}

    SceneGeometry(std::vector<char> l, std::vector<Vec> cur, Vec ld, Camera c, Vec rMin, Vec rMax, Vec cMin, Vec cMax, Vec colMin, Vec colMax)
    {
        lightDirection = !ld;
        letters = l;
        curves = cur;
        cam = c;
        roomMin = rMin;
        roomMax = rMax;
        ceilingMin = cMin;
        ceilingMax = cMax;
        columnMin = colMin;
        columnMax = colMax;
    }
};
