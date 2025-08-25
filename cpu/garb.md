```
Vec hitPos = 0;
    Vec surfaceNormal = 0;
    Vec reflection = 0;
    Vec transmission = 1;
    Vec lightDir(!Vec(.6, .6, .6));

    for (int bounce = bounces; bounce--;)
    {
        int materialType = march(origin, direction, hitPos, surfaceNormal);
        if (materialType == HIT_NONE)
            break;
        if (materialType == HIT_LETTER)
        {
            // reflection = reflection + transmission * Vec(255, 255, 255);

            // reflection = reflection + surfaceNormal * (surfaceNormal % reflection * -2);
            // hitPos = hitPos + reflection * .1;
            // transmission = transmission * .2;

            // reflection = reflection + surfaceNormal * (surfaceNormal % reflection * -2);
            // direction = direction + surfaceNormal * (-2 * (direction % surfaceNormal));
            // origin = hitPos + direction * .1;
            // transmission = transmission * .6;

            direction = direction + surfaceNormal * (surfaceNormal % direction * -2);
            origin = hitPos + direction * 0.1;
            transmission = transmission * 0.2;

            continue;
        }
        if (materialType == HIT_WALL)
        {
            float intensity = surfaceNormal % lightDir;
            float phi = 6.283185 * random();
            float cosTheta = random();
            float sinTheta = sqrtf(1 - cosTheta);
            float sign = surfaceNormal.z < 0 ? -1 : 1;
            float uVal = -1 / (sign + surfaceNormal.z);
            float vVal = surfaceNormal.x * surfaceNormal.y * uVal;
            reflection = Vec(vVal,
                             sign + surfaceNormal.y * surfaceNormal.y * uVal,
                             -surfaceNormal.y) *
                             (cosf(phi) * sinTheta) +
                         Vec(1 + sign * surfaceNormal.x * surfaceNormal.x * uVal,
                             sign * vVal,
                             -sign * surfaceNormal.x) *
                             (sinf(phi) * sinTheta) +
                         surfaceNormal * sqrtf(cosTheta);
            hitPos = hitPos + reflection * .1;
            transmission = transmission * .2;
            if (intensity > 0 && march(hitPos + surfaceNormal * .1, lightDir, hitPos, surfaceNormal) == 3)
                reflection = reflection + transmission * Vec(500, 400, 400) * intensity;
            continue;
        }
        if (materialType == HIT_SUN)
        {
            reflection = reflection + transmission * Vec(50, 80, 100);
            break;
        }
    }
    return reflection;
```