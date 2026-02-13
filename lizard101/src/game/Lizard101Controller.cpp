#include "game/Lizard101Controller.h"

#include "main.h"              // gEventLogEnabled / EVENT_LOG again, should in theory be changed, but fine for now
#include "Input.h"
#include "game/components/TextureRenderer.h"
#include "game/components/BoxCollider.h"
#include "game/components/DebugRenderer.h"
#include "Camera.h"
#include "events/Event.h"

#include <iostream>
#include <random>
#include <algorithm>
#include <filesystem>


// ----------------- ctor & setup -----------------

Lizard101Controller::Lizard101Controller(SDL_Renderer* renderer,
    PhysicsSystem& physics,
    Timeline& timeline,
    EventManager& eventManager,
    EntityManager& entityManager)
    : renderer_(renderer)
    , physics_(physics)
    , timeline_(timeline)
    , eventManager_(eventManager)
    , manager_(entityManager)
{
    // Camera bounds for card game
    Camera::getInstance().setBounds(0.0f, 0.0f, 1920.0f, 1080.0f);
    Camera::getInstance().setPosition(0.0f, 0.0f);

    setupEventSubscriptions();
}

// ----------------- Event subscriptions -----------------

void Lizard101Controller::setupEventSubscriptions() {
    // We don't need Collision for the card game right now, but keep hook if needed.
    eventManager_.subscribe(EventType::Collision, [](const Event& ev) {
        (void)ev;
        });

    // Drag handler: card dragging + targeting
    eventManager_.subscribe(EventType::Input, [this](const Event& ev) {
        if (cardGame_.enemyTurnPending) return;
        if (cardGame_.players.empty())  return;
        if (cardGame_.activePlayerIndex < 0 ||
            cardGame_.activePlayerIndex >= static_cast<int>(cardGame_.players.size())) {
            return;
        }

        auto* di = std::get_if<DragInfo>(&ev.payload);
        if (!di) return;

        const float mx = di->x;
        const float my = di->y;

        auto pointIn = [](const SDL_FRect& r, float px, float py) {
            return (px >= r.x && px <= r.x + r.w &&
                py >= r.y && py <= r.y + r.h);
            };

        if (di->phase == DragInfo::Phase::Start) {
            draggedCard_ = nullptr;
            draggedVisual_ = nullptr;

            for (auto& cv : handVisuals_) {
                Entity* e = cv.entity;
                if (!e) continue;
                auto* col = e->getComponent<BoxCollider>();
                if (!col) continue;
                SDL_FRect r = col->aabb();
                if (pointIn(r, mx, my)) {
                    draggedCard_ = e;
                    draggedVisual_ = &cv;
                    dragOffsetX_ = e->x - mx;
                    dragOffsetY_ = e->y - my;
                    break;
                }
            }
        }
        else if (di->phase == DragInfo::Phase::Move) {
            if (draggedCard_) {
                draggedCard_->x = mx + dragOffsetX_;
                draggedCard_->y = my + dragOffsetY_;
            }
        }
        else if (di->phase == DragInfo::Phase::End) {
            if (cardGame_.players.empty()) return;
            CombatPlayer& active = cardGame_.activePlayer();
            if (!canActivePlayerAct()) {
                std::cout << "[PLAYER] " << active.name << " is downed and cannot act!\n";
                draggedCard_ = nullptr;
                draggedVisual_ = nullptr;
                rebuildHandVisuals();
                ensureDeathOverlays();
                ensureAuraOverlays();
                return;
            }

            bool played = false;

            if (draggedCard_ && draggedVisual_) {
                size_t handIndex = draggedVisual_->handIndex;
                if (handIndex < active.hand.cards.size()) {
                    // IMPORTANT: do NOT hold a reference into the hand
                    CardInstance ci = active.hand.cards[handIndex];
                    TargetScope scope = ci.def ? ci.def->targetScope : TargetScope::Self;

                    CombatPlayer* targetPlayer = nullptr;
                    std::vector<CombatPlayer*> targets;

                    // --- Enemy targeting ---
                    if (scope == TargetScope::SingleEnemy ||
                        scope == TargetScope::AnySingle) {
                        for (size_t i = 0; i < enemyEntities_.size(); ++i) {
                            Entity* enemyEntity = enemyEntities_[i];
                            if (!enemyEntity) continue;
                            if (collisionDetection(*draggedCard_, *enemyEntity)) {
                                if (i < cardGame_.enemies.size()) {
                                    targetPlayer = &cardGame_.enemies[i];
                                    break;
                                }
                            }
                        }
                    }

                    // --- Ally/self targeting ---
                    if (!targetPlayer) {
                        for (size_t i = 0; i < heroVisuals_.size(); ++i) {
                            Entity* heroEntity = heroVisuals_[i].entity;
                            if (!heroEntity) continue;
                            if (collisionDetection(*draggedCard_, *heroEntity)) {
                                if (scope == TargetScope::Self &&
                                    &cardGame_.players[i] == &active) {
                                    targetPlayer = &active;
                                    break;
                                }
                                else if (scope == TargetScope::SingleAlly ||
                                    scope == TargetScope::AnySingle) {
                                    targetPlayer = &cardGame_.players[i];
                                    break;
                                }
                            }
                        }
                    }

                    // Fallback for pure Self cards
                    if (!targetPlayer && scope == TargetScope::Self && playZoneEntity_) {
                        if (collisionDetection(*draggedCard_, *playZoneEntity_)) {
                            targetPlayer = &active;
                        }
                    }

                    if (!targetPlayer) {
                        std::cout << "[PLAYER SPELL] Invalid target for '"
                            << (ci.def ? ci.def->name : "<unnamed>")
                            << "' (scope " << int(scope) << ")\n";
                    }
                    else {
                        targets.push_back(targetPlayer);

                        int cardCost = ci.def ? ci.def->manaCost : 0;
                        if (active.mana >= cardCost) {
                            bool ok = cardGame_.playCardFromHand(handIndex, targets);
                            if (ok) {
                                std::cout << "[PLAYER SPELL] Played '"
                                    << (ci.def ? ci.def->name : "<unnamed>")
                                    << "' via drag, handIndex " << handIndex
                                    << " on target " << targetPlayer->name << "\n";
                                played = true;
                            }
                        }
                        else {
                            std::cout << "[PLAYER SPELL] Not enough mana for '"
                                << (ci.def ? ci.def->name : "<unnamed>") << "'\n";
                        }
                    }
                }

            }

            draggedCard_ = nullptr;
            draggedVisual_ = nullptr;
            rebuildHandVisuals();
            ensureDeathOverlays();
            ensureAuraOverlays();
        }
        });
}

