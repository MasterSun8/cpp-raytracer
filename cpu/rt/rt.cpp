#include <vector>
#include <math.h>
#include <omp.h>
#include <thread>
#include "SceneGeometry.cpp"
#include "Material.cpp"
#include "HaltonState.cpp"

enum HIT
{
    HIT_NONE,
    HIT_LETTER,
    HIT_WALL,
    HIT_SUN
};

enum HIT trash;

class PathTracer
{
private:
    int colour_high, colour_low;

    int imageWidth, imageHeight, numSamples, bounces;

    float cos_table[4096], sin_table[4096];

    HaltonState halton = HaltonState(0);

public:
    Material wall;
    Material letter;

    SceneGeometry scene;
    PathTracer(std::vector<char> l, std::vector<Vec> c, int width = 480, int height = 270, int samples = 8, int bounces = 3, int high = 100, int low = 10) : imageWidth(width), imageHeight(height), numSamples(samples), bounces(bounces), colour_high(high), colour_low(low)
    {
        for (int i = 0; i < 4096; i++)
        {
            float angle = (i / 4096.0f) * 2 * M_PI;
            cos_table[i] = cosf(angle);
            sin_table[i] = sinf(angle);
        }

        scene = SceneGeometry(l,                                              //  letters
                              c,                                              //  curves
                              Vec{.6, .6, 1},                                 //  lightDirection
                              Camera(Vec{-22, 10, 25}, Vec{-3, 4, 0}, width), //  cam
                              Vec(-30, -.5, -30),                             //  roomMin
                              Vec(30, 18, 30),                                //  roomMax
                              Vec(-25, 17, -25),                              //  ceilingMin
                              Vec(25, 20, 25),                                //  ceilingMax
                              Vec(1.5, 18.5, -25),                            //  columnMin
                              Vec(6.5, 20, 25)                                //  columnMax
        );
        wall = Material(Vec(colour_high, colour_low, colour_low), false, 1, 0.5, 1);
        letter = Material(Vec(1, 1, 1), false, 1, 0.8, 1);
    }

    void setLightDirection(Vec ld)
    {
        scene.lightDirection = !ld;
    }

    Vec getLightDirection()
    {
        return scene.lightDirection;
    }

    float min(float l, float r)
    {
        return l < r ? l : r;
    }

    float box(Vec position, Vec left, Vec right)
    {
        left = position + left * -1;
        right = right + position * -1;
        return -min(
            min(
                min(left.x, right.x),
                min(left.y, right.y)),
            min(left.z, right.z));
    }

    float SDF(Vec position, enum HIT &materialType)
    {
        float distance = 1e9;
        Vec flatPos = position;
        flatPos.z = 0;

        for (int id = 0; id < scene.letters.size(); id += 4)
        {
            Vec curveStart = Vec(scene.letters[id] - 79, scene.letters[id + 1] - 79) * .5;
            Vec curveDir = Vec(scene.letters[id + 2] - 79, scene.letters[id + 3] - 79) * .5 + curveStart * -1;
            Vec toFlat = curveStart + flatPos * -1;
            float t = toFlat % curveDir / (curveDir % curveDir);
            t = min(-min(t, 0), 1);
            Vec closestPoint = curveStart + curveDir * t;
            Vec offset = flatPos + closestPoint * -1;
            distance = min(distance, offset % offset);
        }

        distance = sqrtf(distance);

        for (Vec &curve : scene.curves)
        {
            Vec offset = flatPos + curve * -1;
            distance = min(distance,
                           offset.x > 0
                               ? fabsf(sqrtf(offset % offset) - 2)
                               : (offset.y += offset.y > 0 ? -2 : 2, sqrtf(offset % offset)));
        }

        distance = powf(powf(distance, 8) + powf(position.z, 8), .125) - .5;
        materialType = HIT_LETTER;

        float mainRoom = box(position, scene.roomMin, scene.roomMax);
        float ceilingCutout = box(position, scene.ceilingMin, scene.ceilingMax);
        float roomInterior = -min(mainRoom, ceilingCutout);

        Vec columnPos = Vec(fmodf(fabsf(position.x), 8), position.y, position.z);
        float columns = box(columnPos, scene.columnMin, scene.columnMax);

        float room = min(roomInterior, columns);

        if (room < distance)
            distance = room, materialType = HIT_WALL;

        float ceiling = 19.9 - position.y;

        if (ceiling < distance)
            distance = ceiling, materialType = HIT_SUN;
        return distance;
    }

