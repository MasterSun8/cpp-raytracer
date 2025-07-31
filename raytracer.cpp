#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <vector>
#include <time.h>
#include <string>
#include <windows.h>
#include <thread>
#include <random>
#include <algorithm>
#include "display/display.cpp"
#include "rt/rt.cpp"

int imageWidth = 480, imageHeight = 270, numSamples = 4, bounces = 3;
int jumps = 8;
bool state = false;
std::string imageFileName = "pixar";

int colour_high = 1000;
int colour_low = 10;

std::vector<Vec> curves = {};
std::vector<char> letters = {};
/*
    "5O5_"
    "5W9W"
    "5_9_" // P (without curve)
    "AOEO"
    "COC_"
    "A_E_" // I
    "IOQ_"
    "I_QO" // X
    "UOY_"
    "Y_]O"
    "WW[W" // A
    "aOa_"
    "aWeW"
    "a_e_"
    "cWiO"; // R (without curve)
/**/

FILE *lettersFile = fopen("letters.txt", "r");
FILE *curvesFile = fopen("curves.txt", "r");

#define HIT_NONE 0
#define HIT_LETTER 1
#define HIT_WALL 2
#define HIT_SUN 3

void readLetters()
{
    if (!lettersFile)
        return;
    char buffer[256];
    int i = 0;
    while (fgets(buffer, sizeof(buffer), lettersFile))
    {
        for (int i = 0; buffer[i] && buffer[i + 3]; i += 4)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (buffer[i + j] == '\n')
                    continue;
                letters.push_back(buffer[i + j]);
            }
        }
    }

    // for (char c : letters)
    // printf("%c", c);
    // printf("\n");
    fclose(lettersFile);
}

void readCurves()
{
    if (!curvesFile)
        return;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), curvesFile))
    {
        int x, y, z;
        sscanf(buffer, "%d, %d, %d", &x, &y, &z);
        curves.push_back(Vec(x, y, z));
    }
    fclose(curvesFile);
}

int main(int argc, char *argv[])
{
    readLetters();
    readCurves();
    if (argc > 6)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = atoi(argv[2]);
        numSamples = atoi(argv[3]);
        bounces = atoi(argv[4]);
        imageFileName = argv[5];
        jumps = atoi(argv[6]);
        state = false;
    }
    else if (argc > 5)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = atoi(argv[2]);
        numSamples = atoi(argv[3]);
        bounces = atoi(argv[4]);
        imageFileName = argv[5];
    }
    else if (argc > 4)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = atoi(argv[2]);
        numSamples = atoi(argv[3]);
        bounces = atoi(argv[4]);
    }
    else if (argc > 3)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = atoi(argv[2]);
        numSamples = atoi(argv[3]);
    }
    else if (argc > 2)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = atoi(argv[2]);
    }
    else if (argc > 1)
    {
        imageWidth = atoi(argv[1]);
        imageHeight = imageWidth * 9 / 16;
    }

    imageFileName = imageFileName + "_" + std::to_string(imageWidth) + "x" + std::to_string(imageHeight) + "_" + std::to_string(numSamples) + "_" + std::to_string(bounces) + "_" + std::to_string(state);
    imageFileName = imageFileName + ".ppm";

    PathTracer pathTracer(letters, curves, imageWidth, imageHeight, numSamples, bounces, 1000, 10);

    RGBDisplay display(imageWidth, imageHeight);

    FILE *outFile = fopen(imageFileName.c_str(), "wb");
    if (!outFile)
        return 1;

    fprintf(outFile, "P6 %d %d 255 ", imageWidth, imageHeight);
    std::vector<unsigned char> image(imageWidth * imageHeight * 3, 0);

    std::vector<unsigned int> order(imageWidth * imageHeight, 0);
    unsigned int counter = 0;
    for (unsigned int &x : order)
    {
        x = counter++;
    }

    auto rng = std::default_random_engine();
    std::shuffle(order.begin(), order.end(), rng);

    int pix = 0, row = 0;

    printf("Rendering %s with %d samples and %d bounces...\n", imageFileName.c_str(), numSamples, bounces);
    printf("Image size: %dx%d\n", imageWidth, imageHeight);
    printf("Finished %d pixels out of %d (%f%%)\n", pix, imageWidth * imageHeight, (float)pix / (imageWidth * imageHeight) * 100);
    printf("Render time: %.2f seconds\n", 0.0);

    clock_t start = clock();
    clock_t end = clock();

    bool done = false;

    std::thread raytracingThread([&]()
                                 {
#pragma omp parallel for // schedule(guided, jumps)
        for (unsigned int pixel : order)
        {
                int pixelX = pixel % imageWidth;
                int pixelY = pixel / imageWidth;
                Vec pixelColor;

                pixelColor = pathTracer.pathPixel(pixelX, pixelY);

                if (!state)
                {
                    pixelColor = pixelColor * (1.0f / numSamples) + 14.0f / 241.0f;
                    Vec o = pixelColor + 1.0f;
                    pixelColor = Vec(pixelColor.x / o.x, pixelColor.y / o.y, pixelColor.z / o.z) * 255.0f;
                }
                else
                {
                    pixelColor = pixelColor * (1.0f / numSamples);
                    pixelColor.x = std::max(0.0f, std::min(255.0f, pixelColor.x * 255.0f));
                    pixelColor.y = std::max(0.0f, std::min(255.0f, pixelColor.y * 255.0f));
                    pixelColor.z = std::max(0.0f, std::min(255.0f, pixelColor.z * 255.0f));
                }

                int flippedY = imageHeight - 1 - pixelY;
                int flippedX = imageWidth - 1 - pixelX;
                int id = 3 * (flippedY * imageWidth + flippedX);
                image[id + 0] = (unsigned char)pixelColor.x;
                image[id + 1] = (unsigned char)pixelColor.y;
                image[id + 2] = (unsigned char)pixelColor.z;
                pix++;
        }
        done = true; });

    bool ddone = false;

    std::thread estimateThread([&]()
                               {
        while (!done)
        {
            end = clock();
            float completion = (float)pix / (imageWidth * imageHeight),
                elapsed = (double)(end - start) / CLOCKS_PER_SEC,
                estimate = elapsed / completion,
                minutes = estimate / 60,
                left = estimate - elapsed;
            int est = (int)left % 60, minest = (int)estimate % 60;
            printf("\033[2A\rFinished %d pixels out of %d (%f%%)\r\n", pix, imageWidth * imageHeight, completion * 100);
            printf("Render time: %.2f seconds. Expected finish time: %.0f minutes %d seconds   \r\n", elapsed, minutes, minest);
            printf("Estimated time left: %.0f minutes and %d seconds.     ", (left) / 60, est);
            fflush(stdout);
            Sleep(300);
        }
        printf("\nRendering of %s complete.\n", imageFileName.c_str());
        fwrite(image.data(), 1, imageWidth * imageHeight * 3, outFile);
        end = clock();
        printf("Render time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
        printf("Rendered %dx%d with %d samples and %d bounces per ray in %.2f seconds\n", imageWidth, imageHeight, numSamples, bounces, (double)(end - start) / CLOCKS_PER_SEC);
        fclose(outFile);
        ddone = true; });

    for (int x = 0; true; x++)
    {
        x %= 10;
        display.processMessages();
        if (display.isWindowVisible() && x % 10 == 0)
        {
            display.update(image.data());
        }
        if (ddone)
        {
            if (estimateThread.joinable())
                estimateThread.join();
            if (raytracingThread.joinable())
                raytracingThread.join();
            break;
        }
        Sleep(8);
    }
}
// Inspired by Andrew Kensler's postcard pathtracer