// ----------------- Color helpers -----------------

SDL_Color Lizard101Controller::colorForCard(const CardInstance& ci) const {
    if (!ci.def) return SDL_Color{ 200, 200, 200, 200 };
    switch (ci.def->school) {
    case School::Fire:   return SDL_Color{ 255, 80, 80, 200 };
    case School::Water:  return SDL_Color{ 80, 120, 255, 200 };
    case School::Plant:  return SDL_Color{ 80, 200, 80, 200 };
    case School::Generic:
    default:             return SDL_Color{ 180, 180, 180, 200 };
    }
}

// Remove once cards actually have textures!
SDL_Color Lizard101Controller::colorForDeckType(int deckType) const {
    switch (deckType) {
    case 0: return SDL_Color{ 80, 200, 80, 255 };   // Plant
    case 1: return SDL_Color{ 255, 80, 80, 255 };   // Fire
    case 2: return SDL_Color{ 80, 120, 255, 255 };  // Water
    default: return SDL_Color{ 180, 180, 180, 255 };
    }
}

void Lizard101Controller::updateBackground() {

    if (backgroundEntity_) {
        manager_.removeEntity(backgroundEntity_);
        backgroundEntity_ = nullptr;
    }

    std::string bgPath;
    switch (gameState_) {
    case GameState::MainMenu:
    case GameState::DeckSelect:
    case GameState::CardReward:
        bgPath = "media/menu/galaxy-bg.png";
        break;
    case GameState::Gameplay:
        if (currentLevel_ == 1)
            bgPath = "media/menu/level1-bg.png";
        else if (currentLevel_ == 2)
            bgPath = "media/menu/level2-bg.png";
        else if (currentLevel_ == 3)
            bgPath = "media/menu/level3-bg.jpg";
        else
            bgPath = "media/menu/galaxy-bg.png";
        break;
    default:
        bgPath = "media/menu/galaxy-bg.png";
        break;
    }

    // Create background entity (full screen)
    backgroundEntity_ = new Entity("Background", 0.0f, 0.0f, 1920, 1080);
    backgroundEntity_->setTag("BACKGROUND");
    backgroundEntity_->addComponent<TextureRenderer>(renderer_, bgPath);
    //manager_.entities.insert(manager_.entities.begin(), backgroundEntity_);
    manager_.addEntityToFront(backgroundEntity_);
}

// ----------------- Deck helper -----------------

void Lizard101Controller::buildMonoDeck(School school,
    std::vector<CardInstance>& out,
    int& nextInstanceId)
{
    out.clear();
    for (const auto& def : LizardCards::getAllCards()) {
        if (def.school == school) {
            for (int i = 0; i < 5; ++i) {
                out.push_back(CardInstance{ &def, nextInstanceId++ });
            }
        }
    }
    if (out.empty()) {
        for (const auto& def : LizardCards::getAllCards()) {
            if (def.school == School::Generic) {
                out.push_back(CardInstance{ &def, nextInstanceId++ });
            }
        }
    }
}

// ----------------- Game-end helpers -----------------

bool Lizard101Controller::allPlayersDowned() const {
    for (const auto& p : cardGame_.players) {
        if (p.hp > 0) return false;
    }
    return !cardGame_.players.empty();
}

bool Lizard101Controller::allEnemiesDefeated() const {
    for (const auto& e : cardGame_.enemies) {
        if (e.hp > 0) return false;
    }
    return !cardGame_.enemies.empty();
}

bool Lizard101Controller::canActivePlayerAct() {
    if (cardGame_.players.empty()) return false;
    const CombatPlayer& active = cardGame_.activePlayer();
    return active.hp > 0;
}

std::vector<const CardDefinition*> Lizard101Controller::getRandomCardChoices() {
    std::vector<const CardDefinition*> pool;
    for (const auto& def : LizardCards::getAllCards()) {
        if (def.school != School::Generic) {
            pool.push_back(&def);
        }
    }
    std::shuffle(pool.begin(), pool.end(), cardGame_.rng);
    std::vector<const CardDefinition*> choices;
    for (int i = 0; i < 3 && i < pool.size(); ++i) {
        choices.push_back(pool[i]);
    }
    return choices;
}

