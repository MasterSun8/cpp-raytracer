#include "Vec.cpp"

struct Material
{
    Vec colour;

    bool emissive;

    float roughness,
        reflectiveness,
        attenuation;

    Material()
        : colour(1), emissive(false), roughness(1), reflectiveness(0), attenuation(1) {}

    Material(Vec col, bool emissive, float roughness, float reflectiveness, float attenuation)
        : colour(col), emissive(emissive), roughness(roughness), reflectiveness(reflectiveness), attenuation(attenuation) {};

    void calculateRayBounce()
    {
        }
};