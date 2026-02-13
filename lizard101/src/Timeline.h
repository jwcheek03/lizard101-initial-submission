#pragma once

class Timeline {
private:
	float ticSize;
	float elapsedTime;
	float lastTick;
	float currentDelta;
	bool paused;

public:
	float getDeltaTime() const;

	Timeline(float scale = 1.0f);

	void update();
	void pause();
	void unpause();
	void togglePause();
	void setScale(float scale);
	float getElapsedTime() const;
	float getTicSize() const;
	bool isPaused() const;
};