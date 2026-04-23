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
    std::vector<Mesh> objects;

    std::ifstream settings_file("data/settings.json");
    json settings = json::parse(settings_file);
    
    short draw_dist = settings["draw_dist"];

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

        for (Mesh obj: objects) {
            std::vector<short> tri_draw_data;

            for (int i = 0; i < sizeof(obj.indicies) / 3; i++) {
                for (int j = 0; j < 3; j++) {
                    short drawX = obj.points[obj.indicies[i][j]] * (focal_length / v_rel[2]) + width / 2;
                    short drawY = v_rel[1] * (focal_length / v_rel[2]) + height / 2;
                }
            }
        }

        filledPolygonRGBA(renderer, x_coords, y_coords, 3, 255, 0, 255, 255);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
