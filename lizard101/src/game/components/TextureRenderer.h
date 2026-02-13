#pragma once
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "ecs/Component.h"
#include "../Camera.h"

// Currently TextureRenderer supports static rendering OR sprite-sheet animation.
class TextureRenderer : public ecs::Component {
public:
    explicit TextureRenderer(SDL_Renderer* renderer, const std::string& filePath)
        : renderer_(renderer) {
        loadFromFile(filePath);
    }

    TextureRenderer(SDL_Renderer* renderer,
        const std::string& filePath,
        int frameW, int frameH,
        int columns, int rows,
        float fps)
        : renderer_(renderer) {
        loadFromFile(filePath);
        setAnimation(frameW, frameH, columns, rows, fps);
    }

    ~TextureRenderer() override { if (texture_) SDL_DestroyTexture(texture_); }

    void setSourceRect(float x, float y, float w, float h) {
        hasStaticSrc_ = (w > 0 && h > 0);
        staticSrc_ = SDL_FRect{ x, y, w, h };
    }

    void setAnimation(int frameW, int frameH, int columns, int rows, float fps) {
        frameW_ = frameW; frameH_ = frameH; fps_ = fps;

        float tw = 0, th = 0;
        if (texture_) SDL_GetTextureSize(texture_, &tw, &th);
        texW_ = static_cast<int>(tw);
        texH_ = static_cast<int>(th);

        columns_ = (columns > 0) ? columns : (frameW_ > 0 ? texW_ / frameW_ : 0);
        rows_ = (rows > 0) ? rows : (frameH_ > 0 ? texH_ / frameH_ : 0);

        animated_ = (frameW_ > 0 && frameH_ > 0 && fps_ > 0 && columns_ > 0 && rows_ > 0);
        currentRow_ = (rows_ > 0) ? (currentRow_ % rows_) : 0;
        currentFrame_ = (columns_ > 0) ? (currentFrame_ % columns_) : 0;
    }

    void setPaused(bool paused) { paused_ = paused; }
    void setRow(int row) { currentRow_ = (rows_ > 0) ? ((row % rows_ + rows_) % rows_) : 0; }
    int  row() const { return currentRow_; }

    void setFrame(int frame) { currentFrame_ = (columns_ > 0) ? ((frame % columns_ + columns_) % columns_) : 0; }
    int  frame() const { return currentFrame_; }

    void setFPS(float fps) { fps_ = fps; animated_ = (frameW_ > 0 && frameH_ > 0 && fps_ > 0 && columns_ > 0 && rows_ > 0); }

    void stepFrame() { if (columns_ > 0) currentFrame_ = (currentFrame_ + 1) % columns_; }

    void onUpdate(float dt) override {
        if (!animated_ || paused_) return;
        timeAcc_ += dt;
        const float frameTime = (fps_ > 0.f) ? (1.f / fps_) : 0.f;
        while (frameTime > 0.f && timeAcc_ >= frameTime) {
            timeAcc_ -= frameTime;
            stepFrame();
        }
    }

    void onRender(SDL_Renderer* renderer) override {
        Entity* e = getOwner();
        if (!e) return;

        SDL_FRect worldDst{ e->getX(), e->getY(),
                            static_cast<float>(e->getWidth()),
                            static_cast<float>(e->getHeight()) };
        SDL_FRect dst = Camera::getInstance().worldToScreen(worldDst);

        // --- Fallback: draw solid red box if no texture ---
        if (!texture_) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &dst);
            return;
        }

        const SDL_FRect* src = nullptr;
        SDL_FRect local{};

        if (animated_) {
            local.x = static_cast<float>(currentFrame_ * frameW_);
            local.y = static_cast<float>(currentRow_ * frameH_);
            local.w = static_cast<float>(frameW_);
            local.h = static_cast<float>(frameH_);
            src = &local;
        }
        else if (hasStaticSrc_) {
            src = &staticSrc_;
        }
        else {
            src = nullptr; // whole texture
        }

        SDL_RenderTexture(renderer, texture_, src, &dst);
    }

private:
    bool loadFromFile(const std::string& filePath) {
        if (texture_) { SDL_DestroyTexture(texture_); texture_ = nullptr; }
        texture_ = IMG_LoadTexture(renderer_, filePath.c_str());
        if (!texture_) {
            SDL_Log("TextureRenderer: failed to load '%s': %s",
                filePath.c_str(), SDL_GetError());
            return false;
        }
        float tw = 0, th = 0;
        SDL_GetTextureSize(texture_, &tw, &th);
        texW_ = static_cast<int>(tw);
        texH_ = static_cast<int>(th);
        return true;
    }

private:
    SDL_Renderer* renderer_ = nullptr; // not owned
    SDL_Texture* texture_ = nullptr; // owned

    bool      hasStaticSrc_ = false;
    SDL_FRect staticSrc_{ 0,0,0,0 };

    bool  animated_ = false;
    bool  paused_ = false;
    int   frameW_ = 0;
    int   frameH_ = 0;
    int   columns_ = 0;
    int   rows_ = 0;
    float fps_ = 0.f;

    int   currentRow_ = 0;
    int   currentFrame_ = 0;
    float timeAcc_ = 0.f;

    int texW_ = 0, texH_ = 0;
};
