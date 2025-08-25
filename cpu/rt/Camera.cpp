#include "Vec.cpp"

struct Camera
{
    Vec position, direction, left, up;
    int imageWidth;
    Camera() {};
    Camera(Vec pos, Vec dir, int width) : position(pos), direction(dir), imageWidth(width) {};

    void calculateCam()
    {
        direction = !(direction + position * -1);
        left = !Vec(direction.z, 0, -direction.x) * (2. / imageWidth);
        float x = direction.y * left.z - direction.z * left.y,
              y = direction.z * left.x - direction.x * left.z,
              z = direction.x * left.y - direction.y * left.x;

        up = Vec(x, y, z);
    }

    void setPosition(Vec p)
    {
        position = p;
        calculateCam();
    }

    void setDirection(Vec d)
    {
        direction = d;
        calculateCam();
    }
};
