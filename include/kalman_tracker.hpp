#pragma once
#include "cloud_service.hpp"

class KalmanTracker {
public:
    KalmanTracker();
    void update(const FaceResult& measured);
    FaceResult predict();
    bool isActive() const { return active_; }
    void reset();

private:
    // State: [cx, cy, vx, vy]
    float cx_, cy_;
    float vx_, vy_;
    bool active_ = false;
    int missCount_ = 0;
    static constexpr int MAX_MISS = 10;
    static constexpr float ALPHA = 0.4f; // ölçüm ağırlığı
};