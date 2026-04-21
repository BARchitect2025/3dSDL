import numpy as np
import pygame
import math
import copy
import json
import os


# Speed vars
speed = 1600
rotation_speed = 2


# Mesh Class
class Mesh:
    points = []
    indices = []
    obj_type = ""

    def __init__(self, points, indices, offsetx, offsety, offsetz, obj_type):
        self.points = points
        self.indices = indices
        self.obj_type = obj_type

        for i in range(len(self.points)):
            self.points[i][0] += offsetx
            self.points[i][1] += offsety
            self.points[i][2] += offsetz

    
    def __str__(self):
        return str(self.indices)


# Raycasting
def ray_triangle_intersection(ray_origin, ray_vec, v0, v1, v2):
    epsilon = 0.000001
    edge1 = v1 - v0
    edge2 = v2 - v0
    pvec = np.cross(ray_vec, edge2)
    det = np.dot(edge1, pvec)
    
    if abs(det) < epsilon:
        return None
    
    inv_det = 1.0 / det
    tvec = ray_origin - v0
    u = np.dot(tvec, pvec) * inv_det
    if u < 0 or u > 1:
        return None
    
    qvec = np.cross(tvec, edge1)
    v = np.dot(ray_vec, qvec) * inv_det
    if v < 0 or u + v > 1:
        return None
    
    t = np.dot(edge2, qvec) * inv_det
    if t > epsilon:
        return t
    return None


# To find the directories of data files
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
settings_path = os.path.join(BASE_DIR, "settings.json")


# Load graphics settings
settings_data = ""
with open(settings_path, "r") as settings:
    settings_data = json.load(settings)
    settings.close()

fps_setting = settings_data["fps"]
draw_dist = settings_data["draw_dist"]

game_settings = pygame.FULLSCREEN if settings_data["fullscreen"] else 0


# Input floor mesh file
floor_path = os.path.join(BASE_DIR, "floor.json")

floor_data = ""
with open(floor_path, "r") as floor_file:
    floor_data = json.load(floor_file)
    floor_file.close()


# Input dark floor mesh file
dark_floor_path = os.path.join(BASE_DIR, "dark_floor.json")

dark_floor_data = ""
with open(dark_floor_path, "r") as dark_floor_file:
    dark_floor_data = json.load(dark_floor_file)
    dark_floor_file.close()


# Import beveled block thing
slimything_path = os.path.join(BASE_DIR, "slimything.json")

slimything_data = ""
with open(slimything_path, "r") as slimything_file:
    slimything_data = json.load(slimything_file)
    slimything_file.close()


# Initialize Pygame
pygame.init()
screen = pygame.display.set_mode((0, 0), game_settings)
clock = pygame.time.Clock()
running = True
dt = 0.01


# Get screen resolution
width = screen.get_width()
height = screen.get_height()


# Player vars
player_pos = np.array([0, 0, 0], dtype=float)
player_rotation = np.array([0, 0, 0], dtype=float)


# Prepare fonts to show up on the screen
font = pygame.font.SysFont(None, 30)
framerate = font.render("FPS: " + str(int(1 / dt)), False, (0, 0, 0))
position = font.render("POS: [" + str(int(player_pos[0])) + ", " + str(int(player_pos[1])) + ", " + str(int(player_pos[2])) + "]", False, (0, 0, 0))


# Get the data for the floors
floor_points = floor_data["points"]
floor_indices = floor_data["indices"]
floor_type = floor_data["type"]
# No dark_floor_points, we just need the indices for the colors
dark_floor_indices = dark_floor_data["indices"]

objects = []

i = 0
j = 0
while i < 24:
    while j < 24:
        block = 0

        if (i + j) % 2 == 0:
            block = Mesh(copy.deepcopy(floor_points), copy.deepcopy(floor_indices), j * 400, 0, i * -400, floor_type)
        elif (i + j) % 2 != 0:
            block = Mesh(copy.deepcopy(floor_points), copy.deepcopy(dark_floor_indices), j * 400, 0, i * -400, floor_type)
        objects.append(copy.deepcopy(block))
        j += 1
    i += 1
    j = 0

objects.append(Mesh(copy.deepcopy(slimything_data["points"]), copy.deepcopy(slimything_data["indices"]), 4000, 0, 4000, slimything_data["type"]))


# Set fov
FOV = 70


# Get screen rect to do frustum culling
scrRect = screen.get_rect()


# Rect for player movement
gameMap = pygame.Rect(-4800, -4800, 9600, 9600)


# Hide the player's cursor
pygame.mouse.set_visible(False)


