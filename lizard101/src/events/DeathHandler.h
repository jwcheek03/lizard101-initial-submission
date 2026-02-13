class DeathHandler : public EventHandler {
public:
    DeathHandler(EventManger& ev) : events_(ev) {}

    void onEvent(Event& e) override {
        if (e.type != EventType::Death) {
            return;
        }

        if (auto* d = std::get_if<DeathInfo>(&e.payload)) {
            Entity* victim = d->victim;
            if ( !victim ) {
                return;
            }

            Entity* player = d->victim;
            Entity* spawn = nullptr;
            float nearestSpawn = std::numeric_limits<float>::max();

            for( auto* entities : EntityManager::getInstance().entities ) {
                if ( !entities ) {
                    continue;
                }

                // Check for "SPAWN" tag
                if ( entities->hasTag("SPAWN") ) {
                    float dx = (entities->x + entities->width*0.5f) - (victim->x + victim->width*0.5f);
                    float dy = (entities->y + entities->height*0.5f) - (victim->y + victim->height*0.5f);
                    float d = std::sqrt( dx*dx + dy*dy );

                    if ( d < nearestSpawn ) { 
                        nearestSpawn = d; 
                        spawn = entities; 
                    }
                }
            }

            if ( spawn ) {
                player->x = spawn->x;
                player->y = spawn->y;
                player->velY = 0.0f;
                player->velX = 0.0f;

                Camera::getInstance().smoothTo( spawn->x - 200.0f, spawn->y - 100.0f, 0.4f );
            }
        }
    }

private:
    EventManager& events_;
};