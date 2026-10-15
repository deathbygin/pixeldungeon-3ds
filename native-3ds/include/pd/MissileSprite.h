#pragma once

#include "noosa/Image.h"
#include "noosa/TextureFilm.h"
#include "pd/Assets.h"
#include <functional>
#include <memory>

namespace pd {

class Item;

class MissileSprite : public noosa::Image {
public:
    static constexpr float SPEED = 240.0f; // Matching Java MissileSprite.SPEED = 240f

    bool active = false;
    float startX = 0.0f;
    float startY = 0.0f;
    float targetX = 0.0f;
    float targetY = 0.0f;

    float duration = 0.0f;
    float elapsed = 0.0f;
    float heightArc = 16.0f; // Max visual parabolic height in pixels
    float angularSpeed = 0.0f;

    std::function<void()> onComplete;

    MissileSprite();
    ~MissileSprite() override = default;

    static void initFilm(std::shared_ptr<noosa::TextureFilm> film);

    void reset(int fromCell, int toCell, int itemImage, std::function<void()> callback = nullptr);

    void update(float dt) override;
    void draw(const noosa::Camera* camera = nullptr) override;

private:
    static std::shared_ptr<noosa::TextureFilm> s_itemsFilm;
};

} // namespace pd

