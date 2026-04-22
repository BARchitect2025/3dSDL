#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

#include <math.h>
#include <vector>

struct Mesh {
    std::vector<short> points;
    std::vector<short> indicies;
    std::string type;

    Mesh(std::vector<short> points, std::vector<short> indicies, std::string type) {
        this -> points = points;
        this -> indicies = indicies;

        this -> type = type;
    }
};

const short FOV = 94;
const float PI = 3.1415;

int main(int argc, char* argv[]) {
    std::ifstream settings_file("data/settings.json");
    json settings = json::parse(settings_file);

    short x_coords[3];
    short y_coords[3];
    int num_points = 3;

    short points3d[3][3] = {
        {800, 400, 800},
        {600, 400, 800},
        {800, 200, 800}
    };

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("3dSDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    bool running = true;

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false;
                }
            }
        }

        float focal_length = (width / 2.0f) / tan((FOV / 2.0f) * (PI / 180.0f));

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderClear(renderer);

        x_coords[0] = points3d[0][0] * (focal_length / points3d[0][2]) + width / 2;
        x_coords[1] = points3d[1][0] * (focal_length / points3d[1][2]) + width / 2;
        x_coords[2] = points3d[2][0] * (focal_length / points3d[1][2]) + width / 2;
        
        y_coords[0] = points3d[0][1] * (focal_length / points3d[0][2]) + height / 2;
        y_coords[1] = points3d[1][1] * (focal_length / points3d[1][2]) + height / 2;
        y_coords[2] = points3d[2][1] * (focal_length / points3d[1][2]) + height / 2;

        filledPolygonRGBA(renderer, x_coords, y_coords, 3, 255, 0, 255, 255);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
