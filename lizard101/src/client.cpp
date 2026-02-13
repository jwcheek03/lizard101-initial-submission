#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <optional>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "entity.h"
#include "EntityManager.h"
#include "game/Player.h"
#include "game/stationaryPlatform.h"
#include "game/movingPlatform.h"
#include "Input.h"
#include "Timeline.h"
#include "Physics.h"
#include "JobSystem.hpp"
#include "SharedData.hpp"
#include "game/PauseButton.h"

#include "game/components/Health.h"
#include "game/components/TouchDamage.h"
#include "game/components/TextureRenderer.h"
#include "game/components/BoxCollider.h"
#include "game/components/RigidBody.h"
#include "game/components/DeathZones.h"
#include "game/components/DebugRenderer.h"
#include "game/components/SpawnPoint.h"

#include "events/EventManager.h"
#include "events/Event.h"

#include "events/InputHandler.h"

// Pause control coordination
std::atomic<int> pauseRequested{ 0 }; // 0 = none, 1 = request toggle
std::atomic<int> serverPaused{ 0 };   // 0 = running, 1 = paused (set from server STATE messages)

SharedData sharedData;
JobQueue inputJobs, networkJobs, collisionJobs;

std::atomic<bool> running(true);

// pending updates received from network: name -> (x,y,w,h)
static std::unordered_map<std::string, std::tuple<float, float, int, int>> pendingUpdates;
static std::mutex updateEntityMutex;

// Ensure only one send/recv pair uses the dedicated REQ socket at a time
static std::mutex reqMutex;

bool gEventLogEnabled = true;

bool localPlayerNeedsRespawn = false;
bool deathEventSentThisFrame = false; // Prevent spamming Death events
// last time (ms since steady_clock epoch) we sent a Death event from this client
static std::atomic<long long> lastDeathSentMs{0};

static inline long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void send_event(zmq::socket_t& reqSock, const std::string& playerName, const std::string& eventType, const std::string& eventData) {
    std::ostringstream ss;
    ss << "EVENT|" << playerName << "|||"
        << eventType << "|" << eventData;
    std::string msg = ss.str();
    zmq::message_t request(msg.size());
    memcpy(request.data(), msg.data(), msg.size());
    try {
        std::lock_guard<std::mutex> lock(reqMutex);
        reqSock.send(request, zmq::send_flags::none);
        zmq::pollitem_t items[] = { { static_cast<void*>(reqSock), 0, ZMQ_POLLIN, 0 } };
        zmq::poll(items, 1, 100);
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t reply;
            reqSock.recv(reply, zmq::recv_flags::none);
        }
    }
    catch (const zmq::error_t& e) {
        std::cerr << "send_event zmq error: " << e.what() << std::endl;
        // Swallow to avoid terminating the client; caller can retry later
    }
}

// Subscriber: fills pendingUpdates map (no direct changes to EntityManager here)
void receive_state(const std::string& localPlayerName) {
    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect("tcp://localhost:5556");
    sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    while (running.load()) {
        zmq::message_t msg;
        if (!sub.recv(msg, zmq::recv_flags::none)) continue;
        std::string data(static_cast<char*>(msg.data()), msg.size());

        if (data.rfind("STATE", 0) != 0) continue;

        std::istringstream ss(data);
        std::string token;
        std::getline(ss, token, '|'); // skip "STATE"

        // parse groups of 6 tokens: name|type|x|y|w|h
        while (std::getline(ss, token, '|')) {
            std::string name = token;
            // If we encounter a PAUSED token here, handle global pause flag and stop parsing entities
            if (name == "PAUSED") {
                std::string val;
                if (std::getline(ss, val, '|')) {
                    try { serverPaused.store(std::stoi(val)); }
                    catch (...) { /* ignore malformed */ }
                }
                break; // no more entity tokens after PAUSED
            }

            std::string type;
            std::string xs, ys, ws, hs;
            if (!std::getline(ss, type, '|')) break;
            if (!std::getline(ss, xs, '|')) break;
            if (!std::getline(ss, ys, '|')) break;
            if (!std::getline(ss, ws, '|')) break;
            if (!std::getline(ss, hs, '|')) break;
            try {
                float x = std::stof(xs);
                float y = std::stof(ys);
                int w = std::stoi(ws);
                int h = std::stoi(hs);

                std::lock_guard<std::mutex> lock(updateEntityMutex);

                std::string entityType = type + "::" + name;
                pendingUpdates[entityType] = std::make_tuple(x, y, w, h);
            }
            catch (...) {
                // ignore malformed entries
            }
        }
    }

    sub.close();
    ctx.close();
}

