#pragma once
#include "entityManager.h"
#include "entity.h"

#include <iostream>   // added for debug printing
#include <string>     // added for debug printing

// Toggle is defined in main.cpp
extern bool gEventLogEnabled;

// Tiny helper for "raised" event logs (no spam when disabled)
inline void EVENT_LOG(const std::string& s) {
    if (gEventLogEnabled) { std::cout << s << std::endl; }
}

const int WINDOW_WIDTH = 1920;  /*  Width of the game window to be created*/
const int WINDOW_HEIGHT = 1080; /*  Height of the game window to be created*/
const int FRAME_COUNT = 8;     /*  Number of frames in the spritesheet */
const int FRAME_WIDTH = 128;   /*  Width of the frame in the spritesheet */
const int FRAME_HEIGHT = 128;  /*  Height of the frame in the spritesheet */
const int ANIMATION_DELAY = 100;/* Number of iterations between the animation frames (determines delay) */

/* Struct to store the current state*/
struct AppState {
    SDL_Texture* Texture = nullptr;
    int currentFrame = 0;
    Uint32 lastFrameTime = 0;
};
