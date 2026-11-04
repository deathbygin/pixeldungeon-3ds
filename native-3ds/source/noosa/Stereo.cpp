#include "noosa/Stereo.h"
#include <cmath>

namespace noosa {

float Stereo::s_slider = 0.0f;
float Stereo::s_eyeSign = 0.0f;
float Stereo::s_viewShift = 0.0f;

void Stereo::beginFrame() {
    s_slider = osGet3DSliderState();
}

void Stereo::beginEye(Eye eye) {
    // An object in front of the screen sits further right in the left eye's image
    s_eyeSign = (eye == Eye::Left) ? 1.0f : ((eye == Eye::Right) ? -1.0f : 0.0f);
    applyShift(0.0f);
}

void Stereo::endEye() {
    s_eyeSign = 0.0f;
    applyShift(0.0f);
}

void Stereo::setPlane(float plane) {
    applyShift(snappedShift(plane));
}

void Stereo::setDepth(float plane) {
    applyShift(shift(plane));
}

float Stereo::snappedShift(float plane) {
    return std::round(shift(plane));
}

void Stereo::applyShift(float px) {
    if (px == s_viewShift) return;

    C2D_ViewReset();
    C2D_ViewTranslate(px, 0.0f);
    s_viewShift = px;
}

} // namespace noosa