# Finally! The game loop!
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # Wipe everything
    screen.fill("white")


    # Prepare the text to be blitted later
    framerate = font.render("FPS: " + str(int(1 / dt)), False, (0, 0, 0))
    position = font.render("POS: [" + str(int(player_pos[0])) + ", " + str(int(player_pos[1])) + ", " + str(int(player_pos[2])) + "]", False, (0, 0, 0))


    # Get cosines and sines of player rotation
    cos_y, sin_y = math.cos(-player_rotation[1]), math.sin(-player_rotation[1])
    cos_x, sin_x = math.cos(-player_rotation[0]), math.sin(-player_rotation[0])


    # Rotation arrays
    rot_y = np.array([[cos_y, 0, sin_y], [0, 1, 0], [-sin_y, 0, cos_y]])
    rot_x = np.array([[1, 0, 0], [0, cos_x, -sin_x], [0, sin_x, cos_x]])
    camera_matrix = np.dot(rot_x, rot_y)

    focal_length = (width / 2) / math.tan(math.radians(FOV / 2))

    obj_data = []

    obj_num = 0
    for obj in objects:
        for triangle_data in obj.indices:
            draw_data = []
            vertex_data = []
            
            for i in range(3):
                # World Space to Camera Space
                v = np.array(obj.points[triangle_data[i]], dtype=float) - player_pos
                v_rel = np.dot(camera_matrix, v)

                if v_rel[2] > 10 and v_rel[2] < draw_dist:
                    drawX = v_rel[0] * (focal_length / v_rel[2]) + width / 2
                    drawY = v_rel[1] * (focal_length / v_rel[2]) + height / 2

                    draw_data.append([drawX, drawY])
                    vertex_data.append(v_rel)
                else:
                    break
            if len(vertex_data) == 3:
                z_avg = (vertex_data[0][2] + vertex_data[1][2] + vertex_data[2][2]) / 3

                obj_data.append({"draw": draw_data, "vect": vertex_data, "color": triangle_data[3], "z_avg": z_avg})

    for i in range(len(obj_data)):
        for j in range(len(obj_data)):
            if obj_data[j]["z_avg"] < obj_data[i]["z_avg"]:
                temp = obj_data[i]
                obj_data[i] = obj_data[j]
                obj_data[j] = temp
        
    for tri in obj_data:
        if scrRect.collidepoint(tri["draw"][0]) or scrRect.collidepoint(tri["draw"][1]) or scrRect.collidepoint(tri["draw"][2]):
            pygame.draw.polygon(screen, tri["color"], tri["draw"])

    pygame.draw.line(screen, [0, 0, 0], [width / 2 - 8, height / 2], [width / 2 + 9, height / 2], 4)
    pygame.draw.line(screen, [0, 0, 0], [width / 2, height / 2 - 8], [width / 2, height / 2 + 9], 4)
    
    screen.blit(framerate, (10, 10))
    screen.blit(position, (10, 50))

    keys = pygame.key.get_pressed()

    mouse_rel = np.array(pygame.mouse.get_pos(), dtype = float) - np.array([width / 2, height / 2], dtype = float)

    player_rotation[0] -= mouse_rel[1] * (rotation_speed / 8) * dt
    player_rotation[1] += mouse_rel[0] * (rotation_speed / 8) * dt

    pygame.mouse.set_pos([width / 2, height / 2])

    if keys[pygame.K_LEFT]:
        player_rotation[1] -= rotation_speed * dt
    if keys[pygame.K_RIGHT]:
        player_rotation[1] += rotation_speed * dt
    if keys[pygame.K_UP]:
        player_rotation[0] += rotation_speed * dt
    if keys[pygame.K_DOWN]:
        player_rotation[0] -= rotation_speed * dt

    max_pitch = math.radians(89)
    player_rotation[0] = max(min(player_rotation[0], max_pitch), -max_pitch)

    yaw = player_rotation[1]
    forward = np.array([math.sin(yaw), 0, math.cos(yaw)], dtype=float)
    right = np.array([math.cos(yaw), 0, -math.sin(yaw)], dtype=float)

    move = np.array([0.0, 0.0, 0.0])

    if keys[pygame.K_w]: move += forward
    if keys[pygame.K_s]: move -= forward
    if keys[pygame.K_d]: move += right
    if keys[pygame.K_a]: move -= right

    if gameMap.collidepoint((player_pos + (move * speed * dt))[0], (player_pos + (move * speed * dt))[2]):
        player_pos += move * speed * dt
    
    mouse_buttons = pygame.mouse.get_pressed()
    if mouse_buttons[0]:
        ray_direction = np.dot(camera_matrix.T, np.array([0, 0, 1]))

        closest_hit = float('inf')
        hit_obj = None

        for obj in objects:
            for triangle_indices in obj.indices:
                v0 = np.array(obj.points[triangle_indices[0]])
                v1 = np.array(obj.points[triangle_indices[1]])
                v2 = np.array(obj.points[triangle_indices[2]])

                dist = ray_triangle_intersection(player_pos, ray_direction, v0, v1, v2)
                
                if dist and dist < closest_hit:
                    closest_hit = dist
                    hit_obj = obj
            
            if hit_obj != None:
                break

        if hit_obj:
            if hit_obj.obj_type == "enemy":
                objects.remove(hit_obj)

    if keys[pygame.K_ESCAPE]:
        running = False

    # flip() the display to put your work on screen
    pygame.display.flip()

    # limits FPS to 60
    # dt is delta time in seconds since last frame, used for framerate-
    # independent physics.
    dt = clock.tick(fps_setting) / 1000

pygame.quit()