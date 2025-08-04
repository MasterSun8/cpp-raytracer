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
#include <atomic>
#include "display/display.cpp"
#include "rt/rt.cpp"

int imageWidth = 480, imageHeight = 270, numSamples = 4, bounces = 3;
std::string imageFileName = "pixar";

int colour_high = 1000;
int colour_low = 10;

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

#define HIT_NONE 0
#define HIT_LETTER 1
#define HIT_WALL 2
#define HIT_SUN 3

void readLetters(std::vector<char> &letters, FILE *lettersFile)
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
}

void readVectors(std::vector<Vec> &values, FILE *file)
{
    if (!file)
        return;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        float x, y, z;
        sscanf(buffer, "%f, %f, %f", &x, &y, &z);
        values.push_back(Vec(x, y, z));
    }
}

void readDetails(std::vector<int> &details, FILE *file)
{
    if (!file)
        return;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        int value;
        sscanf(buffer, "%d", &value);
        details.push_back(value);
    }
}

int main(int argc, char *argv[])
{
    srand(time(nullptr));

    std::vector<Vec> curves = {};
    std::vector<Vec> camera = {};
    std::vector<char> letters = {};
    std::vector<int> details = {};

    FILE *lettersFile = fopen("letters.txt", "r");
    FILE *curvesFile = fopen("curves.txt", "r");
    FILE *cameraFile = fopen("camera.txt", "r");
    FILE *detailsFile = fopen("details.txt", "r");

    readLetters(letters, lettersFile);
    readVectors(curves, curvesFile);
    readVectors(camera, cameraFile);
    readDetails(details, detailsFile);

    fclose(cameraFile);
    fclose(curvesFile);
    fclose(lettersFile);
    fclose(detailsFile);

    imageWidth = details[0];
    imageHeight = details[1];
    numSamples = details[2];
    bounces = details[3];

    if (argc > 1)
    {
        imageFileName = argv[1];
    }

    imageFileName = imageFileName + "_" + std::to_string(imageWidth) + "x" + std::to_string(imageHeight) + "_" + std::to_string(numSamples) + "_" + std::to_string(bounces) + ".ppm";
    std::string path = "result/" + imageFileName;

    PathTracer pathTracer(letters, curves, imageWidth, imageHeight, numSamples, bounces, 1000, 10);

    pathTracer.setLightDirection(camera[0]);
    pathTracer.scene.cam.setPosition(camera[1]);
    pathTracer.scene.cam.setDirection(camera[2]);

    printf("Light direction: (%f, %f, %f)\n", camera[0].x, camera[0].y, camera[0].z);
    printf("Camera position: (%f, %f, %f)\n", camera[1].x, camera[1].y, camera[1].z);
    printf("Camera direction: (%f, %f, %f)\n", camera[2].x, camera[2].y, camera[2].z);

    RGBDisplay display(imageWidth, imageHeight);

    FILE *outFile = fopen(path.c_str(), "wb");
    if (!outFile)
    {
        printf("Error opening output file %s\n", path.c_str());
        return 1;
    }

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

    std::atomic<int> pix{0};

    printf("Rendering %s with %d samples and %d bounces...\n", imageFileName.c_str(), numSamples, bounces);
    printf("Image size: %dx%d\n", imageWidth, imageHeight);
    printf("Finished %d pixels out of %d (%f%%)\n", 0, imageWidth * imageHeight, 0.0f);
    printf("Render time: %.2f seconds\n", 0.0);

    clock_t start = clock();
    clock_t end = clock();

    bool done = false;

    std::thread raytracingThread([&]()
                                 {
#pragma omp parallel for schedule(dynamic, 64)
        for (unsigned int pixel : order)
        {
                int pixelX = pixel % imageWidth;
                int pixelY = pixel / imageWidth;
                Vec pixelColor;

                pixelColor = pathTracer.pathPixel(pixelX, pixelY);

                pixelColor = pixelColor * (1.0f / numSamples) + 14.0f / 241.0f;
                Vec o = pixelColor + 1.0f;
                pixelColor = Vec(pixelColor.x / o.x, pixelColor.y / o.y, pixelColor.z / o.z) * 255.0f;
                
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
    Sleep(1000);
    while (!done){
            end = clock();
            int p = pix.load();
            float completion = (float)p / (imageWidth * imageHeight),
                elapsed = (double)(end - start) / CLOCKS_PER_SEC,
                estimate = elapsed / completion,
                minutes = estimate - 30 / 60,
                left = estimate - elapsed;
            int est = (int)left % 60, minest = (int)estimate % 60;
            printf("\033[2A\rFinished %d pixels out of %d (%f%%)\r\n", p, imageWidth * imageHeight, completion * 100);
            printf("Render time: %.2f seconds. Expected finish time: %.0f minutes %d seconds   \r\n", elapsed, minutes, minest);
            printf("Estimated time left: %.0f minutes and %d seconds.     ", (left - 30) / 60, est);
            fflush(stdout);
            Sleep(300);
        }
        Sleep(5000);
        end = clock();
        int p = pix.load();
        float completion = (float)p / (imageWidth * imageHeight),
            elapsed = (double)(end - start) / CLOCKS_PER_SEC,
            estimate = elapsed / completion,
            minutes = estimate / 60,
            left = estimate - elapsed;
        int est = (int)left % 60, minest = (int)estimate % 60;
        printf("\033[2A\rFinished %d pixels out of %d (%f%%)\r\n", p, imageWidth * imageHeight, completion * 100);
        printf("Render time: %.2f seconds. Expected finish time: %.0f minutes %d seconds   \r\n", elapsed, minutes, minest);
        printf("Estimated time left: %.0f minutes and %d seconds.     ", (left) / 60, est);
        printf("\nRendering of %s complete.\n", imageFileName.c_str());
        fwrite(image.data(), 1, imageWidth * imageHeight * 3, outFile);
        printf("Render time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
        printf("Rendered %dx%d with %d samples and %d bounces per ray in %.2f seconds\n", imageWidth, imageHeight, numSamples, bounces, (double)(end - start) / CLOCKS_PER_SEC);
        fflush(stdout);
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