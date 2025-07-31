#include <vector>
#include <math.h>

#define HIT_NONE 0
#define HIT_LETTER 1
#define HIT_WALL 2
#define HIT_SUN 3

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
    Vec operator+(Vec rhs)
    {
        return Vec(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Vec operator*(Vec rhs)
    {
        return Vec(x * rhs.x, y * rhs.y, z * rhs.z);
    }
    float operator%(Vec rhs)
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    Vec operator!()
    {
        return *this * (1 / sqrtf(*this % *this));
    }
};

class PathTracer
{
private:
    int colour_high = 1000;
    int colour_low = 10;

    std::vector<char> letters = {};
    std::vector<Vec> curves = {};
    Vec lightDirection{!Vec{.6, .6, 1}};

    int imageWidth = 480, imageHeight = 270, numSamples = 4, bounces = 3;

public:
    struct Camera
    {
        Vec position, direction, left, up;
        int imageWidth;
        Camera() {};
        Camera(Vec pos, Vec dir, int width) : position(pos), direction(dir), imageWidth(width)
        {
            direction = !(dir + position * -1);
            left = !Vec(direction.z, 0, -direction.x) * (1. / imageWidth);
            up = Vec(direction.y * left.z - direction.z * left.y,
                     direction.z * left.x - direction.x * left.z,
                     direction.x * left.y - direction.y * left.x);
        };
    };

    Camera cam;

    PathTracer(std::vector<char> l, std::vector<Vec> c, int width, int height, int samples, int bounces, int high, int low) : imageWidth(width), imageHeight(height), numSamples(samples), bounces(bounces), colour_high(high), colour_low(low)
    {
        letters = l;
        curves = c;
        cam = Camera(Vec{-22, 10, 25}, Vec{-3, 4, 0}, width);
    }

    float min(float l, float r)
    {
        return l < r ? l : r;
    }

    float random()
    {
        return (float)rand() / RAND_MAX;
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

    float SDF(Vec position, int &materialType)
    {
        float distance = 1e9;
        Vec flatPos = position;
        flatPos.z = 0;

        for (int id = 0; id < letters.size(); id += 4)
        {
            Vec curveStart = Vec(letters[id] - 79, letters[id + 1] - 79) * .5;
            Vec curveDir = Vec(letters[id + 2] - 79, letters[id + 3] - 79) * .5 + curveStart * -1;
            Vec offset = flatPos + (curveStart + curveDir * min(-min((curveStart + flatPos * -1) % curveDir / (curveDir % curveDir), 0), 1)) * -1;
            distance = min(distance, offset % offset);
        }

        distance = sqrtf(distance);

        for (Vec &curve : curves)
        {
            Vec offset = flatPos + curve * -1;
            distance = min(distance,
                           offset.x > 0
                               ? fabsf(sqrtf(offset % offset) - 2)
                               : (offset.y += offset.y > 0 ? -2 : 2, sqrtf(offset % offset)));
        }

        distance = powf(powf(distance, 8) + powf(position.z, 8), .125) - .5;
        materialType = 1;
        float room = min(
            -min(
                box(
                    position,
                    Vec(-30, -.5, -30),
                    Vec(30, 18, 30)),
                box(
                    position,
                    Vec(-25, 17, -25),
                    Vec(25, 20, 25))),
            box(
                Vec(
                    fmodf(
                        fabsf(position.x),
                        8),
                    position.y,
                    position.z),
                Vec(1.5, 18.5, -25),
                Vec(6.5, 20, 25)));

        if (room < distance)
            distance = room, materialType = 2;

        float ceiling = 19.9 - position.y;

        if (ceiling < distance)
            distance = ceiling, materialType = 3;
        return distance;
    }

    int march(Vec origin, Vec direction, Vec &hitPos, Vec &normal)
    {
        int materialType = HIT_NONE;
        int steps = 0;
        float d;

        for (float total_d = 0; total_d < 100; total_d += d)
            if ((d = SDF(hitPos = origin + direction * total_d, materialType)) < .01 || ++steps > 99)
                return normal =
                           !Vec(SDF(hitPos + Vec(.01, 0), steps) - d,
                                SDF(hitPos + Vec(0, .01), steps) - d,
                                SDF(hitPos + Vec(0, 0, .01), steps) - d),
                       materialType;
        return 0;
    }

    Vec trace(Vec origin, Vec direction)
    {
        Vec sampledPosition, normal, color, attenuation = 1;

        for (int bounceCount = bounces; bounceCount--;)
        {
            int hitType = march(origin, direction, sampledPosition, normal);
            if (hitType == HIT_NONE)
                break;
            if (hitType == HIT_LETTER)
            {
                color = color * .8;
                direction = direction + normal * (normal % direction * -2);
                origin = sampledPosition + direction * 0.1;
                attenuation = attenuation * 0.1;
            }
            if (hitType == HIT_WALL)
            {

                Vec color1 = Vec(colour_high, colour_low, colour_low); // X-facing walls - red
                Vec color2 = Vec(colour_low, colour_high, colour_low); // Y-facing walls - green
                Vec color3 = Vec(colour_low, colour_low, colour_high); // Z-facing walls - blue

                // Use absolute normal components as weights
                float xWeight = fabsf(normal.x);
                float yWeight = fabsf(normal.y);
                float zWeight = fabsf(normal.z);

                // Normalize weights so they sum to 1
                float totalWeight = xWeight + yWeight + zWeight;
                xWeight /= totalWeight;
                yWeight /= totalWeight;
                zWeight /= totalWeight;

                // Blend colors based on normal direction
                Vec wallColor = color1 * xWeight + color2 * yWeight + color3 * zWeight;
                /**/

                // Vec wallColor = Vec(400, 400, 400);

                float incidence = normal % lightDirection;
                float p = 6.283185 * random();
                float c = random();
                float s = sqrtf(1 - c);
                float g = normal.z < 0 ? -1 : 1;
                float u = -1 / (g + normal.z);
                float v = normal.x * normal.y * u;
                direction = Vec(v,
                                g + normal.y * normal.y * u,
                                -normal.y) *
                                (cosf(p) * s) +
                            Vec(1 + g * normal.x * normal.x * u,
                                g * v,
                                -g * normal.x) *
                                (sinf(p) * s) +
                            normal * sqrtf(c);
                origin = sampledPosition + direction * .1;
                attenuation = attenuation * 0.2;
                if (incidence > 0 && march(sampledPosition + normal * .1,
                                           lightDirection,
                                           sampledPosition,
                                           normal) == HIT_SUN)
                    color = color + attenuation * wallColor * incidence;
            }
            if (hitType == HIT_SUN)
            { //
                color = color + attenuation * Vec(70, 70, 100);
                break; // Sun Color
            }
        }
        return color;
    }

    Vec pathPixel(int pixelX, int pixelY)
    {
        Vec pixelColor;
        for (int sampleIdx = numSamples; sampleIdx--;)
            pixelColor = pixelColor + trace(cam.position, !(cam.direction + cam.left * (pixelX - imageWidth / 2 + random()) + cam.up * (pixelY - imageHeight / 2 + random())));
        return pixelColor;
    }
};