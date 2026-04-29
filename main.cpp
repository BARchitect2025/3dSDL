#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

#include <math.h>
#include <vector>
#include <eigen3/Eigen/Dense>

using namespace std;
using namespace Eigen;

struct Mesh {
    vector<vector<double>> points;
    vector<vector<vector<double>>> indices;
    string type;

    Mesh(vector<vector<double>> points, vector<vector<vector<double>>> indices, string type, double x_offset, double y_offset, double z_offset) {
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

const double speed = 800.0;

int main(int argc, char* argv[]) {
    MatrixXd player_pos(1, 3);
    player_pos(0, 0) = 0;
    player_pos(0, 1) = 0;
    player_pos(0, 2) = 0;

    vector<Mesh> objects;

    // Input settings and model files
    ifstream settings_file("data/settings.json");
    json settings = json::parse(settings_file);
    
    short draw_dist = settings["draw_dist"];

    settings_file.close();

    ifstream floor_file("data/floor.json");
    json floor_data = json::parse(floor_file);

    floor_file.close();

    ifstream dark_floor_file("data/dark_floor.json");
    json dark_floor_data = json::parse(dark_floor_file);

    dark_floor_file.close();

    // Get points and indices from the files that were input
    vector<vector<double>> floor_points = floor_data["points"];
    vector<vector<vector<double>>> floor_indices = floor_data["indices"];
    vector<vector<vector<double>>> dark_floor_indices = dark_floor_data["indices"];

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 24; j++) {
            Mesh block = (((i + j) % 2 == 0)? (Mesh(floor_points, floor_indices, floor_data["type"], 400 * j, 0, -400 * i)): (Mesh(floor_points, dark_floor_indices, floor_data["type"], 400 * j, 0, -400 * i)));
            objects.push_back(block);
        }
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("3dSDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    if (!font) std::cout << "Font error: " << TTF_GetError() << std::endl;

    SDL_Color color = {255, 255, 255, 255}; // White
    SDL_Surface* surface = TTF_RenderText_Solid(font, "Hello SDL2", color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect dstRect = {100, 100, surface->w, surface->h}; // Position

    SDL_FreeSurface(surface);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    bool running = true;

    SDL_Event event;

    Uint32 last_time = 0;
    float delta = 0.0f;

    while (running) {
        Uint32 current_time = SDL_GetTicks();

        delta = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_w) player_pos(0, 2) += speed * delta;
                if (event.key.keysym.sym == SDLK_s) player_pos(0, 2) -= speed * delta;
                if (event.key.keysym.sym == SDLK_a) player_pos(0, 0) -= speed * delta;
                if (event.key.keysym.sym == SDLK_d) player_pos(0, 0) += speed * delta;
            }
        }

        std::cout << (1.0f / delta) << endl;

        float focal_length = (width / 2.0f) / tan((FOV / 2.0f) * (PI / 180.0f));

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderClear(renderer);

        vector<vector<vector<short>>> draw_data;

        for (Mesh obj: objects) {
            for (int i = 0; i < obj.indices.size(); i++) {
                vector<vector<short>> draw(3);

                for (int j = 0; j < 3; j++) {
                    MatrixXd v_rel(1, 3);
                    v_rel(0, 0) = obj.points[obj.indices[i][0][j]][0] - player_pos(0, 0);
                    v_rel(0, 1) = obj.points[obj.indices[i][0][j]][1] - player_pos(0, 1);
                    v_rel(0, 2) = obj.points[obj.indices[i][0][j]][2] - player_pos(0, 2);

                    if (v_rel(0, 2) > 10) {
                        short drawX = v_rel(0, 0) * (focal_length / v_rel(0, 2)) + width / 2.0f;
                        short drawY = v_rel(0, 1) * (focal_length / v_rel(0, 2)) + height / 2.0f;

                        draw[0].push_back(drawX);
                        draw[1].push_back(drawY);
                        draw[2].push_back(obj.indices[i][1][j]);
                    } else {
                        break;
                    }
                }

                if (draw[0].size() == 3) {
                    draw_data.push_back(draw);
                }
            }
        }

        for (vector<vector<short>> data: draw_data) {
            filledPolygonRGBA(renderer, data[0].data(), data[1].data(), 3, data[2][0], data[2][1], data[2][2], 255);
        }

        SDL_RenderCopy(renderer, texture, NULL, &dstRect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