void Lizard101Controller::updateCardReward() {
    updateBackground();
    // Clear previous entities
    for (Entity* e : menuButtonEntities_) {
        manager_.removeEntity(e);
    }
    menuButtonEntities_.clear();

    bool curMouseL = Input::isMouseButtonPressed(SDL_BUTTON_LMASK);
    float mx = Input::mouseX();
    float my = Input::mouseY();

    // Find which player is picking
    int pickingPlayer = -1;
    for (size_t i = 0; i < cardRewardSelections_.size(); ++i) {
        if (cardRewardSelections_[i] == -1) {
            pickingPlayer = static_cast<int>(i);
            break;
        }
    }

    if (pickingPlayer != -1) {
        // Show label at top
        int labelW = 360, labelH = 160;
        float cx = 1920.0f * 0.5f;
        float labelX = cx - labelW * 0.5f;
        float labelY = 150.0f;
        Entity* labelEntity = new Entity("CardRewardPlayerLabel", labelX, labelY, labelW, labelH);
        labelEntity->setTag("CARD_REWARD_PLAYER_LABEL");
        std::string labelTexturePath =
            "media/menu/player_" + std::to_string(pickingPlayer + 1) + "_select.png";
        labelEntity->addComponent<TextureRenderer>(renderer_, labelTexturePath);
        manager_.addEntity(labelEntity);
        menuButtonEntities_.push_back(labelEntity);

        // Show 3 card choices
        int cardW = 180, cardH = 260;
        const float cardSpacing = 60.0f;
        float totalWidth = 3 * cardW + 2 * cardSpacing;
        float startX = cx - totalWidth * 0.5f;
        float cardY = 400.0f;

        auto& choices = cardRewardChoices_[pickingPlayer];
        for (int i = 0; i < 3; ++i) {
            float bx = startX + i * (cardW + cardSpacing);
            float by = cardY;
            Entity* cardEntity = new Entity("CardRewardButton", bx, by, cardW, cardH);
            cardEntity->setTag("CARD_REWARD_BUTTON");
            // Show card art or fallback color
            std::string texturePath = choices[i]->texturePath.empty() ?
                "media/darkworld_platform_mossystone.png" : choices[i]->texturePath;
            cardEntity->addComponent<TextureRenderer>(renderer_, texturePath);
            manager_.addEntity(cardEntity);
            menuButtonEntities_.push_back(cardEntity);

            // Click handling
            if (!waitingForRelease_ && curMouseL && !prevMouseLDeckSelect_) {
                if (mx >= bx && mx <= bx + cardW &&
                    my >= by && my <= by + cardH) {
                    cardRewardSelections_[pickingPlayer] = i;
                    // Add 3 copies to player's deck
                    int nextInstanceId = static_cast<int>(cardGame_.players[pickingPlayer].deck.cards.size()) + 1;
                    for (int c = 0; c < 3; ++c) {
                        cardGame_.players[pickingPlayer].deck.cards.push_back(CardInstance{ choices[i], nextInstanceId++ });
                    }
                    cardGame_.players[pickingPlayer].earnedRewardCards.push_back(choices[i]);
                    waitingForRelease_ = true;
                    break;
                }
            }
        }

        if (!curMouseL && waitingForRelease_) {
            waitingForRelease_ = false;
        }
    }

    // If all players have picked, advance to next round or level
    bool allPicked = std::all_of(cardRewardSelections_.begin(), cardRewardSelections_.end(),
        [](int sel) { return sel != -1; });
    if (allPicked) {
        std::cout << "[card reward] All players picked cards. Advancing...\n";
        clearAllVisualsAndEntities();
        // Print deck sizes for debugging
        for (size_t i = 0; i < cardGame_.players.size(); ++i) {
            std::cout << "Player " << (i + 1) << " deck size: "
                << cardGame_.players[i].deck.cards.size() << "\n";
        }
        if (cardRewardRound_ < maxCardRewardRounds_) {
            cardRewardRound_++;
            if (cardRewardRound_ < maxCardRewardRounds_) {
                // Prepare next round of card rewards
                cardRewardChoices_.clear();
                cardRewardSelections_.assign(cardGame_.players.size(), -1);
                for (size_t i = 0; i < cardGame_.players.size(); ++i) {
                    cardRewardChoices_.push_back(getRandomCardChoices());
                }
            }
            else {
                // Finished all card reward rounds, advance to next level or end game
                if (currentLevel_ < maxLevels_) {
                    currentLevel_++;
                    cardRewardRound_ = 0;
                    gameState_ = GameState::Gameplay;
                    cardGameInitialized_ = false;
                }
                else {
                    std::cout << "[GAME END] All levels won! Lizards win!\n";
                    manager_.destroyAll();
                    resetGameState();
                    gameState_ = GameState::MainMenu;
                    cardGameInitialized_ = false;
                }
            }
        }
    }

    prevMouseLDeckSelect_ = curMouseL;
}

void Lizard101Controller::resetGameState() {
    cardGame_.players.clear();
    cardGame_.enemies.clear();
    cardGame_.activePlayerIndex = 0;
    cardGame_.enemyTurnIndex = -1;
    cardGame_.enemyTurnPending = false;
    cardGame_.enemyTurnPauseTime = 0.0;
    cardGame_.firstRound = true;

    heroVisuals_.clear();
    handVisuals_.clear();
    enemyEntities_.clear();
    enemyOverlays_.clear();
    playerOverlays_.clear();
    menuButtonEntities_.clear();
    playZoneEntity_ = nullptr;

    draggedCard_ = nullptr;
    draggedVisual_ = nullptr;

    selectedNumPlayers_ = 0;
    currentDeckSelectPlayer_ = 0;
    playerDeckChoices_.clear();

    cardRewardChoices_.clear();
    cardRewardSelections_.clear();
    cardRewardRound_ = 0;
    currentLevel_ = 1;

    waitingForRelease_ = true;
    prevMouseLDeckSelect_ = false;
    prevKeyE_ = prevKey1_ = prevKey2_ = prevKey3_ = prevKey4_ = prevKey5_ = false;
}

void Lizard101Controller::clearAllVisualsAndEntities() {
    for (Entity* e : menuButtonEntities_) manager_.removeEntity(e);
    menuButtonEntities_.clear();
    for (auto& hv : heroVisuals_) if (hv.entity) manager_.removeEntity(hv.entity);
    heroVisuals_.clear();
    for (auto* e : enemyEntities_) if (e) manager_.removeEntity(e);
    enemyEntities_.clear();
    for (auto* e : enemyOverlays_) if (e) manager_.removeEntity(e);
    enemyOverlays_.clear();
    for (auto* e : playerOverlays_) if (e) manager_.removeEntity(e);
    playerOverlays_.clear();
    for (auto* e : enemyAuraOverlays_) if (e) manager_.removeEntity(e);
    enemyAuraOverlays_.clear();
    for (auto* e : playerAuraOverlays_) if (e) manager_.removeEntity(e);
    playerAuraOverlays_.clear();
    if (playZoneEntity_) { manager_.removeEntity(playZoneEntity_); playZoneEntity_ = nullptr; }
    handVisuals_.clear();
    manaSymbolEntities_.clear();
    deckEntity_ = nullptr;
}

