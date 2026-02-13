#pragma once
#include <SDL3/SDL.h>

class Input {
public:
    static void update();
    static bool isKeyPressed(SDL_Scancode key);

    static void getMousePosition(float& outX, float& outY);
    static float mouseX();
    static float mouseY();

    // mouse buttons
    static bool isMouseButtonPressed(Uint32 sdlButtonMask);
    static bool isChordPressed(SDL_Scancode a, SDL_Scancode b);


private:
    static const bool* keyboardState;

    static float s_mouseX;
    static float s_mouseY;
    static Uint32 s_mouseButtons;
};
