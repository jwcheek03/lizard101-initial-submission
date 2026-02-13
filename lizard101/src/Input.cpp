#include "Input.h"

const bool* Input::keyboardState = nullptr;

float  Input::s_mouseX = 0.0f;
float  Input::s_mouseY = 0.0f;
Uint32 Input::s_mouseButtons = 0;

void Input::update() {
    // SDL_PumpEvents(); to use SDL's built in updating [Not used]

    keyboardState = SDL_GetKeyboardState(nullptr);

    // SDL3 returns a button bitmask and writes the cursor position
    s_mouseButtons = SDL_GetMouseState(&s_mouseX, &s_mouseY);
}

bool Input::isKeyPressed(SDL_Scancode key) {
    return keyboardState && keyboardState[key];
}

void Input::getMousePosition(float& outX, float& outY) {
    outX = s_mouseX;
    outY = s_mouseY;
}

float Input::mouseX() { return s_mouseX; }
float Input::mouseY() { return s_mouseY; }

bool Input::isMouseButtonPressed(Uint32 sdlButtonMask) {
    return (s_mouseButtons & sdlButtonMask) != 0;
}

bool Input::isChordPressed(SDL_Scancode a, SDL_Scancode b) {
    return isKeyPressed(a) && isKeyPressed(b);
}