bool Lizard101Controller::checkGameEnd() {
    if (allPlayersDowned()) {
        std::cout << "[GAME END] All players are downed! Restarting to main menu...\n";
        clearAllVisualsAndEntities();
        manager_.destroyAll();
        resetGameState();
        gameState_ = GameState::MainMenu;
        cardGameInitialized_ = false;
        return true;
    }
    if (allEnemiesDefeated()) {
        std::cout << "[LEVEL END] All enemies defeated! Moving to next round!\n";
        clearAllVisualsAndEntities();
        if (currentLevel_ >= maxLevels_) {
            std::cout << "[GAME END] All levels won! Lizards win!\n";
            manager_.destroyAll();
            resetGameState();
            gameState_ = GameState::MainMenu;
            cardGameInitialized_ = false;
            return true;
        }
        if (cardRewardRound_ < maxCardRewardRounds_) {
            manager_.destroyAll();
            gameState_ = GameState::CardReward;
            cardRewardRound_++;
            cardRewardChoices_.clear();
            cardRewardSelections_.assign(cardGame_.players.size(), -1);
            for (size_t i = 0; i < cardGame_.players.size(); ++i) {
                cardRewardChoices_.push_back(getRandomCardChoices());
            }
            // Heal and reset decks
            for (auto& player : cardGame_.players) {
                player.hp = player.maxHp; // or whatever max HP is
                player.discard.cards.clear();
                player.hand.cards.clear();
                player.deck.cards.clear();
                int nextInstanceId = 1;
                LizardCards::buildStartingDeck(player.school, player.deck.cards, nextInstanceId);
                for (const CardDefinition* def : player.earnedRewardCards) {
                    for (int c = 0; c < 3; ++c) {
                        player.deck.cards.push_back(CardInstance{ def, nextInstanceId++ });
                    }
                }
                // Add previously earned cards (see below)
            }
            for (auto& enemy : cardGame_.enemies) {
                enemy.hp = enemy.maxHp;
            }
            std::cout << "[DEBUG] REACHED CARD SELECT STATE\n";
        }
        else if (currentLevel_ < maxLevels_) {
            currentLevel_++;
            cardRewardRound_ = 0;
            // Setup next level here
            gameState_ = GameState::Gameplay;
            cardGameInitialized_ = false;
        }
        else {
            std::cout << "[GAME END] All levels won! Lizards win!\n";
            manager_.destroyAll();
            resetGameState();
            gameState_ = GameState::MainMenu;
            cardGameInitialized_ = false;
        }
        return true;
    }
    return false;
}

// ----------------- Visual helpers -----------------

void Lizard101Controller::ensureDeathOverlays() {
    // Enemy overlays
    if (enemyOverlays_.size() != enemyEntities_.size())
        enemyOverlays_.resize(enemyEntities_.size(), nullptr);

    for (size_t i = 0; i < cardGame_.enemies.size(); ++i) {
        const CombatPlayer& enemy = cardGame_.enemies[i];
        if (i >= enemyEntities_.size()) continue;
        Entity* eEntity = enemyEntities_[i];
        if (!eEntity) continue;

        if (enemy.hp <= 0) {
            if (!enemyOverlays_[i]) {
                enemyOverlays_[i] = new Entity(
                    "EnemyDeadOverlay",
                    eEntity->x, eEntity->y,
                    eEntity->width, eEntity->height
                );
                enemyOverlays_[i]->setTag("ENEMY_DEAD");
                enemyOverlays_[i]->addComponent<DebugRenderer>(
                    SDL_Color{ 255, 0, 0, 120 });
                manager_.addEntity(enemyOverlays_[i]);
                std::cout << "[ENEMY DOWNED] " << enemy.name
                    << " defeated - overlay created\n";
            }
        }
        else {
            if (enemyOverlays_[i]) {
                manager_.removeEntity(enemyOverlays_[i]);
                enemyOverlays_[i] = nullptr;
                std::cout << "[ENEMY REVIVED] " << enemy.name
                    << " revived - overlay removed\n";
            }
        }
    }

    // Player overlays
    if (playerOverlays_.size() != heroVisuals_.size())
        playerOverlays_.resize(heroVisuals_.size(), nullptr);

    for (size_t i = 0; i < cardGame_.players.size(); ++i) {
        const CombatPlayer& player = cardGame_.players[i];
        if (i >= heroVisuals_.size()) continue;
        Entity* heroEntity = heroVisuals_[i].entity;
        if (!heroEntity) continue;

        if (player.hp <= 0) {
            if (!playerOverlays_[i]) {
                playerOverlays_[i] = new Entity(
                    "PlayerDeadOverlay",
                    heroEntity->x, heroEntity->y,
                    heroEntity->width, heroEntity->height
                );
                playerOverlays_[i]->setTag("PLAYER_DEAD");
                playerOverlays_[i]->addComponent<DebugRenderer>(
                    SDL_Color{ 0, 0, 255, 120 });
                manager_.addEntity(playerOverlays_[i]);
                std::cout << "[PLAYER DOWNED] " << player.name
                    << " downed - overlay created\n";
            }
        }
        else {
            if (playerOverlays_[i]) {
                manager_.removeEntity(playerOverlays_[i]);
                playerOverlays_[i] = nullptr;
                std::cout << "[PLAYER REVIVED] " << player.name
                    << " revived - overlay removed\n";
            }
        }
    }
}

