#include "control_service.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

ControlService::ControlService() {}

float ControlService::clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

float ControlService::angleToPW(float angle) {
    // 0° → 500µs, 180° → 2500µs
    return 500.0f + (angle / 180.0f) * 2000.0f;
}

float ControlService::pidCompute(float error, float& integral,
                                  float& prevErr, float dt) {
    integral += error * dt;
    integral  = clamp(integral, -100.0f, 100.0f); // anti-windup
    float derivative = (error - prevErr) / dt;
    prevErr = error;
    return Kp * error + Ki * integral + Kd * derivative;
}

void ControlService::update(const FaceResult& r) {
    if (!r.found) return;

    float dt = 0.033f; // ~30Hz

    // Hata: yüz merkezi - görüntü merkezi
    float ex = r.cx - IMG_W / 2.0f;
    float ey = r.cy - IMG_H / 2.0f;

    // Deadband: küçük hatalarda hareket etme (titreşim önleme)
    if (std::fabs(ex) < DEADBAND) ex = 0;
    if (std::fabs(ey) < DEADBAND) ey = 0;

    float dPan  = pidCompute(ex,  panIntegral_,  panPrev_,  dt);
    float dTilt = pidCompute(ey,  tiltIntegral_, tiltPrev_, dt);

    // Açıyı güncelle (pan: sağa+, tilt: aşağı+)
    panAngle_  = clamp(panAngle_  + dPan  * 0.05f, 10.0f, 170.0f);
    tiltAngle_ = clamp(tiltAngle_ + dTilt * 0.05f, 30.0f, 150.0f);

    panPW_  = angleToPW(panAngle_);
    tiltPW_ = angleToPW(tiltAngle_);

    // Servo motor bağlandığında buraya PWM yazma kodu gelecek
    // Şimdilik sadece değerleri hesaplıyoruz
    std::cout << "[SERVO] pan=" << (int)panAngle_ << "° ("
              << (int)panPW_ << "µs) tilt=" << (int)tiltAngle_
              << "° (" << (int)tiltPW_ << "µs)" << std::endl;
}