cmake_minimum_required(VERSION 3.10)
project(MySDLGame)

# 1. Look for SDL2 package
# On Linux, this finds the system-installed SDL2
# On Windows, you may need to point to your SDL2 folder (see Step 3)
find_package(SDL2 REQUIRED)

# 2. Add your source code
add_executable(MySDLGame main.cpp)

# 3. Link SDL2 to your program
target_link_libraries(MySDLGame SDL2::SDL2 SDL2_gfx)