void Lizard101Controller::ensureAuraOverlays() {
    // Enemy aura overlays
    if (enemyAuraOverlays_.size() != enemyEntities_.size())
        enemyAuraOverlays_.resize(enemyEntities_.size(), nullptr);

    for (size_t i = 0; i < cardGame_.enemies.size(); ++i) {
        const CombatPlayer& enemy = cardGame_.enemies[i];
        if (i >= enemyEntities_.size()) continue;
        Entity* eEntity = enemyEntities_[i];
        if (!eEntity) continue;

        bool shouldShow = (enemy.hp > 0 && enemy.aura.kind != AuraKind::None);
        Entity*& overlay = enemyAuraOverlays_[i];

        if (shouldShow) {
            if (!overlay) {
                float fullW = eEntity->width;
                float fullH = eEntity->height;
                float h = fullH * 0.25f;
                float y = eEntity->y + fullH - h;

                overlay = new Entity("EnemyAuraOverlay",
                    eEntity->x, y, fullW, h);
                overlay->setTag("ENEMY_AURA");
                overlay->addComponent<DebugRenderer>(
                    SDL_Color{ 255, 255, 0, 120 });
                manager_.addEntity(overlay);
                std::cout << "[AURA VIS] Enemy aura overlay created for "
                    << enemy.name << "\n";
            }
            else {
                overlay->x = eEntity->x;
                float fullH = eEntity->height;
                float h = overlay->height;
                overlay->y = eEntity->y + fullH - h;
            }
        }
        else {
            if (overlay) {
                manager_.removeEntity(overlay);
                overlay = nullptr;
                std::cout << "[AURA VIS] Enemy aura overlay removed for "
                    << enemy.name << "\n";
            }
        }
    }

    // Player aura overlays
    if (playerAuraOverlays_.size() != heroVisuals_.size())
        playerAuraOverlays_.resize(heroVisuals_.size(), nullptr);

    for (size_t i = 0; i < cardGame_.players.size(); ++i) {
        const CombatPlayer& player = cardGame_.players[i];
        if (i >= heroVisuals_.size()) continue;
        Entity* heroEntity = heroVisuals_[i].entity;
        if (!heroEntity) continue;

        bool shouldShow = (player.hp > 0 && player.aura.kind != AuraKind::None);
        Entity*& overlay = playerAuraOverlays_[i];

        if (shouldShow) {
            if (!overlay) {
                float fullW = heroEntity->width;
                float fullH = heroEntity->height;
                float h = fullH * 0.25f;
                float y = heroEntity->y + fullH - h;

                overlay = new Entity("PlayerAuraOverlay",
                    heroEntity->x, y, fullW, h);
                overlay->setTag("PLAYER_AURA");
                overlay->addComponent<DebugRenderer>(
                    SDL_Color{ 255, 255, 0, 120 });
                manager_.addEntity(overlay);
                std::cout << "[AURA VIS] Player aura overlay created for "
                    << player.name << "\n";
            }
            else {
                overlay->x = heroEntity->x;
                float fullH = heroEntity->height;
                float h = overlay->height;
                overlay->y = heroEntity->y + fullH - h;
            }
        }
        else {
            if (overlay) {
                manager_.removeEntity(overlay);
                overlay = nullptr;
                std::cout << "[AURA VIS] Player aura overlay removed for "
                    << player.name << "\n";
            }
        }
    }
}


void Lizard101Controller::rebuildHandVisuals() {
    if (cardGame_.enemyTurnPending) return;
    if (cardGame_.players.empty())  return;
    if (cardGame_.activePlayerIndex < 0 ||
        cardGame_.activePlayerIndex >= static_cast<int>(cardGame_.players.size())) {
        return;
    }

    // Destroy old cards
    for (auto& cv : handVisuals_) {
        if (cv.entity) {
            manager_.removeEntity(cv.entity);
        }
    }
    handVisuals_.clear();

    // Destroy old mana symbols
    for (auto* e : manaSymbolEntities_) {
        if (e) manager_.removeEntity(e);
    }
    manaSymbolEntities_.clear();

    // Destroy old deck entity
    if (deckEntity_) {
        manager_.removeEntity(deckEntity_);
        deckEntity_ = nullptr;
    }

    CombatPlayer& active = cardGame_.activePlayer();
    auto& hand = active.hand.cards;
    const int N = static_cast<int>(hand.size());
    if (N == 0) return;

    const int cardW = 170;
    const int cardH = 256;
    const float bottomMargin = 40.0f;
    const float centerX = 1920.0f * 0.5f;
    const float handY = 1080.0f - bottomMargin - static_cast<float>(cardH);

    const float maxVisible = 5.0f;
    float span;
    if (N <= 1) {
        span = static_cast<float>(cardW);
    }
    else if (N <= 5) {
        span = static_cast<float>(cardW * N);
    }
    else {
        span = static_cast<float>(cardW * maxVisible);
    }

    float spacing = (N <= 1) ? 0.0f : (span / static_cast<float>(N - 1));
    float startX = centerX - span * 0.5f;

    for (int i = 0; i < N; ++i) {
        float x = startX + i * spacing;
        float y = handY;

        Entity* e = new Entity("Card", x, y, cardW, cardH);
        e->setTag("CARD");

        const CardInstance& ci = hand[static_cast<size_t>(i)];

        // ------------ TEXTURE LOGIC ------------
        std::string texturePath = "media/darkworld_platform_mossystone.png";
        bool useFallbackHighlight = true;

        if (ci.def && !ci.def->texturePath.empty()) {
            texturePath = ci.def->texturePath;
            useFallbackHighlight = false;
        }

        // Always have *some* texture (card art or default platform)
        e->addComponent<TextureRenderer>(renderer_, texturePath);

        // Only add colored highlight if we're on the fallback texture
        if (useFallbackHighlight) {
            SDL_Color col = colorForCard(ci);
            e->addComponent<DebugRenderer>(col);
        }
        // ------------ END TEXTURE LOGIC ------------

        auto& colComp = e->addComponent<BoxCollider>();
        colComp.setEventManager(&eventManager_);

        manager_.addEntity(e);
        handVisuals_.push_back(CardVisual{ e, static_cast<size_t>(i) });
    }

    // --- Display mana symbols above hand ---
    const int manaSymbolW = 64;
    const int manaSymbolH = 64;
    float manaY = handY - manaSymbolH - 10.0f; // 10px gap above hand
    float manaStartX = centerX - (active.mana * manaSymbolW) * 0.5f;

    for (int i = 0; i < active.mana; ++i) {
        float x = manaStartX + i * manaSymbolW;
        Entity* manaEntity = new Entity("ManaSymbol", x, manaY, manaSymbolW, manaSymbolH);
        manaEntity->setTag("MANA_SYMBOL");
        manaEntity->addComponent<TextureRenderer>(renderer_, "media/mana-symbol.png");
        manager_.addEntity(manaEntity);
        manaSymbolEntities_.push_back(manaEntity);
    }

    // --- Display deck as card-back sprite in bottom right ---
    const int deckW = 200;
    const int deckH = 280;
    float deckX = 1920.0f - deckW - 60.0f; // 40px margin from right
    float deckY = 1080.0f - deckH - 40.0f; // 40px margin from bottom

    deckEntity_ = new Entity("DeckCardBack", deckX, deckY, deckW, deckH);
    deckEntity_->setTag("DECK_CARD_BACK");
    deckEntity_->addComponent<TextureRenderer>(renderer_, "media/card-back.png");
    manager_.addEntity(deckEntity_);
}