void sendInputEvent(zmq::socket_t& socket, const std::string& playerName, InputAction::Kind kind, bool pressed) {
    std::string actionStr;
    if (kind == InputAction::Kind::MoveLeft) actionStr = "MoveLeft";
    else if (kind == InputAction::Kind::MoveRight) actionStr = "MoveRight";
    else if (kind == InputAction::Kind::Jump) actionStr = "Jump";
    else return;

    std::string eventData = actionStr + ":" + (pressed ? "1" : "0");
    // Use the thread-safe helper which locks the REQ socket and handles replies/timeouts
    send_event(socket, playerName, "Input", eventData);
}

// Send one UPDATE request to server using the dedicated port
bool send_update(zmq::socket_t& req, const std::string& name, float x, float y, int timeoutMs = 500) {
    std::ostringstream ss;
    ss << "UPDATE|" << name << "|" << x << "|" << y;
    std::string msg = ss.str();
    zmq::message_t request(msg.size());
    memcpy(request.data(), msg.data(), msg.size());

    try {
        std::lock_guard<std::mutex> lock(reqMutex);
        req.send(request, zmq::send_flags::none);

        // wait for reply
        zmq::pollitem_t items[] = { { static_cast<void*>(req), 0, ZMQ_POLLIN, 0 } };
        int rc = zmq::poll(items, 1, timeoutMs);
        if (rc <= 0) {
            return false;
        }

        zmq::message_t reply;
        if (!req.recv(reply, zmq::recv_flags::none)) {
            return false;
        }
        std::string repStr(static_cast<char*>(reply.data()), reply.size());
        return repStr.rfind("OK", 0) == 0;
    }
    catch (const zmq::error_t& e) {
        std::cerr << "send_update zmq error: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Ask for player name
    std::string playerName = "Player";
    std::cout << "Enter player name (no spaces): ";
    std::cin >> playerName;

    // --- Handshake: get dedicated port from server ---
    int dedicatedPort = 0;
    {
        zmq::context_t handshakeCtx(1);
        zmq::socket_t handshakeSock(handshakeCtx, zmq::socket_type::req);
        handshakeSock.connect("tcp://localhost:5555");
        std::string handshakeMsg = "CONNECT|" + playerName;
        zmq::message_t handshakeReq(handshakeMsg.size());
        memcpy(handshakeReq.data(), handshakeMsg.data(), handshakeMsg.size());
        handshakeSock.send(handshakeReq, zmq::send_flags::none);

        zmq::message_t handshakeReply;
        if (!handshakeSock.recv(handshakeReply, zmq::recv_flags::none)) {
            std::cerr << "Failed to receive handshake reply from server." << std::endl;
            return 1;
        }
        std::string portStr(static_cast<char*>(handshakeReply.data()), handshakeReply.size());
        try {
            dedicatedPort = std::stoi(portStr);
        }
        catch (...) {
            std::cerr << "Invalid port received from server: " << portStr << std::endl;
            return 1;
        }
        handshakeSock.close();
        handshakeCtx.close();
    }

    // --- Open dedicated REQ socket for updates ---
    zmq::context_t reqCtx(1);
    zmq::socket_t reqSock(reqCtx, zmq::socket_type::req);
    std::string endpoint = "tcp://localhost:" + std::to_string(dedicatedPort);
    reqSock.connect(endpoint);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Networked Client", 1920, 1080, 0, &window, &renderer)) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "media/darkworld_enemy_nyx_idle.png");
    if (!playerTexture) {
        SDL_Log("Warning: failed to load player texture: %s", SDL_GetError());
    }

    SDL_Texture* platformTexture = IMG_LoadTexture(renderer, "media/darkworld_platform_mossystone.png");
    if (!platformTexture) {
        SDL_Log("Warning: failed to load platform texture: %s", SDL_GetError());
    }

    SDL_Texture* skullTexture = IMG_LoadTexture(renderer, "media/darkworld_enemy_skullduggery_idle.png");
    if (!skullTexture) {
        SDL_Log("Failed to load texture: %s", SDL_GetError());
    }

    // Start subscriber thread
    std::thread subThread(receive_state, playerName);

    // Create local player entity (only local client owns this entity's input)
    EntityManager& manager = EntityManager::getInstance();

    Timeline timeline(1.0f); // Normal speed

    EventManager eventManager(timeline);

    auto* localPlayer = new Player(playerName, 100.0f, 100.0f, 128, 128, eventManager);
    localPlayer->setTag("PLAYER");
    localPlayer->addComponent<TextureRenderer>(renderer, "media/darkworld_enemy_nyx_idle.png", 128, 128, 8, 1, 6.6667f);
    localPlayer->addComponent<BoxCollider>();
    if (auto* c = localPlayer->getComponent<BoxCollider>()) c->setEventManager(&eventManager);
    manager.addEntity(localPlayer);

    // Add Pause button (passes pointer to timeline so the button can pause/unpause)
    auto* pauseBtn = new PauseButton("PauseButton", 1870.0f, 20.0f, 32, 32, &timeline);
    pauseBtn->addComponent<TextureRenderer>(
        renderer, "media/darkworld_enemy_skullduggery_idle.png",
        32, 32, 6, 1, 6.6667f
    );
    manager.addEntity(pauseBtn);

    PauseButton::onPauseRequested = []() { pauseRequested.store(1); };

    // Subscribe to input events and send them to the server
    eventManager.subscribe(EventType::Input, [&](const Event& e) {
        if (auto* ia = std::get_if<InputAction>(&e.payload)) {
            sendInputEvent(reqSock, playerName, ia->action, ia->pressed);
        }
        });

    // Main loop
    bool clientRunning = true;
    SDL_Event event;
    const float sendIntervalSec = 0.001f; // 100 ms

    float elapsedTime = 0.0f;
    float lastSendTime = 0.0f;

    PhysicsSystem physics;

    inputJobs.push_back([&]() {
        static bool prevA = false, prevD = false, prevJump = false;
        Input::update();

        const bool curA = Input::isKeyPressed(SDL_SCANCODE_A);
        const bool curD = Input::isKeyPressed(SDL_SCANCODE_D);
        const bool curJump = Input::isKeyPressed(SDL_SCANCODE_W) || Input::isKeyPressed(SDL_SCANCODE_SPACE);

        auto raiseInput = [&](InputAction::Kind kind, bool pressed) {
            eventManager.raise(Event{
                EventType::Input,
                EventPriority::High,
                0.0f,
                EventPayload{ InputAction{ kind, pressed, playerName } }
                });
            };

        if (curA != prevA) { raiseInput(InputAction::Kind::MoveLeft, curA); prevA = curA; }
        if (curD != prevD) { raiseInput(InputAction::Kind::MoveRight, curD); prevD = curD; }
        if (curJump != prevJump) { raiseInput(InputAction::Kind::Jump, curJump); prevJump = curJump; }

        if (Input::isKeyPressed(SDL_SCANCODE_1)) { timeline.setScale(0.5f); }
        if (Input::isKeyPressed(SDL_SCANCODE_2)) { timeline.setScale(1.0f); }
        if (Input::isKeyPressed(SDL_SCANCODE_3)) { timeline.setScale(2.0f); }

        });

    collisionJobs.push_back([&]() {
        std::cout << "[Job] Collision job started" << std::endl;

        Entity* localPlayer = manager.findEntityByName(playerName);
        if (!localPlayer) {
            std::cout << "[Job] Collision job complete (no local player)" << std::endl;
            return;
        }
        auto& entities = manager.entities;

        bool playerInDeathZone = false; 
        bool playerWasInDeathZone = false;

        for (auto* other : entities) {
            // Only check for death zones here; all other collision handling is done via the event system
            if (!other->getComponent<DeathZone>() && (other == localPlayer || !other->isSolid)) continue;

            if (collisionDetection(*localPlayer, *other)) {

                if (other->hasTag("DEATH")) {
                    playerInDeathZone = true;
                    if (!playerWasInDeathZone) {
                        localPlayerNeedsRespawn = true;
                        std::ostringstream deathData;
                            deathData << localPlayer->name << "," << other->name;
                            // suppress repeated Death sends for a short window
                            {
                                long long last = lastDeathSentMs.load();
                                long long now = nowMs();
                                if (now - last >= 250) {
                                    send_event(reqSock, playerName, "Death", deathData.str());
                                    lastDeathSentMs.store(now);
                                }
                            }
                    }
                }
            }
        }
        playerWasInDeathZone = playerInDeathZone; 
        std::cout << "[Job] Collision job complete" << std::endl;
        });

    std::thread inputThread(worker, std::ref(sharedData), std::cref(inputJobs), JobType::Input);
    std::thread collisionThread(worker, std::ref(sharedData), std::cref(collisionJobs), JobType::Collision);

    while (clientRunning) {
        // Use Input system for event processing and key state
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                clientRunning = false;
            }
        }

        std::cout << "[Main] New frame" << std::endl;

        std::cout << "[Main] Entity count: " << manager.entities.size() << std::endl;
        //Reset job indices at the start of each frame
        {
            std::unique_lock<std::mutex> lock(sharedData.frameMutex);
            sharedData.inputJobIndex = 0;
            sharedData.collisionJobIndex = 0;
            sharedData.jobsToComplete = 2;   // reset counter
            sharedData.frameReady = true;    // signal work is ready
        }
        sharedData.frameCv.notify_all();

        // Sync local timeline paused state with server's authoritative paused flag
        // If serverPaused differs from current timeline state, toggle timeline to match
        try {
            bool targetPaused = serverPaused.load() != 0;
            if (timeline.isPaused() != targetPaused) {
                timeline.togglePause();
            }
        }
        catch (...) {
            // If Timeline doesn't support isPaused/toggle in this build, ignore
        }

        timeline.update();

        float deltaTime = timeline.getDeltaTime();
        elapsedTime += deltaTime;

        // Dispatch queued events (input events sent to server here)
        eventManager.dispatch();

        localPlayer = dynamic_cast<Player*>(manager.findEntityByName(playerName));
            if (localPlayer && (elapsedTime - lastSendTime) >= sendIntervalSec) {
                // if we recently sent a Death event, skip updates for a short window to avoid overwriting server respawn
                long long last = lastDeathSentMs.load();
                if (nowMs() - last >= 300) {
                    send_update(reqSock, playerName, localPlayer->x, localPlayer->y);
                    lastSendTime = elapsedTime;
                }
            }

        // If Pause was requested by local PauseButton, send PAUSE command to server
        if (pauseRequested.load() != 0) {
            // Build PAUSE command. Server will toggle global timeline and publish PAUSED flag.
            std::ostringstream pss;
            pss << "PAUSE|" << playerName;
            std::string pmsg = pss.str();
            zmq::message_t request(pmsg.size());
            memcpy(request.data(), pmsg.data(), pmsg.size());

            {
                std::lock_guard<std::mutex> lock(reqMutex);
                SDL_Log("Client: sending PAUSE request to server: %s", pmsg.c_str());
                reqSock.send(request, zmq::send_flags::none);

                // wait for reply (with short timeout)
                zmq::pollitem_t items[] = { { static_cast<void*>(reqSock), 0, ZMQ_POLLIN, 0 } };
                int rc = zmq::poll(items, 1, 500);
                if (rc > 0) {
                    zmq::message_t reply;
                    if (reqSock.recv(reply, zmq::recv_flags::none)) {
                        std::string repStr(static_cast<char*>(reply.data()), reply.size());
                        SDL_Log("PAUSE request reply: %s", repStr.c_str());
                    }
                }
            }
            pauseRequested.store(0);
        }

        manager.updateAll(deltaTime);
        physics.updatePhysics(manager, deltaTime);

        Entity* localPlayer = manager.findEntityByName(playerName);
        if (localPlayer) {
            for (auto* other : manager.entities) {
                if (other == localPlayer || !other->isSolid) continue;
                if (collisionDetection(*localPlayer, *other)) {
                    // Example death zone check
                    if (other->hasTag("DEATH") && !deathEventSentThisFrame) {
                        localPlayerNeedsRespawn = true;
                        std::ostringstream deathData;
                        deathData << localPlayer->name << "," << other->name;
                        send_event(reqSock, playerName, "Death", deathData.str());
                        deathEventSentThisFrame = true;
                    }
                }
            }
        }
        deathEventSentThisFrame = false; // Reset for next frame

        // Apply pending network updates on main thread
        {
            std::lock_guard<std::mutex> lock(updateEntityMutex);

            // 1. Collect all entity names from the latest server state
            std::unordered_set<std::string> serverEntityNames;
            for (auto& kv : pendingUpdates) {
                const std::string key = kv.first; // format: "TYPE::name"
                std::string::size_type pos = key.find("::");
                std::string name = (pos != std::string::npos) ? key.substr(pos + 2) : key;
                serverEntityNames.insert(name);
            }
            serverEntityNames.insert(playerName); // Always keep local player

            // 2. Add known server-side static entities
            std::unordered_set<std::string> staticEntities = {
                "Ground", "MP1" // Add more as needed
            };
            for (const auto& staticName : staticEntities) {
                serverEntityNames.insert(staticName);
            }

            // 3. Apply updates and add new entities
            for (auto& kv : pendingUpdates) {
                const std::string key = kv.first; // format: "TYPE::name"
                float x; float y; int w; int h;
                std::tie(x, y, w, h) = kv.second;

                // split key into type and name
                std::string::size_type pos = key.find("::");
                std::string type = "GENERIC";
                std::string name = key;
                if (pos != std::string::npos) {
                    type = key.substr(0, pos);
                    name = key.substr(pos + 2);
                }

                Entity* e = manager.findEntityByName(name);
                if (e) {
                    if (name == playerName) {
                        if (localPlayerNeedsRespawn) {
                            e->x = x;
                            e->y = y;
                            localPlayerNeedsRespawn = false;
                        }
                    }
                    else {
                        e->x = x;
                        e->y = y;
                        e->width = w;
                        e->height = h;
                    }
                }
                else {
                    // Instantiate components from server based on reported type
                    if (type == "PLAYER") {
                        auto* e = new Player(name, x, y, w, h, eventManager);
                        e->setTag("PLAYER");
                        e->addComponent<TextureRenderer>(renderer, "media/darkworld_enemy_nyx_idle.png", 128, 128, 8, 1, 6.6667f);
                        e->addComponent<BoxCollider>();
                        if (auto* c = e->getComponent<BoxCollider>()) c->setEventManager(&eventManager);
                        manager.addEntity(e);
                    }
                    else if (type == "GROUND") {
                        auto* e = new Entity(name, x, y, w, h, false, true);
                        e->setTag("GROUND");
                        e->addComponent<TextureRenderer>(renderer, "media/darkworld_platform_mossystone.png");
                        e->addComponent<BoxCollider>();
                        if (auto* c = e->getComponent<BoxCollider>()) c->setEventManager(&eventManager);

                        manager.addEntity(e);
                    }
                    else if (type == "MOVING_PLATFORM") {
                        auto* e = new MovingPlatform(name, x, y, w, h, true, 150.0f, 400.0f);
                        e->setTag("MOVING_PLATFORM");
                        e->addComponent<TextureRenderer>(renderer, "media/darkworld_platform_mossystone.png");
                        e->addComponent<BoxCollider>();
                        if (auto* c = e->getComponent<BoxCollider>()) c->setEventManager(&eventManager);
                        manager.addEntity(e);
                    }
                    else if (type == "PAUSEBUTTON") {
                        auto* e = new PauseButton(name, x, y, w, h, &timeline);
                        e->setTag("PAUSEBUTTON");
                        e->addComponent<TextureRenderer>(
                            renderer, "media/darkworld_enemy_skullduggery_idle.png",
                            32, 32, 6, 1, 6.6667f
                        );
                        manager.addEntity(e);
                    }
                    else if (type == "DEATH") {
                        auto* e = new Entity(name, x, y, w, h, false, false, type);
                        e->setTag("DEATH");
                        e->addComponent<DeathZone>("Spawn A");
                        e->addComponent<DebugRenderer>(SDL_Color{ 255, 0, 0, 160 });
                        manager.addEntity(e);
                    }
                    else if (type == "SPAWN") {
                        auto* e = new Entity(name, x, y, w, h, false, false, type);
                        e->setTag("SPAWN");
                        e->addComponent<SpawnPoint>();
                        e->addComponent<DebugRenderer>(SDL_Color{ 0, 255, 0, 160 });
                        manager.addEntity(e);
                    }
                    else {
                        auto* e = new Entity(name, x, y, w, h, false, false, type);
                        e->setTag(type);
                        manager.addEntity(e);
                    }
                }
            }

            // 4. Remove local entities not present in server state or static set (except local player)
            auto& entities = manager.entities;
            for (auto it = entities.begin(); it != entities.end(); ) {
                Entity* e = *it;
                if (serverEntityNames.find(e->name) == serverEntityNames.end()) {
                    delete e;
                    it = entities.erase(it);
                }
                else {
                    ++it;
                }
            }

            pendingUpdates.clear();
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        {
            std::lock_guard<std::mutex> lock(updateEntityMutex);
            manager.renderAll(renderer);
        }
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    {
        std::ostringstream ss;
        ss << "DISCONNECT|" << playerName;
        std::string msg = ss.str();
        zmq::message_t request(msg.size());
        memcpy(request.data(), msg.data(), msg.size());
        reqSock.send(request, zmq::send_flags::none);

        zmq::message_t reply;
        reqSock.recv(reply, zmq::recv_flags::none);
    }

    running = false;
    sharedData.running = false;
    sharedData.frameCv.notify_all();
    if (subThread.joinable()) subThread.join();
    inputThread.join();
    collisionThread.join();

    if (playerTexture) SDL_DestroyTexture(playerTexture);
    if (platformTexture) SDL_DestroyTexture(platformTexture);
    if (skullTexture) SDL_DestroyTexture(skullTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}