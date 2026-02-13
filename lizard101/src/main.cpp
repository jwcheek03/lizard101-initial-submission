#include "main.h"
#include "Input.h"

#include "Physics.h"
#include "Timeline.h"
#include "events/EventManager.h"
#include "events/Event.h"
#include "EntityManager.h"
#include "game/Lizard101Controller.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <atomic>
#include <iostream>

// Global debug toggle (declared in main.h)
bool gEventLogEnabled = false;

// main.h declares this as extern, we keep the definition here:
std::atomic<int> pauseRequested{ 0 };

int main(int, char**) {
    // SDL core init
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Lizard101",
        1920, 1080,
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!window || !renderer) {
        std::cerr << "Failed to create SDL window/renderer: "
            << SDL_GetError() << "\n";
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window)   SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Core engine systems
    PhysicsSystem   physics;
    Timeline        gameTimeline(1.0f);
    EventManager    eventManager(gameTimeline);
    EntityManager& manager = EntityManager::getInstance();

    // High-level game controller (menus + deck select + combat)
    Lizard101Controller controller(renderer, physics, gameTimeline, eventManager, manager);

    bool      running = true;
    SDL_Event ev{};

    while (running) {
        // Basic SDL event handling TODO:|WE WILL NEED TO CHANGE THIS!!! (actually technically we don't but we really should)|
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Keyboard / mouse
        Input::update();

        // Timeline / delta
        gameTimeline.update();
        float dt = gameTimeline.getDeltaTime();

        // Background
        SDL_SetRenderDrawColor(renderer, 0, 0, 32, 255);
        SDL_RenderClear(renderer);

        // Let controller drive the game
        controller.update(dt);
        controller.render();

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    manager.destroyAll();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
