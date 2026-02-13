#include "PauseButton.h"
#include "Input.h"
#include "game/components/TextureRenderer.h"
#include "game/Camera.h"

std::function<void()> PauseButton::onPauseRequested = nullptr;

PauseButton::PauseButton(std::string name, float screenX, float screenY,
    int w, int h, Timeline* timeline)
    : Entity(std::move(name), screenX, screenY, w, h,
        /*tex*/ nullptr,
        /*physics*/ false,
        /*solid*/ false,
        "PAUSEBUTTON"),
    timeline(timeline),
    screenX_(screenX),
    screenY_(screenY) {
}

void PauseButton::update(float deltaTime) {
    Entity::update(deltaTime);

    // Make sure the pause button follows the camera and stays in the same screen position
    float camX = Camera::getInstance().getX();
    float camY = Camera::getInstance().getY();
    x = screenX_ + camX;
    y = screenY_ + camY;

    // Mouse is reported in screen coordinates; check against screen-space bounds for the button
    float mx = Input::mouseX();
    float my = Input::mouseY();
    bool inBounds = (mx >= screenX_ && mx <= screenX_ + width && my >= screenY_ && my <= screenY_ + height);
    bool clicked = Input::isMouseButtonPressed(SDL_BUTTON_LMASK);



    if (inBounds && clicked && !wasPressed) {
        SDL_Log("PauseButton: mouse=(%f,%f) inBounds=%d clicked=%d wasPressed=%d", mx, my, (int)inBounds, (int)clicked, (int)wasPressed);
        if (PauseButton::onPauseRequested) PauseButton::onPauseRequested();
        SDL_Log("Pause requested (callback invoked)");
        wasPressed = true;
    }

    if (!clicked) wasPressed = false;
}
