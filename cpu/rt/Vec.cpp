#pragma once

#include <cmath>

struct Vec
{
    float x, y, z;
    Vec(float v = 0) { x = y = z = v; }
    Vec(float a, float b, float c = 0)
    {
        x = a;
        y = b;
        z = c;
    }
    Vec operator+(const Vec &rhs)
    {
        return Vec(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Vec operator*(const Vec &rhs)
    {
        return Vec(x * rhs.x, y * rhs.y, z * rhs.z);
    }
    Vec operator*(float &val)
    {
        return Vec(x * val, y * val, z * val);
    }
    float operator%(const Vec &rhs)
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    Vec operator!()
    {
        return *this * (1 / sqrtf(*this % *this));
    }
};
