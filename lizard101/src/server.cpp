#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <sstream>
#include <unordered_map>
#include "entityManager.h"
#include "entity.h"
#include "movingPlatform.h"
#include "Timeline.h"
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

#include "game/Player.h"

#define BASE_CLIENT_PORT 6000

std::mutex entityMutex;
std::atomic<bool> running{ true };
std::mutex clientThreadsMutex;
std::vector<std::thread> clientThreads;
std::atomic<int> clientCount{ 0 };

// Global server timeline (authoritative)
Timeline gameTimeline(1.0f);
EventManager eventManager(gameTimeline);

bool gEventLogEnabled = true;

// Handles communication with a single client
void client_handler(int clientId, int port, std::string playerName) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    std::string endpoint = "tcp://*:" + std::to_string(port);
    socket.bind(endpoint);

    int timeout = 1000; // ms
    socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    while (running) {
        zmq::message_t request;
        if (!socket.recv(request, zmq::recv_flags::none)) {
            if (!running) break;
            continue;
        }

        std::string request_str(static_cast<char*>(request.data()), request.size());
        std::istringstream ss(request_str);
        std::string cmd, name, xs, ys, eventType, eventData;
        std::getline(ss, cmd, '|');
        std::getline(ss, name, '|');
        std::getline(ss, xs, '|');
        std::getline(ss, ys, '|');
        std::getline(ss, eventType, '|');
        std::getline(ss, eventData, '|');

        std::string reply_str = "ERR";
        if (cmd == "UPDATE" && !name.empty()) {
            try {
                float x = std::stof(xs), y = std::stof(ys);
                std::lock_guard<std::mutex> lock(entityMutex);
                Entity* e = EntityManager::getInstance().findEntityByName(name);
                if (e) {
                    e->x = x;
                    e->y = y;
                }
                else {
                    auto* player = new Entity(name, x, y, 128, 128, true, false, "PLAYER");
                    player->setTag("PLAYER");
                    EntityManager::getInstance().addEntity(player);
                }
                reply_str = "OK";
            }
            catch (...) { reply_str = "ERR"; }
        }
        else if (cmd == "EVENT" && !eventType.empty()) {

          std::cout << "[Server] EVENT command received from " << playerName
                << ": eventType=\"" << eventType << "\" eventData=\"" << eventData << "\"" << std::endl;

          if (eventType == "Input") {
              // Example: eventData = "MoveLeft:1"
              auto sep = eventData.find(':');
              if (sep != std::string::npos) {
                  std::string action = eventData.substr(0, sep);
                  bool pressed = eventData.substr(sep + 1) == "1";
                  InputAction::Kind kind;
                  if (action == "MoveLeft") kind = InputAction::Kind::MoveLeft;
                  else if (action == "MoveRight") kind = InputAction::Kind::MoveRight;
                  else if (action == "Jump") kind = InputAction::Kind::Jump;
                  else kind = InputAction::Kind::None;

                  std::lock_guard<std::mutex> lock(entityMutex);
                  Entity* playerEntity = EntityManager::getInstance().findEntityByName(playerName);
                  if (playerEntity) {

                      float moveSpeed = 300.0f; 
                      if (kind == InputAction::Kind::MoveLeft && pressed) {
                          playerEntity->x -= moveSpeed * gameTimeline.getDeltaTime();
                      }
                      else if (kind == InputAction::Kind::MoveRight && pressed) {
                          playerEntity->x += moveSpeed * gameTimeline.getDeltaTime();
                      }
                      else if (kind == InputAction::Kind::Jump && pressed) {
                          if (playerEntity->type == "PLAYER") {
                              playerEntity->y -= 150.0f; 
                          }
                      }
                  }

                  eventManager.raise(Event{
                      EventType::Input,
                      EventPriority::High,
                      0.0f,
                      EventPayload{ InputAction{ kind, pressed } }
                      });
                  reply_str = "EVENT_OK";
              }
          }
          else if (eventType == "Death") {
              // Parse eventData: "PlayerName,DeathZoneName"
              auto sep = eventData.find(',');
              std::string victimName = eventData.substr(0, sep);
              std::string deathZoneName = (sep != std::string::npos) ? eventData.substr(sep + 1) : "";

              std::lock_guard<std::mutex> lock(entityMutex);
              Entity* victim = EntityManager::getInstance().findEntityByName(victimName);

              if (!victim) {
                  std::cerr << "[Server] Death event: victim entity not found: " << victimName << std::endl;
                  reply_str = "EVENT_ERR";
              }
              else {
                  // Find nearest spawn point
                  Entity* spawn = nullptr;
                  float nearest = std::numeric_limits<float>::max();
                  for (auto* e : EntityManager::getInstance().entities) {
                      if (e && e->hasTag("SPAWN")) {
                          float dx = (e->x + e->width * 0.5f) - (victim->x + victim->width * 0.5f);
                          float dy = (e->y + e->height * 0.5f) - (victim->y + victim->height * 0.5f);
                          float d = std::sqrt(dx * dx + dy * dy);
                          if (d < nearest) { nearest = d; spawn = e; }
                      }
                  }
                  if (spawn) {
                      victim->x = spawn->x;
                      victim->y = spawn->y;
                      victim->velY = 0.0f;
                      std::cout << "[Server] Player " << victimName << " respawned at " << spawn->name << std::endl;
                      reply_str = "EVENT_OK";
                  }
                  else {
                      std::cerr << "[Server] Death event: no spawn found for respawn" << std::endl;
                      reply_str = "EVENT_ERR";
                  }
              }
          }
        }
        else if (cmd == "PAUSE") {
            gameTimeline.togglePause();
            bool paused = gameTimeline.isPaused();
            std::cout << "[Server] PAUSE command received from " << name << ". Now paused = " << (paused ? "true" : "false") << std::endl;
            reply_str = paused ? "PAUSED" : "RUNNING";
        }
        else if (cmd == "DISCONNECT" && !name.empty()) {
            {
                std::lock_guard<std::mutex> lock(entityMutex);
                Entity* e = EntityManager::getInstance().findEntityByName(name);
                if (e) {
                    EntityManager::getInstance().removeEntity(e);
                    std::cout << "[Server] Client disconnected: Player Name=\"" << name << "\". Entity removed." << std::endl;
                }
                else {
                    std::cout << "[Server] Client disconnected: Player Name=\"" << name << "\". No entity found to remove." << std::endl;
                }
            }
            reply_str = "BYE";
            zmq::message_t reply(reply_str.size());
            memcpy(reply.data(), reply_str.data(), reply_str.size());
            socket.send(reply, zmq::send_flags::none);

            break;
        }
        zmq::message_t reply(reply_str.size());
        memcpy(reply.data(), reply_str.data(), reply_str.size());
        if (!socket.send(reply, zmq::send_flags::none)) {
            break;
        }
    }

    socket.close();
    context.close();
}

