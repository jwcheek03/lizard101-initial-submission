#pragma once
#include "ecs/Component.h"
#include "EntityManager.h"
#include "Entity.h"
#include "../Camera.h"
#include "ScreenAnchor.h"
#include <chrono>
#include <cmath>
#include <algorithm>

// SideScroll component: when the player collides with this invisible boundary, translate all entities horizontally by 
// an offset to simulate side-scrolling.
class SideScroll : public ecs::Component {

public:

	SideScroll(float shiftX = 800.0f, float cooldownSeconds = 0.1f, float boundaryMoveAfter = 800.0f, bool moveBoundary = true)
		: shiftX_(shiftX), cooldown_(cooldownSeconds), boundaryMoveAfter_(boundaryMoveAfter), moveBoundary_(moveBoundary) {}

	void onStart() override {
		prevColliding_ = false;
		timer_ = 0.0f;
        // initialize lastPlayerX_ if player exists
        if (Entity* p = EntityManager::getInstance().findEntityByName("Player")) {
            lastPlayerX_ = p->x;
        }
	}

	void onUpdate(float dt) override {

		// Handle temporary disable/rearm for screen-anchored boundaries
		if (disabled_) {
			rearmTimer_ += dt;
			if (rearmTimer_ >= rearmSeconds_) {
				disabled_ = false;
				rearmTimer_ = 0.0f;
			}

			// Update lastPlayerX_ to avoid large jumps when re-enabled
			if (Entity* ptmp = EntityManager::getInstance().findEntityByName("Player")) {
				lastPlayerX_ = ptmp->x;
			}
			return;
		}

		timer_ += dt;
		Entity* player = EntityManager::getInstance().findEntityByName("Player");
		Entity* owner = getOwner();
		
		if (!player || !owner) 
			return;

		// Check for player collision with boundary
		bool coll = collisionDetection(*owner, *player);
		float dx = player->x - lastPlayerX_;

		// Minimum movement threshold to consider approaching the boundary
		const float moveThreshold = 1.0f;

		// Determine if we should trigger side scroll
		bool triggerScroll = false;

		// Normal trigger: first collision frame and moving toward boundary
		bool approaching = (shiftX_ > 0.0f) ? (dx > moveThreshold) : (dx < -moveThreshold);
		if (coll && !prevColliding_ && timer_ >= cooldown_ && approaching) {
			triggerScroll = true;
		}

		// Fallback trigger: player stuck inside boundary for too long
		float stuckThreshold = 0.1f; // seconds of continuous collision
		if (coll && prevColliding_ && timer_ >= stuckThreshold) {
			triggerScroll = true;
		}

		if (triggerScroll) {
			float absShift = std::fabs(shiftX_);
			float appliedShift = (shiftX_ > 0.0f) ? absShift : -absShift;

			float camX = Camera::getInstance().getX();
			float targetCamX = camX + appliedShift;
			float clampedTarget = std::clamp(targetCamX, Camera::getInstance().getMinX(), Camera::getInstance().getMaxX());
			float effectiveShift = clampedTarget - camX;

			if (std::fabs(effectiveShift) > 0.001f) {
				// Move camera smoothly
				Camera::getInstance().smoothTranslate(effectiveShift, 0.0f, 0.35f);
				SDL_Log("SideScroll: camera smoothTranslate by %.1f (target cam x=%.1f)", effectiveShift, camX + effectiveShift);

				// Move boundary if allowed
				if (moveBoundary_) {
					if (auto* anchor = owner->getComponent<ScreenAnchor>()) {
						disabled_ = true;
						rearmTimer_ = 0.0f;
						SDL_Log("SideScroll: screen-anchored boundary '%s' triggered; temporarily disabled for %.2fs", owner->name.c_str(), rearmSeconds_);
					} else {
						owner->x += (effectiveShift >= 0.0f ? 1.0f : -1.0f) * boundaryMoveAfter_;
						SDL_Log("SideScroll: moved boundary '%s' by %.1f to x=%.1f", owner->name.c_str(), (effectiveShift >= 0.0f ? 1.0f : -1.0f) * boundaryMoveAfter_, owner->x);
					}
				}

				// Push player out of the boundary to avoid getting stuck
				float playerLeft = player->x;
				float playerRight = player->x + player->width;
				float boundaryLeft = owner->x;
				float boundaryRight = owner->x + owner->width;

				// Right boundary
				if (shiftX_ > 0.0f && playerRight > boundaryLeft) {         
					player->x = boundaryLeft - player->width - 0.1f;
				}

				// Left boundary
				else if (shiftX_ < 0.0f && playerLeft < boundaryRight) { 
					player->x = boundaryRight + 0.1f;
				}

			} else {
				SDL_Log("SideScroll: camera clamped, no move (requested %.1f, cam=%.1f bounds=[%.1f,%.1f])", appliedShift, camX, Camera::getInstance().getMinX(), Camera::getInstance().getMaxX());
			}

			timer_ = 0.0f;
		}

		prevColliding_ = coll;
		lastPlayerX_ = player->x;
	}


private:
	float shiftX_ = 800.0f;
	float cooldown_ = 0.1f;
	float timer_ = 0.0f;
	bool prevColliding_ = false;
	float lastPlayerX_ = 0.0f;
	float boundaryMoveAfter_ = 800.0f;
	bool moveBoundary_ = true;
	bool disabled_ = false;
	float rearmSeconds_ = 0.5f;
	float rearmTimer_ = 0.0f;
};

