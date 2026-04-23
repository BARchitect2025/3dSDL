#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

#include <math.h>
#include <vector>

struct Mesh {
    std::vector<std::vector<short>> points;
    std::vector<std::vector<std::vector<short>>> indices;
    std::string type;

    Mesh(std::vector<std::vector<short>> points, std::vector<std::vector<std::vector<short>>> indices, std::string type, short x_offset, short y_offset, short z_offset) {
        for (int i = 0; i < points.size(); i++) {
            points[i][0] += x_offset;
            points[i][1] += y_offset;
            points[i][2] += z_offset;
        }

        this -> points = points;
        this -> indices = indices;

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

    settings_file.close();

    std::ifstream floor_file("data/floor.json");
    json floor_data = json::parse(floor_file);

    floor_file.close();

    std::vector<std::vector<short>> floor_points = floor_data["points"];
    std::vector<std::vector<std::vector<short>>> floor_indices = floor_data["indices"];

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 24; j++) {
            Mesh block = Mesh(floor_points, floor_indices, floor_data["type"], 400 * j, 0, -400 * i);
            objects.push_back(block);
        }
    }

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

        std::vector<std::vector<std::vector<short>>> draw_data;

        for (Mesh obj: objects) {
            for (int i = 0; i < obj.indices.size(); i++) {
                std::vector<std::vector<short>> draw(2);

                for (int j = 0; j < 3; j++) {
                    std::vector<short> v_rel = obj.points[obj.indices[i][0][j]];

                    short drawX = v_rel[0] * (focal_length / v_rel[2]) + width / 2;
                    short drawY = v_rel[1] * (focal_length / v_rel[2]) + height / 2;

                    draw[0].push_back(drawX);
                    draw[1].push_back(drawY);
                }

                draw_data.push_back(draw);
            }
        }

        for (std::vector<std::vector<short>> data: draw_data) {
            filledPolygonRGBA(renderer, data[0].data(), data[1].data(), 3, 255, 120, 80, 255);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