// Accepts new clients and creates a handler thread for each
void acceptor_handler() {
    zmq::context_t context(1);
    zmq::socket_t acceptor(context, zmq::socket_type::rep);
    acceptor.bind("tcp://*:5555");

    while (running) {
        zmq::message_t request;
        if (!acceptor.recv(request, zmq::recv_flags::none)) continue;

        std::string request_str(static_cast<char*>(request.data()), request.size());
        std::istringstream ss(request_str);
        std::string cmd, playerName;
        std::getline(ss, cmd, '|');
        std::getline(ss, playerName, '|');

        int clientId = clientCount.fetch_add(1);
        int clientPort = BASE_CLIENT_PORT + clientId;

        std::cout << "[Server] Client connected: Player Name=\"" << playerName
            << "\" Assigned Port=" << clientPort << std::endl;

        {
            std::lock_guard<std::mutex> lock(clientThreadsMutex);
            clientThreads.emplace_back(client_handler, clientId, clientPort, playerName);
        }

        std::string reply_str = std::to_string(clientPort);
        zmq::message_t reply(reply_str.size());
        memcpy(reply.data(), reply_str.data(), reply_str.size());
        acceptor.send(reply, zmq::send_flags::none);
    }
    acceptor.close();
    context.close();
}

// Publishes all entity states (unchanged)
void pub_handler() {
    zmq::context_t context(1);
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5556");
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::string state = "STATE";
        {
            std::lock_guard<std::mutex> lock(entityMutex);
            for (auto* e : EntityManager::getInstance().entities) {
                std::string t = e->type.empty() ? "GENERIC" : e->type;
                state += "|" + e->name + "|" + t + "|" + std::to_string(e->x) + "|" + std::to_string(e->y) +
                    "|" + std::to_string(e->width) + "|" + std::to_string(e->height);
            }
        }
        state += "|PAUSED|" + std::string(gameTimeline.isPaused() ? "1" : "0");
        zmq::message_t message(state.size());
        memcpy(message.data(), state.data(), state.size());
        publisher.send(message, zmq::send_flags::none);
    }
    publisher.close();
    context.close();
}

// Updates all entities on the server (unchanged)
void update_handler() {
    while (running) {
        gameTimeline.update();
        float deltaTime = gameTimeline.getDeltaTime();
        eventManager.dispatch();
        EntityManager::getInstance().updateAll(deltaTime);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {

    auto* ground = new Entity("Ground", 0.0f, 500.0f, 1024, 64, false, true);
    ground->setTag("GROUND");
    EntityManager::getInstance().addEntity(ground);

    auto* mp1 = new MovingPlatform("MP1", 250.0f, 650.0f, 96, 32, true, 150.0f, 400.0f);
    mp1->setTag("MOVING_PLATFORM");
    EntityManager::getInstance().addEntity(mp1);

    auto* pause = new PauseButton("PauseButton", 1870.0f, 20.0f, 32, 32, &gameTimeline);
    pause->setTag("PAUSEBUTTON");
    EntityManager::getInstance().addEntity(pause);

    auto* debugDeathZone = new Entity("Death", 300.0f, 1000.0f, 100, 50, /*physics*/false, /*solid*/false);
    debugDeathZone->setTag("DEATH");
    debugDeathZone->addComponent<DeathZone>("SpawnA"); 
    debugDeathZone->addComponent<DebugRenderer>(SDL_Color{ 255, 0, 0, 160 }); 
    EntityManager::getInstance().addEntity(debugDeathZone);

    auto* debugSpawnZone = new Entity("SpawnA", 600.0f, 900.0f, 64, 64);
    debugSpawnZone->setTag("SPAWN");
    debugSpawnZone->addComponent<SpawnPoint>();
    debugSpawnZone->addComponent<DebugRenderer>(SDL_Color{ 0, 255, 0, 160 }); 
    EntityManager::getInstance().addEntity(debugSpawnZone);

    std::cout << "[Server] Entities: ";
    for (auto* e : EntityManager::getInstance().entities) {
        std::cout << e->name << " ";
    }
    std::cout << std::endl;

    std::thread update_thread(update_handler);
    std::thread pub_thread(pub_handler);
    std::thread acceptor_thread(acceptor_handler);

    std::cout << "Server running.\n";
    std::cin.get();
    running = false;

    update_thread.join();
    pub_thread.join();
    acceptor_thread.join();

    {
        std::lock_guard<std::mutex> lock(clientThreadsMutex);
        for (auto& t : clientThreads) t.join();
    }
    return 0;
}