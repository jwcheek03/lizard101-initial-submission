class SpawnHandler : public EventHandler {
public:
    void onEvent(Event e);
}
void SpawnHandler::onEvent(Event e) {
    if (e.type == "Spawn")
        //handle the event
}