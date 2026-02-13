#pragma once

#include <vector>
#include <SDL3/SDL.h>

#include "game/Lizard101Core.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Timeline.h"
#include "events/EventManager.h"

// Forward declarations
class TextureRenderer;
class BoxCollider;
class DebugRenderer;

enum class GameState {
    MainMenu,
    DeckSelect,
    Gameplay,
    CardReward,
};

// Visual mapping for card + hand index
struct CardVisual {
    Entity* entity = nullptr;
    size_t  handIndex = 0;
};

// Visual mapping for heroes
struct HeroVisual {
    Entity* entity = nullptr;
    int deckType = 0; // 0=Plant,1=Fire,2=Water
};

class Lizard101Controller {
public:
    Lizard101Controller(SDL_Renderer* renderer,
        PhysicsSystem& physics,
        Timeline& timeline,
        EventManager& eventManager,
        EntityManager& entityManager);

    // Called once per frame, after Input::update(), before generic engine update/physics.
    void update(float dt);

    // Called once per frame for rendering. This just relies on EntityManager::renderAll;
    // we expose it in case you want controller-specific overlays later.
    void render();

    // Access to current game state (for debugging / UI)
    GameState gameState() const { return gameState_; }

private:
    // Core engine references
    SDL_Renderer* renderer_;
    PhysicsSystem& physics_;
    Timeline& timeline_;
    EventManager& eventManager_;
    EntityManager& manager_;

    // High-level state
    GameState gameState_ = GameState::MainMenu;

    int selectedNumPlayers_ = 0;
    int currentDeckSelectPlayer_ = 0;
    std::vector<int> playerDeckChoices_; // 0 = Plant, 1 = Fire, 2 = Water

    CardGameState cardGame_;
    bool          cardGameInitialized_ = false;

    Entity* backgroundEntity_ = nullptr;

    // Entities
    std::vector<Entity*> menuButtonEntities_;
    std::vector<Entity*> enemyEntities_;
    std::vector<Entity*> enemyOverlays_;
    std::vector<Entity*> playerOverlays_;
    std::vector<Entity*> enemyAuraOverlays_;
    std::vector<Entity*> playerAuraOverlays_;
    std::vector<HeroVisual> heroVisuals_;
    std::vector<CardVisual> handVisuals_;
    std::vector<Entity*> manaSymbolEntities_;
    Entity* deckEntity_ = nullptr;
    Entity* playZoneEntity_ = nullptr;

    // Drag state for cards
    CardVisual* draggedVisual_ = nullptr;
    Entity* draggedCard_ = nullptr;
    float       dragOffsetX_ = 0.0f;
    float       dragOffsetY_ = 0.0f;

    // Deck select click gating
    bool waitingForRelease_ = true;
    bool prevMouseLDeckSelect_ = false;

    // Keyboard state for end-turn / debug play
    bool prevKeyE_ = false;
    bool prevKey1_ = false;
    bool prevKey2_ = false;
    bool prevKey3_ = false;
    bool prevKey4_ = false;
    bool prevKey5_ = false;
    bool prevToggleLog_ = false;

    //  Card level tracking
    int currentLevel_ = 1;
    int cardRewardRound_ = 0;
    const int maxLevels_ = 3;
    const int maxCardRewardRounds_ = 2;

    std::vector<std::vector<const CardDefinition*>> cardRewardChoices_; // [player][3 choices]
    std::vector<int> cardRewardSelections_; // -1 if not picked yet

    // --- Internal helpers ---
    void updateBackground();

    // Top-level per-state updates
    void updateMainMenu();
    void updateDeckSelect();
    void updateGameplay(float dt);

    // One-time gameplay init when entering Gameplay state
    void initGameplay();

    // Visual helpers
    void rebuildHandVisuals();
    void ensureDeathOverlays();
    void ensureAuraOverlays();

    // Game-end & reset
    bool allPlayersDowned() const;
    bool allEnemiesDefeated() const;
    bool checkGameEnd();
    void resetGameState();
    void clearAllVisualsAndEntities();

    // Card game reward
    std::vector<const CardDefinition*> getRandomCardChoices();
    void updateCardReward();

    // Convenience
    bool canActivePlayerAct();

    // Deck-building helper for enemies
    void buildMonoDeck(School school, std::vector<CardInstance>& out, int& nextInstanceId);

    // Colors for card debug overlays
    SDL_Color colorForCard(const CardInstance& ci) const;
    SDL_Color colorForDeckType(int deckType) const;

    // Event subscriptions
    void setupEventSubscriptions();
};
