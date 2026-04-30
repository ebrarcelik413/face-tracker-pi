#pragma once
#include <vector>
#include <string>
#include "cloud_service.hpp"

class ControlService {
public:
    ControlService();
    void update(const FaceResult& result);
    float getPanPW()  const { return panPW_;  }
    float getTiltPW() const { return tiltPW_; }

private:
    float panPW_  = 1500.0f;
    float tiltPW_ = 1500.0f;
    float panAngle_  = 90.0f;
    float tiltAngle_ = 90.0f;
    float panIntegral_  = 0, panPrev_  = 0;
    float tiltIntegral_ = 0, tiltPrev_ = 0;

    static constexpr float Kp       = 0.4f;
    static constexpr float Ki       = 0.02f;
    static constexpr float Kd       = 0.08f;
    static constexpr float DEADBAND = 15.0f;
    static constexpr float IMG_W    = 640.0f;
    static constexpr float IMG_H    = 480.0f;

    float pidCompute(float error, float& integral, float& prevErr, float dt);
    static float clamp(float v, float lo, float hi);
    static float angleToPW(float angle);
};