// ----------------- Gameplay init -----------------

void Lizard101Controller::initGameplay() {
    // Clean previous entities
    for (Entity* e : menuButtonEntities_) {
        manager_.removeEntity(e);
    }
    menuButtonEntities_.clear();

    for (auto& hv : heroVisuals_) {
        if (hv.entity) manager_.removeEntity(hv.entity);
    }
    heroVisuals_.clear();

    for (auto* e : enemyEntities_) {
        if (e) manager_.removeEntity(e);
    }
    enemyEntities_.clear();

    for (auto* e : enemyOverlays_) {
        if (e) manager_.removeEntity(e);
    }
    enemyOverlays_.clear();

    for (auto* e : playerOverlays_) {
        if (e) manager_.removeEntity(e);
    }
    playerOverlays_.clear();

    for (auto* e : enemyAuraOverlays_) {
        if (e) manager_.removeEntity(e);
    }
    enemyAuraOverlays_.clear();

    for (auto* e : playerAuraOverlays_) {
        if (e) manager_.removeEntity(e);
    }
    playerAuraOverlays_.clear();

    if (playZoneEntity_) {
        manager_.removeEntity(playZoneEntity_);
        playZoneEntity_ = nullptr;
    }

    handVisuals_.clear();
    std::vector<std::vector<const CardDefinition*>> prevRewards;
    for (const auto& player : cardGame_.players) {
        prevRewards.push_back(player.earnedRewardCards);
    }
    cardGame_.players.clear();
    cardGame_.enemies.clear();

    // Build players from selected decks
    int nextInstanceId = 1;
    std::vector<CombatPlayer> players;
    for (int i = 0; i < selectedNumPlayers_; ++i) {
        School school = School::Generic;
        switch (playerDeckChoices_[i]) {
        case 0: school = School::Plant; break;
        case 1: school = School::Fire;  break;
        case 2: school = School::Water; break;
        default: break;
        }
        std::string name = "Player " + std::to_string(i + 1);
        CombatPlayer player(name, school, 30);
        LizardCards::buildStartingDeck(school, player.deck.cards, nextInstanceId);
        std::shuffle(player.deck.cards.begin(), player.deck.cards.end(), cardGame_.rng);
        // --- Restore earnedRewardCards if available ---
        if (i < prevRewards.size()) {
            player.earnedRewardCards = prevRewards[i];
        }
        players.push_back(player);
    }
    cardGame_.players = players;

    const int numEnemies = static_cast<int>(cardGame_.players.size());
    std::vector<std::string> enemyNames;
    std::vector<std::string> enemySprites;
    std::vector<int> enemyHPs;

    if (currentLevel_ == 1) {
        // Level 1: Snakes (same stats as old slime)
        for (int i = 0; i < numEnemies; ++i) {
            enemyNames.push_back("Snake " + std::to_string(i + 1));
            enemySprites.push_back("media/Snake.png");
            enemyHPs.push_back(20);
        }
    }
    else if (currentLevel_ == 2) {
        // Level 2: Roadrunners (TODO: increase stats)
        for (int i = 0; i < numEnemies; ++i) {
            enemyNames.push_back("Roadrunner " + std::to_string(i + 1));
            enemySprites.push_back("media/Roadrunner.png");
            enemyHPs.push_back(25); // TODO: adjust stats as needed
        }
    }
    else if (currentLevel_ == 3) {
        // Level 3: Hawks (different stats)
        for (int i = 0; i < numEnemies; ++i) {
            enemyNames.push_back("Hawk " + std::to_string(i + 1));
            enemySprites.push_back("media/Hawk.png");
            enemyHPs.push_back(30); // Example: 30 HP for Hawks
        }
    }

    for (int i = 0; i < numEnemies; ++i) {
        CombatPlayer enemy(enemyNames[i], School::Generic, enemyHPs[i]);
        LizardCards::buildStartingDeck(School::Generic, enemy.deck.cards, nextInstanceId);
        std::shuffle(enemy.deck.cards.begin(), enemy.deck.cards.end(), cardGame_.rng);
        cardGame_.enemies.push_back(enemy);
    }

    cardGame_.dealStartingHands(5);
    cardGame_.activePlayerIndex = 0;
    cardGame_.firstRound = true;
    cardGame_.firstRoundTurnsRemaining =
        static_cast<int>(cardGame_.players.size() + cardGame_.enemies.size());
    cardGame_.startTurn();

    // Play Zone
    {
        const int zoneW = 600;
        const int zoneH = 300;
        float x = 1920.0f * 0.5f - zoneW * 0.5f;
        float y = 1080.0f * 0.5f - zoneH * 0.5f;

        playZoneEntity_ = new Entity("PlayZone", x, y, zoneW, zoneH);
        playZoneEntity_->setTag("PLAY_ZONE");
        auto& col = playZoneEntity_->addComponent<BoxCollider>();
        col.setEventManager(&eventManager_);
        playZoneEntity_->addComponent<DebugRenderer>(SDL_Color{ 0, 255, 0, 80 });
        manager_.addEntity(playZoneEntity_);
    }

    // Enemy entities (right side)
    {
        const int actorW = 256;
        const int actorH = 358;
        float baseX = 1920.0f - 375.0f - actorW;
        float baseY = 250.0f;
        float offsetY[3] = { 0.0f, -230.0f, 230.0f };
        float offsetX[3] = { 0.0f, 250.0f, 250.0f };

        // use the same numEnemies from above (players.size(), max 3)
        for (int i = 0; i < numEnemies; ++i) {
            float x = baseX + offsetX[i];
            float y = baseY + offsetY[i];
            Entity* e = new Entity("EnemyEntity", x, y, actorW, actorH);
            e->setTag("ENEMY_" + std::to_string(i));
            e->addComponent<TextureRenderer>(
                renderer_,
                enemySprites[i]
            );
            e->addComponent<BoxCollider>();
            manager_.addEntity(e);
            enemyEntities_.push_back(e);
        }
    }


    // Hero entities (left side)
    {
        const int actorW = 256;
        const int actorH = 358;
        float baseX = 329.0f;
        float centerY = 250.0f;
        float offsetY[3] = { 0.0f, -230.0f, 230.0f };
        float offsetX[3] = { 0.0f, -250.0f, -250.0f };

        const char* deckSprites[3] = {
            "media/plant-wizard.png",  
            "media/fire-wizard.png",   
            "media/water-wizard.png"   
        };

        for (int i = 0; i < selectedNumPlayers_; ++i) {
            float x = baseX + offsetX[i];
            float y = centerY + offsetY[i];
            int deckType = playerDeckChoices_[i];
            Entity* heroEntity = new Entity("HeroEntity", x, y, actorW, actorH);
            heroEntity->setTag("ACTOR");

            if (deckType >= 0 && deckType < 3) {
                heroEntity->addComponent<TextureRenderer>(renderer_, deckSprites[deckType]);
            }
            else {
                // Fallback: colored rectangle if deck type is invalid
                heroEntity->addComponent<DebugRenderer>(colorForDeckType(deckType));
            }
            manager_.addEntity(heroEntity);
            heroVisuals_.push_back(HeroVisual{ heroEntity, playerDeckChoices_[i] });
        }
    }

    rebuildHandVisuals();
    cardGameInitialized_ = true;
}

