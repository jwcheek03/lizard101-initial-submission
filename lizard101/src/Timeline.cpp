#include "Timeline.h"
#include "Input.h"
#include <SDL3/SDL.h>

Timeline::Timeline(float scale)
	: ticSize(scale), elapsedTime(0.0f), currentDelta(0.0f), paused(false)
{
	lastTick = static_cast<float>(SDL_GetTicks());
}

void Timeline::update() {
	if (paused) {
		currentDelta = 0.0f;
		lastTick = static_cast<float>(SDL_GetTicks());
		return;
	}

	float currentTick = static_cast<float>(SDL_GetTicks());
	float rawDelta = (currentTick - lastTick) / 1000.0f;
	currentDelta = rawDelta * ticSize;
	elapsedTime += currentDelta;
	lastTick = currentTick;
	float deltaTime = getDeltaTime();
	// SDL_Log("DeltaTime = %.4f, Elapsed = %.2f", deltaTime, getElapsedTime()); // Debug for logging timeline
}

void Timeline::pause() {
	paused = true;
}

void Timeline::unpause() {
	paused = false;
	lastTick = static_cast<float>(SDL_GetTicks()); // Resets baseline
}

void Timeline::togglePause() {
	if (paused) unpause();
	else pause();
}

void Timeline::setScale(float scale) {
	ticSize = scale;
}

float Timeline::getDeltaTime() const {
	return currentDelta;
}

float Timeline::getElapsedTime() const {
	return elapsedTime;
}

float Timeline::getTicSize() const {
	return ticSize;
}

bool Timeline::isPaused() const {
	return paused;
}