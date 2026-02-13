cmake_minimum_required(VERSION 3.10)
project(GameEngine)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# SDL3
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/External/SDL3")

# SDL_image
set(SDL_IMAGE_DIR "${CMAKE_SOURCE_DIR}/External/SDL_image")

# Gather all .cpp files in src and its subdirectories
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# Main/game executable
add_executable(main
    src/main.cpp
    src/entity.cpp
    src/entityManager.cpp
    src/Input.cpp
    src/game/Player.cpp
    src/game/PauseButton.cpp
    src/game/movingPlatform.cpp
    src/game/stationaryPlatform.cpp
    src/Timeline.cpp
    src/physics.cpp
    src/JobSystem.cpp
    src/events/EventManager.cpp
    src/game/Lizard101Core.cpp
    src/game/Lizard101Controller.cpp
    # ...add any other files needed for main
)

# Server executable
add_executable(server
    src/server.cpp
    src/entity.cpp
    src/entityManager.cpp
    src/game/movingPlatform.cpp
    src/Timeline.cpp
    src/Input.cpp
    src/JobSystem.cpp
    src/game/PauseButton.cpp
    src/events/EventManager.cpp
    # ...add any other files needed for server
)

# Client executable
add_executable(client
    src/client.cpp
    src/entity.cpp
    src/entityManager.cpp
    src/Input.cpp
    src/game/Player.cpp
    src/game/movingPlatform.cpp
    src/game/stationaryPlatform.cpp
    src/Timeline.cpp
    src/physics.cpp
    src/JobSystem.cpp
    src/game/PauseButton.cpp
    src/events/EventManager.cpp
    # ...add any other files needed for client
)

# Include directories
target_include_directories(main PRIVATE
    ${SDL3_DIR}/include
    ${SDL_IMAGE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/game
)
target_include_directories(server PRIVATE
    ${SDL3_DIR}/include
    ${SDL_IMAGE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/game
)
target_include_directories(client PRIVATE
    ${SDL3_DIR}/include
    ${SDL_IMAGE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/game
)

# Link libraries
target_link_libraries(main PRIVATE SDL3 SDL3_image zmq)
target_link_libraries(client PRIVATE SDL3 SDL3_image zmq)
target_link_libraries(server PRIVATE SDL3 SDL3_image zmq)

# Copy DLLs to output
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SDL3_DIR}/bin/SDL3.dll
    ${SDL_IMAGE_DIR}/bin/SDL3_image.dll
    $<TARGET_FILE_DIR:main>)

# Copy media folder to output
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/media
    $<TARGET_FILE_DIR:main>/media
)
