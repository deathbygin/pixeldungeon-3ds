#pragma once

#include "Image.h"
#include <vector>
#include <functional>

namespace noosa {

struct Animation {
    float delay = 0.1f; // seconds per frame
    std::vector<int> frames;
    bool looped = true;

    Animation() = default;
    Animation(float d, std::vector<int> f, bool l = true)
        : delay(d), frames(std::move(f)), looped(l) {}
};

class MovieClip : public Image {
public:
    using AnimationCallback = std::function<void(MovieClip*)>;

    MovieClip();
    explicit MovieClip(std::shared_ptr<TextureFilm> film);
    ~MovieClip() override = default;

    void play(const Animation& anim, bool forceReset = false);
    void pause();
    void resume();
    void restart();

    void update(float dt) override;

    bool isFinished() const { return m_finished; }
    bool isPaused() const { return m_paused; }
    int currentFrame() const;

    void setCallback(AnimationCallback callback) { m_callback = std::move(callback); }

protected:
    Animation m_currentAnim;
    size_t m_frameIndex = 0;
    float m_elapsed = 0.0f;
    bool m_paused = false;
    bool m_finished = false;
    AnimationCallback m_callback = nullptr;
};

} // namespace noosa

