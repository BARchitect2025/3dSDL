#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL2_framerate.h>
#include <SDL2/SDL_ttf.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

#include <math.h>
#include <vector>
#include <eigen3/Eigen/Dense>
#include <algorithm>

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

struct Triangle {
    SDL_Vertex vertices[3];
    double average_z;
};

namespace VecMath {
    struct Vector3 {
        double x, y, z;
        Vector3 operator - (const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    };
    
    // Standard math helper functions
    double dot(Vector3 a, Vector3 b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    Vector3 cross(Vector3 a, Vector3 b) {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }
    
    bool RayTriangleIntersect(Vector3 rayOrigin, Vector3 rayVector, Vector3 v0, Vector3 v1, Vector3 v2, float& t, float& u, float& v) {
        const float EPSILON = 0.0000001f;
        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 h = cross(rayVector, edge2);
        float a = dot(edge1, h);
    
        // If 'a' is near zero, the ray is parallel to the triangle
        if (a > -EPSILON && a < EPSILON) return false;
    
        float f = 1.0f / a;
        Vector3 s = rayOrigin - v0;
        u = f * dot(s, h);
    
        if (u < 0.0f || u > 1.0f) return false;
    
        Vector3 q = cross(s, edge1);
        v = f * dot(rayVector, q);
    
        if (v < 0.0f || u + v > 1.0f) return false;
    
        // Calculate t to find where the intersection point is on the line
        t = f * dot(edge2, q);
    
        return t > EPSILON; // Ray intersection
    }
}

const short FOV = 94;
const float PI = 3.1415;

const double speed = 1600.0;
const double rotation_speed = 1.5;

int main(int argc, char* argv[]) {
    MatrixXd player_pos(1, 3);
    MatrixXd player_rotation(1, 3);

    player_pos(0, 0) = 0;
    player_pos(0, 1) = 0;
    player_pos(0, 2) = 0;

    player_rotation(0, 0) = 0;
    player_rotation(0, 1) = 0;
    player_rotation(0, 2) = 0;

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
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
    if (!font) cout << "Font error: " << TTF_GetError() << endl;

    SDL_Color color = {255, 255, 255, 255}; // White
    SDL_Texture* fpsTexture = nullptr; // Position

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    bool running = true;

    SDL_Event event;

    Uint32 last_time = 0;
    float delta = 0.0f;
    short frameCount = 0;
    int fps = 0;

    vector<vector<vector<short>>> draw_data;
    draw_data.reserve(2000);

    MatrixXd move(1, 3);

    MatrixXd rot_x(3, 3);
    MatrixXd rot_y(3, 3);
    MatrixXd camera_matrix(3, 3);

    float cos_y, cos_x, sin_y, sin_x;

    vector<Triangle> triangles_to_draw;
    triangles_to_draw.reserve(2000); 

    while (running) {
        frameCount++;

        if (frameCount == 240) {
            char fps_text[32];
            sprintf(fps_text, "FPS: %d", fps);

            if (fpsTexture != nullptr) {
                SDL_DestroyTexture(fpsTexture);
            }

            SDL_Surface* surf = TTF_RenderText_Solid(font, fps_text, {255, 255, 255});
            fpsTexture = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);

            frameCount = 0;
        }
        
        Uint32 current_time = SDL_GetTicks();

        delta = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        cos_y = cos(-player_rotation(0, 1));
        sin_y = sin(-player_rotation(0, 1));
        cos_x = cos(-player_rotation(0, 0));
        sin_x = sin(-player_rotation(0, 0));

        rot_y(0, 0) = cos_y;  rot_y(0, 1) = 0; rot_y(0, 2) = sin_y;
        rot_y(1, 0) = 0;      rot_y(1, 1) = 1; rot_y(1, 2) = 0;
        rot_y(2, 0) = -sin_y; rot_y(2, 1) = 0; rot_y(2, 2) = cos_y;

        rot_x(0, 0) = 1; rot_x(0, 1) = 0;     rot_x(0, 2) = 0;
        rot_x(1, 0) = 0; rot_x(1, 1) = cos_x; rot_x(1, 2) = -sin_x;
        rot_x(2, 0) = 0; rot_x(2, 1) = sin_x; rot_x(2, 2) = cos_x;

        camera_matrix = rot_x * rot_y;


        move(0, 0) = 0;
        move(0, 1) = 0;
        move(0, 2) = 0;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int index = 0;
                    int closest_index = -1;
                    
                    VecMath::Vector3 ray_direction;

                    Vector3d mat = camera_matrix.transpose() * Vector3d(0, 0, 1);

                    float closest_hit = numeric_limits<float>::max();

                    float dist, intersectX, intersectY;

                    for (const auto& obj: objects) {
                        for (const auto& triangle_indices: obj.indices) {
                            VecMath::Vector3 v0 {obj.points[0][triangle_indices[0][0]]};
                            VecMath::Vector3 v1 {obj.points[0][triangle_indices[0][1]]};
                            VecMath::Vector3 v2 {obj.points[0][triangle_indices[0][2]]};

                            VecMath::RayTriangleIntersect(
                                VecMath::Vector3 {player_pos(0, 0), player_pos(0, 1), player_pos(0, 2)},
                                VecMath::Vector3 {mat(0), mat(0), mat(0)},
                                v0, v1, v2, dist, intersectX, intersectY
                            );

                            if (dist && dist < closest_hit) {
                                closest_hit = dist;
                                closest_index = index;
                            }
                        }
                        index++;
                    }

                    if (closest_index != -1) {
                        objects.erase(objects.begin() + closest_index);
                    }

                    cout << "Index: " << closest_index << " Distance: " << dist << endl;
                }
            }
        }

        const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

        if (keyboard[SDL_SCANCODE_ESCAPE]) running = false;

        float yaw = player_rotation(0, 1);
        MatrixXd forward(1, 3);
        MatrixXd right(1, 3);

        forward(0, 0) = sin(yaw);
        forward(0, 1) = 0;
        forward(0, 2) = cos(yaw);

        right(0, 0) = cos(yaw);
        right(0, 1) = 0;
        right(0, 2) = -sin(yaw);

        if (keyboard[SDL_SCANCODE_W]) move += forward;
        if (keyboard[SDL_SCANCODE_S]) move -= forward;
        if (keyboard[SDL_SCANCODE_A]) move -= right;
        if (keyboard[SDL_SCANCODE_D]) move += right;

        player_pos += move * delta * speed;
        if (!(player_pos(0, 0) > -4800 && player_pos(0, 0) < 4800 && player_pos(0, 2) > -4800 && player_pos(0, 2) < 4800)) {
            player_pos -= move * delta * speed;
        }

        if (keyboard[SDL_SCANCODE_LEFT]) player_rotation(0, 1) -= rotation_speed * delta;
        if (keyboard[SDL_SCANCODE_RIGHT]) player_rotation(0, 1) += rotation_speed * delta;
        if (keyboard[SDL_SCANCODE_UP]) player_rotation(0, 0) += rotation_speed * delta;
        if (keyboard[SDL_SCANCODE_DOWN]) player_rotation(0, 0) -= rotation_speed * delta;

        double max_pitch = 89.0 * (PI / 180.0);

        player_rotation(0, 0) = clamp(player_rotation(0, 0), -max_pitch, max_pitch);

        fps = 1.0f / delta;

        draw_data.clear();

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderClear(renderer);

        triangles_to_draw.clear();
        float focal_length = (width / 2.0f) / tan((FOV / 2.0f) * (PI / 180.0f));

        for (const auto& obj : objects) {
            for (const auto& index_data : obj.indices) {
                Triangle tri;
                double z_sum = 0;
                bool visible = true;
            
                for (int j = 0; j < 3; j++) {
                    double worldX = obj.points[index_data[0][j]][0] - player_pos(0, 0);
                    double worldY = obj.points[index_data[0][j]][1] - player_pos(0, 1);
                    double worldZ = obj.points[index_data[0][j]][2] - player_pos(0, 2);
                
                    double relX = camera_matrix(0, 0) * worldX + camera_matrix(0, 1) * worldY + camera_matrix(0, 2) * worldZ;
                    double relY = camera_matrix(1, 0) * worldX + camera_matrix(1, 1) * worldY + camera_matrix(1, 2) * worldZ;
                    double relZ = camera_matrix(2, 0) * worldX + camera_matrix(2, 1) * worldY + camera_matrix(2, 2) * worldZ;
                
                    if (relZ > 10 && relZ < draw_dist) {
                        tri.vertices[j].position.x = relX * (focal_length / relZ) + width / 2.0f;
                        tri.vertices[j].position.y = relY * (focal_length / relZ) + height / 2.0f;
                        tri.vertices[j].color = { (Uint8)index_data[1][0], (Uint8)index_data[1][1], (Uint8)index_data[1][2], 255 };
                        tri.vertices[j].tex_coord = {0, 0};
                        z_sum += relZ;
                    } else {
                        visible = false;
                        break;
                    }
                }
            
                if (visible) {
                    tri.average_z = z_sum / 3.0;
                    triangles_to_draw.push_back(tri);
                }
            }
        }


        sort(triangles_to_draw.begin(), triangles_to_draw.end(), [](const Triangle& a, const Triangle& b) {
            return a.average_z > b.average_z; 
        });

        for (const auto& tri : triangles_to_draw) {
            SDL_RenderGeometry(renderer, NULL, tri.vertices, 3, NULL, 0);
        }

        if (fpsTexture != nullptr) {
            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(fpsTexture, NULL, NULL, &texW, &texH);
            SDL_Rect dstRect = { 20, 20, texW, texH }; // Position at top-left
            SDL_RenderCopy(renderer, fpsTexture, NULL, &dstRect);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(fpsTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}