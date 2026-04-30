#include "kalman_tracker.hpp"
#include <cmath>

KalmanTracker::KalmanTracker()
    : cx_(0), cy_(0), vx_(0), vy_(0) {}

void KalmanTracker::update(const FaceResult& m) {
    if (!active_) {
        cx_ = m.cx; cy_ = m.cy;
        vx_ = 0;    vy_ = 0;
        active_ = true;
    } else {
        float newVx = m.cx - cx_;
        float newVy = m.cy - cy_;
        vx_ = ALPHA * newVx + (1-ALPHA) * vx_;
        vy_ = ALPHA * newVy + (1-ALPHA) * vy_;
        cx_ = ALPHA * m.cx + (1-ALPHA) * (cx_ + vx_);
        cy_ = ALPHA * m.cy + (1-ALPHA) * (cy_ + vy_);
    }
    missCount_ = 0;
}

FaceResult KalmanTracker::predict() {
    FaceResult res;
    if (!active_) return res;

    missCount_++;
    if (missCount_ > MAX_MISS) {
        active_ = false;
        return res;
    }

    // Hız ile tahmin et
    cx_ += vx_ * 0.5f;
    cy_ += vy_ * 0.5f;

    res.cx = cx_;
    res.cy = cy_;
    res.found = true;
    return res;
}

void KalmanTracker::reset() {
    active_ = false;
    missCount_ = 0;
    vx_ = vy_ = 0;
}