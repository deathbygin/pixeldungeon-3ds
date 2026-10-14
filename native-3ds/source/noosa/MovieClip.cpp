#include "noosa/MovieClip.h"

namespace noosa {

MovieClip::MovieClip() = default;

MovieClip::MovieClip(std::shared_ptr<TextureFilm> film)
    : Image(std::move(film)) {}

void MovieClip::play(const Animation& anim, bool forceReset) {
    if (!forceReset && m_currentAnim.frames == anim.frames && !m_finished) {
        return;
    }

    m_currentAnim = anim;
    m_frameIndex = 0;
    m_elapsed = 0.0f;
    m_finished = false;
    m_paused = false;

    if (!m_currentAnim.frames.empty()) {
        setFrame(m_currentAnim.frames[0]);
    }
}

void MovieClip::pause() {
    m_paused = true;
}

void MovieClip::resume() {
    m_paused = false;
}

void MovieClip::restart() {
    m_frameIndex = 0;
    m_elapsed = 0.0f;
    m_finished = false;
    if (!m_currentAnim.frames.empty()) {
        setFrame(m_currentAnim.frames[0]);
    }
}

int MovieClip::currentFrame() const {
    if (m_frameIndex < m_currentAnim.frames.size()) {
        return m_currentAnim.frames[m_frameIndex];
    }
    return 0;
}

void MovieClip::update(float dt) {
    Image::update(dt);

    if (m_paused || m_finished || m_currentAnim.frames.empty()) {
        return;
    }

    m_elapsed += dt;
    float delay = (m_currentAnim.delay > 0.001f) ? m_currentAnim.delay : 0.1f;

    while (m_elapsed >= delay) {
        m_elapsed -= delay;
        m_frameIndex++;

        if (m_frameIndex >= m_currentAnim.frames.size()) {
            if (m_currentAnim.looped) {
                m_frameIndex = 0;
            } else {
                m_frameIndex = m_currentAnim.frames.size() - 1;
                m_finished = true;
                if (m_callback) {
                    m_callback(this);
                }
                break;
            }
        }
    }

    if (m_frameIndex < m_currentAnim.frames.size()) {
        setFrame(m_currentAnim.frames[m_frameIndex]);
    }
}

} // namespace noosa

