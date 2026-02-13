#pragma once
#include "EventHandler.h"
#include <iostream>

class CollisionHandler : public EventHandler {
public:
    void onEvent(const Event& e) override {
        if (e.name == "Collision") {
            std::cout << "Handle collision\n";
        }
    }
};



// --- Event implementation example ---

//class ReplayRecorder : public EventHandler {
//public:
//	void onEvent(Event);
//	void setReplayLocation(string, filename);
//	void setReplayLocation(std::Vector<event>);
//private:
//	void startRecording();
//	void stopRecording();
//	void recordEvent();
//	std::Vector<event*> events;
//}


// --- Cooresponding Event call ---

//void ReplayRecorder::onEvent(Event e) {
//	if (e.type == “start_replay_recording”)
//		startRecording();
//	else if (e.type == “stop_replay_recording”)
//		stopRecording();
//	else
//		recordEvent(e);
//}
