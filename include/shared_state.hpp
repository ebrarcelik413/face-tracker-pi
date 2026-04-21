#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <opencv2/opencv.hpp>

// C++'ın her döngüde güncellediği yapı
struct TrackerState {
    // Yüz
    bool   face_found  = false;
    float  raw_cx      = 0, raw_cy  = 0;   // ham dedektör
    float  kal_cx      = 0, kal_cy  = 0;   // kalman çıktısı
    float  error_x     = 0, error_y = 0;   // görüntü merkezine hata

    // Servo
    int    pan_pw      = 1500;   // mikrosaniye
    int    tilt_pw     = 1500;

    // Güvenlik
    bool   aes_active  = true;

    // Latency (ms)
    double lat_capture = 0;
    double lat_detect  = 0;
    double lat_total   = 0;

    // Bağlantı
    bool   cloud_connected = false;
};

class SharedStateWriter {
public:
    // JSON dosyasına yaz — Flask bunu okur
    static void write(const TrackerState& s,
                      const std::string& path = "/tmp/tracker_state.json") {
        std::ostringstream j;
        j << "{\n"
          << "  \"face_found\":"       << (s.face_found ? "true" : "false") << ",\n"
          << "  \"raw_cx\":"           << s.raw_cx   << ",\n"
          << "  \"raw_cy\":"           << s.raw_cy   << ",\n"
          << "  \"kal_cx\":"           << s.kal_cx   << ",\n"
          << "  \"kal_cy\":"           << s.kal_cy   << ",\n"
          << "  \"error_x\":"          << s.error_x  << ",\n"
          << "  \"error_y\":"          << s.error_y  << ",\n"
          << "  \"pan_pw\":"           << s.pan_pw   << ",\n"
          << "  \"tilt_pw\":"          << s.tilt_pw  << ",\n"
          << "  \"aes_active\":"       << (s.aes_active ? "true" : "false") << ",\n"
          << "  \"cloud_connected\":"  << (s.cloud_connected ? "true" : "false") << ",\n"
          << "  \"lat_capture\":"      << s.lat_capture << ",\n"
          << "  \"lat_detect\":"       << s.lat_detect  << ",\n"
          << "  \"lat_total\":"        << s.lat_total   << "\n"
          << "}\n";

        // Atomik yaz — önce tmp'ye, sonra rename
        std::string tmp = path + ".tmp";
        std::ofstream f(tmp);
        if (f.is_open()) {
            f << j.str();
            f.close();
            std::rename(tmp.c_str(), path.c_str());
        }
    }

    // Kareyi JPEG olarak yaz — Flask stream için okur
static void writeFrame(const cv::Mat& frame,
                       const std::string& path = "/tmp/latest_frame.jpg") {
    // Geçici dosya da .jpg uzantılı olmalı
    std::string tmp = path + ".part.jpg";
    cv::imwrite(tmp, frame, {cv::IMWRITE_JPEG_QUALITY, 75});
    std::rename(tmp.c_str(), path.c_str());
}
};