// ----------------- Per-state updates -----------------

void Lizard101Controller::updateMainMenu() {
    updateBackground();
    // Clear previous menu entities
    for (Entity* e : menuButtonEntities_) {
        manager_.removeEntity(e);
    }
    menuButtonEntities_.clear();

    int buttonW = 180, buttonH = 80;
    float cx = 1920.0f * 0.5f;
    float by = 450.0f;

    float mx = Input::mouseX();
    float my = Input::mouseY();
    bool mouseDown = Input::isMouseButtonPressed(SDL_BUTTON_LMASK);

    const int numButtons = 3;
    const float spacing = 40.0f;

    float totalWidth = numButtons * buttonW + (numButtons - 1) * spacing;
    float startX = cx - totalWidth * 0.5f;

    for (int i = 0; i < numButtons; ++i) {
        float bx = startX + i * (buttonW + spacing);
        Entity* buttonEntity = new Entity("MenuButton", bx, by, buttonW, buttonH);
        buttonEntity->setTag("MENU_BUTTON");

        std::string texturePath =
            "media/menu/" + std::to_string(i + 1) + "_player.png";
        buttonEntity->addComponent<TextureRenderer>(renderer_, texturePath);

        manager_.addEntity(buttonEntity);
        menuButtonEntities_.push_back(buttonEntity);

        if (mouseDown &&
            mx >= bx && mx <= bx + buttonW &&
            my >= by && my <= by + buttonH) {
            std::cout << "[menu] " << (i + 1) << " Player(s) selected\n";
            selectedNumPlayers_ = i + 1;
            playerDeckChoices_.assign(selectedNumPlayers_, -1);
            currentDeckSelectPlayer_ = 0;
            waitingForRelease_ = true;
            prevMouseLDeckSelect_ = mouseDown;
            gameState_ = GameState::DeckSelect;
            break;
        }
    }
}



void Lizard101Controller::updateDeckSelect() {
    updateBackground();
    // Clear previous deck button entities
    for (Entity* e : menuButtonEntities_) {
        manager_.removeEntity(e);
    }
    menuButtonEntities_.clear();

    bool curMouseL = Input::isMouseButtonPressed(SDL_BUTTON_LMASK);
    float mx = Input::mouseX();
    float my = Input::mouseY();

    if (currentDeckSelectPlayer_ < selectedNumPlayers_) {
        int deckW = 180, deckH = 260;
        float cx = 1920.0f * 0.5f;
        SDL_Color deckColors[3] = {
            {80,200,80,255}, {255,80,80,255}, {80,120,255,255}
        };

        const char* deckNames[3] = {
          "Plant Deck", "Fire Deck", "Water Deck"
        };
        {
            int labelW = 360;
            int labelH = 160;
            float labelX = cx - labelW * 0.5f;
            float labelY = 150.0f;

            Entity* labelEntity =
                new Entity("DeckPlayerLabel", labelX, labelY, labelW, labelH);
            labelEntity->setTag("DECK_PLAYER_LABEL");

            int playerIndex1Based = currentDeckSelectPlayer_ + 1;
            std::string labelTexturePath =
                "media/menu/player_" + std::to_string(playerIndex1Based) + "_deck.png";

            labelEntity->addComponent<TextureRenderer>(renderer_, labelTexturePath);

            manager_.addEntity(labelEntity);
            menuButtonEntities_.push_back(labelEntity);
        }

        const int numDecks = 3;
        const float deckSpacing = 60.0f;
        float totalWidth = numDecks * deckW + (numDecks - 1) * deckSpacing;
        float startX = cx - totalWidth * 0.5f;
        float deckY = 400.0f;

        // Click handling (using same positions)
        if (!waitingForRelease_ && curMouseL && !prevMouseLDeckSelect_) {
            for (int i = 0; i < numDecks; ++i) {
                float bx = startX + i * (deckW + deckSpacing);
                float by = deckY;
                if (mx >= bx && mx <= bx + deckW &&
                    my >= by && my <= by + deckH) {
                    playerDeckChoices_[currentDeckSelectPlayer_] = i;
                    std::cout << "[DECK] " << deckNames[i]
                        << " for Player " << (currentDeckSelectPlayer_ + 1)
                        << "\n";
                    currentDeckSelectPlayer_++;
                    waitingForRelease_ = true;
                    break;
                }
            }
        }

        if (!curMouseL && waitingForRelease_) {
            waitingForRelease_ = false;
        }

        // Draw deck tiles (unchanged visuals, fixed positions)
        for (int i = 0; i < numDecks; ++i) {
            float bx = startX + i * (deckW + deckSpacing);
            float by = deckY;
            Entity* buttonEntity = new Entity("DeckButton", bx, by, deckW, deckH);
            buttonEntity->setTag("DECK_BUTTON");
            buttonEntity->addComponent<DebugRenderer>(deckColors[i]);
            manager_.addEntity(buttonEntity);
            menuButtonEntities_.push_back(buttonEntity);
        }
    }

    if (currentDeckSelectPlayer_ >= selectedNumPlayers_) {
        std::cout << "[deck] All players picked decks. Starting gameplay.\n";
        gameState_ = GameState::Gameplay;
        cardGameInitialized_ = false;
    }

    prevMouseLDeckSelect_ = curMouseL;
}