    enum HIT march(Vec origin, Vec direction, Vec &hitPos, Vec &normal)
    {
        enum HIT materialType = HIT_NONE;
        int steps = 0;
        float d;

        for (float total_d = 0; total_d < 100; total_d += d)
            if ((d = SDF(hitPos = origin + direction * total_d, materialType)) < .01 || ++steps > 99)
                return normal =
                           !Vec(SDF(hitPos + Vec(.01, 0), trash) - d,
                                SDF(hitPos + Vec(0, .01), trash) - d,
                                SDF(hitPos + Vec(0, 0, .01), trash) - d),
                       materialType;
        return HIT_NONE;
    }

    Vec trace(Vec origin, Vec direction)
    {
        Vec sampledPosition, normal, color;
        float attenuation = 1;

        for (int bounceCount = bounces; bounceCount--;)
        {
            enum HIT hitType = march(origin, direction, sampledPosition, normal);
            if (hitType == HIT_NONE)
                break;
            if (hitType == HIT_LETTER)
            {
                color = color * 1;
                direction = direction + normal * (normal % direction * -2);
                origin = sampledPosition + direction * 0.1;
                attenuation = attenuation * 1;
            }
            if (hitType == HIT_WALL)
            {

                Vec color1 = Vec(colour_high, colour_low, colour_low);
                Vec color2 = Vec(colour_low, colour_high, colour_low);
                Vec color3 = Vec(colour_low, colour_low, colour_high);

                float xWeight = fabsf(normal.x);
                float yWeight = fabsf(normal.y);
                float zWeight = fabsf(normal.z);

                float totalWeight = xWeight + yWeight + zWeight;
                xWeight /= totalWeight;
                yWeight /= totalWeight;
                zWeight /= totalWeight;

                Vec wallColor = color1 * xWeight + color2 * yWeight + color3 * zWeight;

                float incidence = normal % scene.lightDirection;
                float p = 6.283185 * halton.dim(3);
                int idx = (int)(p * 651.8986f) & 4095;
                float c = halton.dim(4);
                float s = sqrtf(1 - c);
                float g = normal.z < 0 ? -1 : 1;
                float u = -1 / (g + normal.z);
                float v = normal.x * normal.y * u;

                Vec tangent1 = Vec(v, g + normal.y * normal.y * u, -normal.y);
                Vec tangent2 = Vec(1 + g * normal.x * normal.x * u, g * v, -g * normal.x);
                direction = tangent1 * (cos_table[idx] * s) + tangent2 * (sin_table[idx] * s) + normal * sqrtf(c);
                origin = sampledPosition + direction * .1;
                attenuation = attenuation * .2;
                if (incidence > 0 && march(sampledPosition + normal * .1,
                                           scene.lightDirection,
                                           sampledPosition,
                                           normal) == HIT_SUN)
                    color = color + wallColor * attenuation * incidence;
            }
            if (hitType == HIT_SUN)
            {
                color = color + Vec(70, 70, 100) * attenuation;
                break;
            }
        }
        return color;
    }

    Vec pathPixel(int pixelX, int pixelY)
    {
        Vec pixelColor;
        int sequenceIndex = (pixelY * imageHeight + pixelX) * (numSamples + bounces * 2) + numSamples;
        halton = HaltonState(sequenceIndex);
        for (int sampleIdx = 0; sampleIdx < numSamples; sampleIdx++)
        {
            halton.baseIndex++;
            pixelColor = pixelColor + trace(
                                          scene.cam.position,
                                          !(scene.cam.direction +
                                            scene.cam.left * (pixelX - imageWidth / 2 + halton.dim(1)) +
                                            scene.cam.up * (pixelY - imageHeight / 2 + halton.dim(2))));
        }
        return pixelColor;
    }
};