void Lizard101Controller::updateGameplay(float dt) {
    updateBackground();

    if (!cardGameInitialized_) {
        initGameplay();
    }

    if (cardGame_.enemyTurnPending) {
        
        for (auto& cv : handVisuals_) {
            if (cv.entity) manager_.removeEntity(cv.entity);
        }
        handVisuals_.clear();

        for (auto* e : manaSymbolEntities_) {
            if (e) manager_.removeEntity(e);
        }
        manaSymbolEntities_.clear();

        if (deckEntity_) {
            manager_.removeEntity(deckEntity_);
            deckEntity_ = nullptr;
        }

        cardGame_.enemyTurnPauseTime -= dt;
        if (cardGame_.enemyTurnPauseTime <= 0.0) {
            ensureDeathOverlays();
            ensureAuraOverlays();
            cardGame_.enemyTurnPending = false;
            cardGame_.endTurn();
            rebuildHandVisuals();
            if (checkGameEnd()) {
                return;
            }
        }
    }

    // Toggle event log with L
    bool curL = Input::isKeyPressed(SDL_SCANCODE_L);
    if (curL && !prevToggleLog_) {
        gEventLogEnabled = !gEventLogEnabled;
        std::cout << "[events] logging "
            << (gEventLogEnabled ? "ENABLED" : "DISABLED") << "\n";
    }
    prevToggleLog_ = curL;

    // Mouse drag -> DragInfo events
    static bool prevMouseL = false;
    bool curMouseL = Input::isMouseButtonPressed(SDL_BUTTON_LMASK);
    float mx = Input::mouseX();
    float my = Input::mouseY();

    auto raiseDrag = [&](DragInfo::Phase phase, float x, float y) {
        eventManager_.raise(Event{
            EventType::Input, EventPriority::High, 0.0f,
            EventPayload{ DragInfo{ phase, x, y } }
            });
        };

    if (curMouseL && !prevMouseL) {
        raiseDrag(DragInfo::Phase::Start, mx, my);
    }
    else if (curMouseL && prevMouseL) {
        raiseDrag(DragInfo::Phase::Move, mx, my);
    }
    else if (!curMouseL && prevMouseL) {
        raiseDrag(DragInfo::Phase::End, mx, my);
    }
    prevMouseL = curMouseL;


    // End turn with E
    bool curE = Input::isKeyPressed(SDL_SCANCODE_E);
    if (curE && !prevKeyE_ && !cardGame_.enemyTurnPending) {
        cardGame_.endTurn();
        rebuildHandVisuals();
        ensureDeathOverlays();
        ensureAuraOverlays();
        if (checkGameEnd()) {
            return;
        }
    }
    prevKeyE_ = curE;

    // --- DEBUG: Advance to next level with N ---
    static bool prevKeyN = false;
    bool curN = Input::isKeyPressed(SDL_SCANCODE_N);
    if (curN && !prevKeyN) {
        std::cout << "[DEBUG] N pressed: advancing to next level (as if all enemies defeated)\n";
        // Simulate all enemies defeated
        for (auto& enemy : cardGame_.enemies) {
            enemy.hp = 0;
        }
        clearAllVisualsAndEntities();
        checkGameEnd();
        return;
    }
    prevKeyN = curN;

    // Enemy pause & auto-progression
    if (cardGame_.enemyTurnPending) {
        // During enemy turns, hide the hand
        for (auto& cv : handVisuals_) {
            if (cv.entity) manager_.removeEntity(cv.entity);
        }
        handVisuals_.clear();

        cardGame_.enemyTurnPauseTime -= dt;
        if (cardGame_.enemyTurnPauseTime <= 0.0) {
            ensureDeathOverlays();
            ensureAuraOverlays();
            cardGame_.enemyTurnPending = false;
            cardGame_.endTurn();
            rebuildHandVisuals();
            if (checkGameEnd()) {
                return;
            }
        }
    }

    // Engine-level systems
    eventManager_.dispatch();
    manager_.updateAll(dt);
    physics_.updatePhysics(manager_, dt);
    Camera::getInstance().update(dt);
}

// ----------------- Public update/render -----------------

void Lizard101Controller::update(float dt) {
    switch (gameState_) {
    case GameState::MainMenu:
        updateMainMenu();
        break;
    case GameState::DeckSelect:
        updateDeckSelect();
        break;
    case GameState::CardReward:
        updateCardReward();
        break;
    case GameState::Gameplay:
        updateGameplay(dt);
        break;
    }
}

void Lizard101Controller::render() {
    // Right now everything is rendered via EntityManager.
    // This exists so we can add controller-specific HUD later (realistically we won't).
    manager_.renderAll(renderer_);
}
